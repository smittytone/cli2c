/*
 * LTP305 2x5x7 matrix driver
 *
 * Version 1.2.0
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


/*
 * STATIC PROTOTYPES
 */
static void LTP305_write_cmd(uint8_t cmd, uint8_t value);
static void LTP305_write_buffers(void);
static uint32_t LTP305_bcd(uint32_t base);


/*
 * GLOBALS
 */
static uint8_t left_buffer[9]  = { MATRIX_LEFT_ADDR, 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t right_buffer[9] = { MATRIX_RIGHT_ADDR, 0, 0, 0, 0, 0, 0, 0, 0 };

// The I2C bus
static I2CDriver* host_i2c;
static int i2c_address = IS31FL3730_I2C_ADDR;
static int brightness  = IS31FL3730_DEFAULT_BRIGHT;

extern const char CHARSET[128][6];


void LTP305_init(I2CDriver *sd, int address) {

    if (address != -1) i2c_address = address;
    host_i2c = sd;
}


void LTP305_set_brightness(int new_brightess) {
    
    brightness = new_brightess;
    LTP305_write_cmd(IS31FL3730_PWM_REG, brightness);
    LTP305_write_cmd(IS31FL3730_UPDATE_COL_REG, 0x01);
}


void LTP305_clear_buffers() {

    memset(&left_buffer[1],  0x00, 8);
    memset(&right_buffer[1], 0x00, 8);
}


void LTP305_set_glyph(uint8_t led, uint8_t* glyph) {

    switch(led) {
        case LEFT:
            for (uint i = 0 ; i < 5 ; ++i) {
                uint8_t d = 0;
                for (uint j = 0 ; j < 7 ; ++j) {
                    d |= (glyph[j] & (1 << i)) << (5 - i);
                }  
                left_buffer[i + 1] = d;
            }
            break;
        case RIGHT:
            for (uint i = 0 ; i < 7 ; ++i) {
                uint8_t d = 0;
                for (uint j = 0 ; j < 5 ; ++j) {
                    d |= (glyph[i] & (1 << (5 - j)));
                }
                right_buffer[i + 1] = d;
            }
    }
}


void LTP305_set_char(uint8_t led, uint8_t ascii) {

    switch(led) {
        case LEFT:
            for (uint i = 0 ; i < 8 ; ++i) {
                uint8_t a = CHARSET[ascii - 32][i];
                if (a == 0) break;
                uint8_t b = 0;
                for (uint j = 0 ; j < 8 ; ++j) {
                    b |= (((a & (1 << j)) >> j) << (7 - j));
                }
                left_buffer[i + 1] = b;
            }
            break;
        case RIGHT:
            for (uint i = 0 ; i < 7 ; ++i) {
                uint8_t a = CHARSET[ascii - 32][i];
                if (a == 0) break;
                for (uint j = 0 ; j < 7 ; ++j) {
                    uint8_t b = (a & (1 << (7 - j)));
                    right_buffer[j + 1] |= ((b > 0 ? 1 : 0) << i);
                }
            }
    }
}


void LTP305_plot(uint8_t led, uint8_t x, uint8_t y, bool ink) {

    assert (x < 5);
    assert (y < 7);

    switch(led) {
        case LEFT:
            {
                uint8_t byte = left_buffer[x + 1];
                if (ink) {
                    byte |= (1 << y);
                } else {
                    byte &= ~(1 << y);
                }
                left_buffer[x + 1] = byte;
                break;
            }
        case RIGHT:
            {
                uint8_t byte = left_buffer[y + 1];
                x = 5 - x;
                if (ink) {
                    byte |= (1 << x);
                } else {
                    byte &= ~(1 << x);
                }
                right_buffer[y + 1] = byte;
            }
    }
}


void LTP305_set_point(uint8_t led) {

    switch(led) {
        case LEFT:
            left_buffer[8] = 0x40; 
            break;
        case RIGHT:
            right_buffer[7] |= 0x80;
    }
}


void LTP305_clear() {

    LTP305_clear_buffers();
    LTP305_draw();
}


void LTP305_draw() {
    
    LTP305_write_buffers();
    LTP305_write_cmd(IS31FL3730_CONFIG_REG, 0x18);
    LTP305_write_cmd(IS31FL3730_LIGHT_EFFECT_REG, 0x0E);
    LTP305_write_cmd(IS31FL3730_PWM_REG, brightness);
    LTP305_write_cmd(IS31FL3730_UPDATE_COL_REG, 0x01);
}


static void LTP305_write_cmd(uint8_t cmd, uint8_t value) {

    // NOTE Already connected at this stage
    uint8_t data[2] = {cmd, value};
    i2c_start(host_i2c, i2c_address, 0);
    i2c_write(host_i2c, data, 2);
    i2c_stop(host_i2c);
}


static void LTP305_write_buffers() {

    i2c_start(host_i2c, i2c_address, 0);
    i2c_write(host_i2c, left_buffer, 9);
    i2c_write(host_i2c, right_buffer, 9);
    i2c_stop(host_i2c);
}


/**
 * @brief Convert a 16-bit value (0-9999) to BCD notation.
 *
 * @param base: The value to convert.
 *
 * @retval The BCD form of the value.
 */
static uint32_t LTP305_bcd(uint32_t base) {

    if (base > 9999) base = 9999;
    for (uint32_t i = 0 ; i < 16 ; ++i) {
        base = base << 1;
        if (i == 15) break;
        if ((base & 0x000F0000) > 0x0004FFFF) base += 0x00030000;
        if ((base & 0x00F00000) > 0x004FFFFF) base += 0x00300000;
        if ((base & 0x0F000000) > 0x04FFFFFF) base += 0x03000000;
        if ((base & 0xF0000000) > 0x4FFFFFFF) base += 0x30000000;
    }

    return (base >> 16) & 0xFFFF;
}
