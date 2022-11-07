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
static void     LTP305_write_register(uint8_t reg, uint8_t value, bool do_stop);
static void     LTP305_write_buffers(void);


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


/**
 * @brief Set up the data the driver needs.
 *
 * @param sd:       Pointer to the main I2C driver data structure.
 * @param address:  A non-standard HT16K33 address, or -1.
 */
void LTP305_init(I2CDriver *sd, int address) {

    if (address != -1) i2c_address = address;
    host_i2c = sd;
}


/**
 * @brief Set the display brightness.
 *
 * @param brightness: The display brightness (1-127).
 */
void LTP305_set_brightness(int new_brightess) {
    
    brightness = new_brightess;
    LTP305_write_register(IS31FL3730_PWM_REG, brightness, false);
    LTP305_write_register(IS31FL3730_UPDATE_COL_REG, 0x01, true);
}


/**
 * @brief Zero the display's two buffers.
 */
void LTP305_clear_buffers() {

    memset(&left_buffer[1],  0x00, 8);
    memset(&right_buffer[1], 0x00, 8);
}


/**
 * @brief Set a user-defined character on the display.
 *
 * @param row:   The row at which the glyph will be drawn.
 * @param glyph: Pointer to an array of bytes, one per column.
 *               A row's bit 0 is the bottom.
 * @param width: The number of rows in the glypp (1-10).
 */
void LTP305_set_glyph(uint8_t row, uint8_t* glyph, size_t width) {

    assert(width < 11);
    assert(row < 10);
    
    for (size_t x = 0 ; x < width ; ++x) {
        uint8_t c = glyph[x];
        uint8_t b = 0;
        if (x + row < 5) {
            for (size_t y = 0 ; y < 8 ; ++y) {
                b |= (((c & (1 << y)) >> y) << (7 - y));
            }
            left_buffer[x + 1 + row] = b;
        } else {
            for (size_t y = 0 ; y < 8 ; ++y) {
                b = (c & (1 << (7 - y)));
                right_buffer[y + 1] |= ((b > 0 ? 1 : 0) << (x - 5));
            }
        }
    }
}


/**
 * @brief Set an Ascii character on the display.
 *
 * @param led:   The LED (0 or 1) on which the character will be drawn.
 * @param ascii: The character's Ascii value (32-127).
 */
void LTP305_set_char(uint8_t led, uint8_t ascii) {
    
    assert(ascii > 31 && ascii < 128);
    assert(led == LEFT || led == RIGHT);
    
    size_t d = 0;
    size_t cols = strlen(CHARSET[ascii - 32]);
    if (cols < 5) d = (5 - cols) >> 1;
    
    switch(led) {
        case LEFT:
            //    y 6543210
            //     01111111 x 0
            //     01111111 x 1
            //     01111111 x 2
            //     01111111 x 3
            //     01111111 x 4
            if (cols % 2 == 0) d +=1;
            for (size_t x = 0 ; x < 8 ; ++x) {
                if (x == cols) break;
                uint8_t c = CHARSET[ascii - 32][x];
                uint8_t b = 0;
                for (size_t y = 0 ; y < 8 ; ++y) {
                    b |= (((c & (1 << y)) >> y) << (7 - y));
                }
                left_buffer[x + 1 + d] = b;
            }
            break;
        case RIGHT:
            //       x  43210
            //       00011111 y 0
            //       00011111 y 1
            //       00011111 y 2
            //       00011111 y 3
            //       00011111 y 4
            //       00011111 y 5
            //       10011111 y 6 (bit 8 = decimal point)
            for (size_t x = 0 ; x < 8 ; ++x) {
                if (x == cols) break;
                uint8_t c = CHARSET[ascii - 32][x];
                for (size_t y = 0 ; y < 8 ; ++y) {
                    uint8_t b = (c & (1 << (7 - y)));
                    right_buffer[y + 1] |= ((b > 0 ? 1 : 0) << (x + d));
                }
            }
    }
}


/**
 * @brief Plot a point on the display. The two LEDs are treated
 *        as a single 10 x 7 array.
 *
 * @param x:   The X co-ordinate.
 * @param y:   The Y co-orindate.
 * @param ink: Whether to set (`true`) or clear (`false`) the pixel.
 */
void LTP305_plot(uint8_t x, uint8_t y, bool ink) {

    assert (x < 10);
    assert (y < 7);

    if (x < 5) {
        uint8_t byte = left_buffer[x + 1];
        if (ink) {
            byte |= (1 << y);
        } else {
            byte &= ~(1 << y);
        }
        left_buffer[x + 1] = byte;
    } else {
        uint8_t byte = right_buffer[y + 1];
        x -= 5;
        if (ink) {
            byte |= (1 << x);
        } else {
            byte &= ~(1 << x);
        }
        right_buffer[y + 1] = byte;
    }
}


/**
 * @brief Plot an LED's decimal point.
 *
 * @param led: The LED (0 or 1).
 */
void LTP305_set_point(uint8_t led) {

    switch(led) {
        case LEFT:
            left_buffer[8] = 0x40; 
            break;
        case RIGHT:
            right_buffer[7] |= 0x80;
    }
}


/**
 * @brief Clear the display and update it.
 */
void LTP305_clear() {

    LTP305_clear_buffers();
    LTP305_draw();
}


/**
 * @brief Update the display.
 */
void LTP305_draw() {
    
    LTP305_write_buffers();
    LTP305_write_register(IS31FL3730_CONFIG_REG, 0x18, false);
    LTP305_write_register(IS31FL3730_LIGHT_EFFECT_REG, 0x0E, false);
    LTP305_write_register(IS31FL3730_PWM_REG, brightness, false);
    LTP305_write_register(IS31FL3730_UPDATE_COL_REG, 0x01, true);
}


/**
 * @brief Write a value to a display register.
 *
 * @param reg:     The display register.
 * @param value:   The value to write.
 * @param do_stop: Issue a STOP if `true`.
 */
static void LTP305_write_register(uint8_t reg, uint8_t value, bool do_stop) {

    // NOTE Already connected at this stage
    uint8_t data[2] = {reg, value};
    i2c_start(host_i2c, i2c_address);
    i2c_write(host_i2c, data, 2);
    if (do_stop) i2c_stop(host_i2c);
}


/**
 * @brief Write the two LED buffers to the display.
 */
static void LTP305_write_buffers() {

    i2c_start(host_i2c, i2c_address);
    i2c_write(host_i2c, left_buffer, 9);
    i2c_write(host_i2c, right_buffer, 9);
}


