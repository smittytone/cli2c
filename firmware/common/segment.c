/*
 * I2C Host Firmware - Debugging LED segment driver
 *
 * @version     0.1.6
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "segment.h"


// The hex character set
char        CHARSET[19] = "\x3F\x06\x5B\x4F\x66\x6D\x7D\x07\x7F\x6F\x5F\x7C\x58\x5E\x7B\x71\x40\x63";

// Map display digits to bytes in the buffer
uint8_t     POS[4] = {0, 2, 6, 8};
uint8_t     display_buffer[17];


/*
 * HT16K33 Functions
 */

/**
 * @brief Power on the LEDs and set the brightness.
 */
void HT16K33_init(void) {
    
    HT16K33_write_cmd(0x21);     // System on
    HT16K33_write_cmd(0x81);     // Display on
    HT16K33_write_cmd(0xEF);     // Set brightness
    HT16K33_clear_buffer();
}


/**
 * @brief Issue a single command byte to the HT16K33.
 *
 * @param cmd: The single-byte command.
 */
void HT16K33_write_cmd(uint8_t cmd) {
    
    i2c_write_blocking(i2c1, DEBUG_SEG_ADDR, &cmd, 1, false);
}


/**
 * @brief Clear the display buffer.
 *        This does not clear the LED -- call `HT16K33_draw()`.
 */
void HT16K33_clear_buffer(void) {
    
    memset(display_buffer, 0x00, 8);
}


/**
 * @brief Write the display buffer out to the LED.
 */
void HT16K33_draw(void) {
    
    // Set up the buffer holding the data to be
    // transmitted to the LED
    uint8_t tx_buffer[17] = { 0 };
    memcpy(tx_buffer + 1, display_buffer, 16);

    // Display the buffer and flash the LED
    i2c_write_blocking(i2c1, DEBUG_SEG_ADDR, tx_buffer, sizeof(tx_buffer), false);
}


/**
 * @brief Write a single-digit hex number to the display buffer at the specified digit.
 *
 * @param  number:  The value to write.
 * @param  digit:   The digit that will show the number.
 * @param  has_dot: `true` if the digit's decimal point should be lit,
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
 * @param glyph:   The glyph to write.
 * @param digit:   The digit that will show the number.
 * @param has_dot: `true` if the digit's decimal point should be lit,
 *                  `false` otherwise.
 */
void HT16K33_set_glyph(uint8_t glyph, uint8_t digit, bool has_dot) {
    
    if (digit > 3) return;
    display_buffer[POS[digit]] = glyph;
    if (has_dot) display_buffer[POS[digit]] |= 0x80;
}


/**
 * @brief Write a positive decimal value to the entire 4-digit display buffer.
 *
 * @param value:   The value to write.
 * @param decimal: `true` if digit 1's decimal point should be lit,
 *                  `false` otherwise.
 */
void HT16K33_show_value(int16_t value, bool decimal) {
    
    // Convert the value to BCD...
    uint16_t bcd_val = bcd(value);
    HT16K33_clear_buffer();
    HT16K33_set_number((bcd_val >> 12) & 0x0F, 0, false);
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
uint32_t bcd(uint32_t base) {
    
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