/*
 * HT16K33 4-digit, 7-segment driver
 *
 * Version 0.1.2
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


// The hex character set
char CHARSET[19] = "\x3F\x06\x5B\x4F\x66\x6D\x7D\x07\x7F\x6F\x5F\x7C\x58\x5E\x7B\x71\x40\x63";

// Map display digits to bytes in the buffer
uint8_t POS[4] = {1, 3, 7, 9};
uint8_t display_buffer[17] = {0};
bool is_flipped = false;

// The I2C bus
I2CDriver* host_i2c;
int i2c_address = HT16K33_I2C_ADDR;


/**
 * @brief Set up the data the driver needs.
 *
 * @param sd:          Pointer to the main I2C driver data structure.
 * @param address:     A non-standard HT16K33 address, or -1.
 */
void HT16K33_init(I2CDriver *sd, int address) {
    
    if (address != -1) i2c_address = address;
    host_i2c = sd;
}


/**
 * @brief Flip the display through 180 degrees.
 */
void HT16K33_flip(void) {
    
    is_flipped = !is_flipped;
}


/**
 * @brief Power the display on or off
 *
 * @param is_on: Whether to power up (`true`) the display or
 *               shut it down (`false`).
 */
void HT16K33_power(bool is_on) {
    
    if (is_on) {
        HT16K33_write_cmd(HT16K33_CMD_POWER_ON);
        HT16K33_write_cmd(HT16K33_CMD_DISPLAY_ON);
    } else {
        HT16K33_write_cmd(HT16K33_CMD_DISPLAY_OFF);
        HT16K33_write_cmd(HT16K33_CMD_POWER_OFF);
    }
}


/**
 * @brief Set the display brightness.
 *
 * @param brightness: The display brightness (1-15).
 */
void HT16K33_set_brightness(uint8_t brightness) {
    
    if (brightness > 15) brightness = 15;
    HT16K33_write_cmd(HT16K33_CMD_BRIGHTNESS | brightness);
}


/**
 * @brief Clear the display buffer.
 *
 * This does not clear the LED -- call `HT16K33_draw()`.
 */
void HT16K33_clear_buffer(void) {
    
    memset(&display_buffer[1], 0x00, 16);
}


/**
 * @brief Write the display buffer out to the LED.
 */
void HT16K33_draw(void) {
    
    // Check for an overturned LED
    if (is_flipped) {
        printf("FLIPPED\n");
        // Swap digits 0,3 and 1,2
        uint8_t a = display_buffer[POS[0]];
        display_buffer[POS[0]] = display_buffer[POS[3]];
        display_buffer[POS[3]] = a;
            
        a = display_buffer[POS[1]];
        display_buffer[POS[1]] = display_buffer[POS[2]];
        display_buffer[POS[2]] = a;
            
        // Rotate each digit
        for (uint32_t i = 0 ; i < 4 ; ++i) {
            a = display_buffer[POS[i]];
            uint8_t b = (a & 0x07) << 3;
            uint8_t c = (a & 0x38) >> 3;
            a &= 0xC0;
            display_buffer[POS[i]] = (a | b | c);
        }
    }
    
    // Display the buffer and flash the LED
    i2c_start(host_i2c, i2c_address, 0);
    i2c_write(host_i2c, display_buffer, 17);
    i2c_stop(host_i2c);
}


/**
 *  @brief Write a single-digit hex number to the display buffer at the specified digit.
 *
 *  @param  number:  The value to write.
 *  @param  digit:   The digit that will show the number.
 *  @param  has_dot: `true` if the digit's decimal point should be lit,
 *                   `false` otherwise.
 */
void HT16K33_set_number(uint8_t number, uint8_t digit, bool has_dot) {
    
    if (digit > 3) return;
    if (number > 15) return;
    display_buffer[POS[digit]] = CHARSET[number];
}


/**
 * @brief Write a single character glyph to the display buffer at the specified digit.
 *
 *  Glyph values are 8-bit integers representing a pattern of set LED segments.
 *  The value is calculated by setting the bit(s) representing the segment(s) you want illuminated.
 *  Bit-to-segment mapping runs clockwise from the top around the outside of the matrix; the inner segment is bit 6:
 *       0
 *       _
 *   5 |   | 1
 *     |   |
 *       - <----- 6
 *   4 |   | 2
 *     | _ |
 *       3
 *
 *  @param glyph:   The glyph to write.
 *  @param digit:   The digit that will show the number.
 *  @param has_dot: `true` if the digit's decimal point should be lit,
 *                  `false` otherwise.
 */
void HT16K33_set_glyph(uint8_t glyph, uint8_t digit, bool has_dot) {
    
    if (digit > 3) return;
    display_buffer[POS[digit]] = glyph;
    if (has_dot) display_buffer[POS[digit]] |= 0x80;
}


/**
 * @brief Write a decimal value to the entire 4-digit display buffer.
 *
 *  @param value:   The value to write.
 *  @param decimal: `true` if digit 1's decimal point should be lit,
 *                  `false` otherwise.
 */
void HT16K33_show_value(int value, bool decimal) {
    
    bool is_neg = false;
    if (value < 0) {
        is_neg = true;
        value *= -1;
    }
    
    // Convert the value to BCD...
    uint16_t bcd_val = bcd(value);
    HT16K33_clear_buffer();
    
    if (is_neg) {
        HT16K33_set_glyph(0x40, 0, false);
    } else {
        HT16K33_set_number((bcd_val >> 12) & 0x0F, 0, false);
    }
    
    HT16K33_set_number((bcd_val >> 8)  & 0x0F, 1, decimal);
    HT16K33_set_number((bcd_val >> 4)  & 0x0F, 2, false);
    HT16K33_set_number(bcd_val & 0x0F,         3, false);
}


/**
 * @brief Convert a 16-bit value (0-9999) to BCD notation.
 *
 * @param base: The value to convert.
 *
 * @retval The BCD form of the value.
 */
static uint32_t bcd(uint32_t base) {
    
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


/**
 * @brief Sleep the thread for the specified period in milliseconds.
 *
 * @param ms: The sleep period.
 */
static void HT16K33_sleep_ms(int ms) {
    
    struct timespec ts;
    int res;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
}


/**
 * @brief Issue a single command byte to the HT16K33.
 *
 * @param cmd: The single-byte command.
 */
static void HT16K33_write_cmd(uint8_t cmd) {
    
    // NOTE Already connected at this stage
    i2c_start(host_i2c, i2c_address, 0);
    i2c_write(host_i2c, &cmd, 1);
    i2c_stop(host_i2c);
}