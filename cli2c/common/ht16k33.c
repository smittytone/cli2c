/**
 *
 * I2C driver w. HT16K33
 * Version 1.0.0
 * Copyright © 2022, smittytone
 * Licence: MIT
 *
 */
#include "ht16k33.h"


// Defined in `main.c`
extern I2CDriver i2c;
extern int i2c_address;


// The Ascii character set
const char CHARSET[128][6] = {
    "\x00\x00\x00",              // space - Ascii 32
    "\xfa\x00",                  // !
    "\xc0\x00\xc0\x00",          // "
    "\x24\x7e\x24\x7e\x24\x00",  // #
    "\x24\xd4\x56\x48\x00",      // $
    "\xc6\xc8\x10\x26\xc6\x00",  // %
    "\x6c\x92\x6a\x04\x0a\x00",  // &
    "\xc0\x00",                  // '
    "\x7c\x82\x00",              // (
    "\x82\x7c\x00",              // )
    "\x10\x7c\x38\x7c\x10\x00",  // *
    "\x10\x10\x7c\x10\x10\x00",  // +
    "\x06\x07\x00",              // ,
    "\x10\x10\x10\x10\x00",      // -
    "\x06\x06\x00",              // .
    "\x04\x08\x10\x20\x40\x00",  // /
    "\x7c\x8a\x92\xa2\x7c\x00",  // 0 - Ascii 48
    "\x42\xfe\x02\x00",          // 1
    "\x46\x8a\x92\x92\x62\x00",  // 2
    "\x44\x92\x92\x92\x6c\x00",  // 3
    "\x18\x28\x48\xfe\x08\x00",  // 4
    "\xf4\x92\x92\x92\x8c\x00",  // 5
    "\x3c\x52\x92\x92\x8c\x00",  // 6
    "\x80\x8e\x90\xa0\xc0\x00",  // 7
    "\x6c\x92\x92\x92\x6c\x00",  // 8
    "\x60\x92\x92\x94\x78\x00",  // 9
    "\x36\x36\x00",              // : - Ascii 58
    "\x36\x37\x00",              //
    "\x10\x28\x44\x82\x00",      // <
    "\x24\x24\x24\x24\x24\x00",  // =
    "\x82\x44\x28\x10\x00",      // >
    "\x60\x80\x9a\x90\x60\x00",  // ?
    "\x7c\x82\xba\xaa\x78\x00",  // @
    "\x7e\x90\x90\x90\x7e\x00",  // A - Ascii 65
    "\xfe\x92\x92\x92\x6c\x00",  // B
    "\x7c\x82\x82\x82\x44\x00",  // C
    "\xfe\x82\x82\x82\x7c\x00",  // D
    "\xfe\x92\x92\x92\x82\x00",  // E
    "\xfe\x90\x90\x90\x80\x00",  // F
    "\x7c\x82\x92\x92\x5c\x00",  // G
    "\xfe\x10\x10\x10\xfe\x00",  // H
    "\x82\xfe\x82\x00",          // I
    "\x0c\x02\x02\x02\xfc\x00",  // J
    "\xfe\x10\x28\x44\x82\x00",  // K
    "\xfe\x02\x02\x02\x00",      // L
    "\xfe\x40\x20\x40\xfe\x00",  // M
    "\xfe\x40\x20\x10\xfe\x00",  // N
    "\x7c\x82\x82\x82\x7c\x00",  // O
    "\xfe\x90\x90\x90\x60\x00",  // P
    "\x7c\x82\x92\x8c\x7a\x00",  // Q
    "\xfe\x90\x90\x98\x66\x00",  // R
    "\x64\x92\x92\x92\x4c\x00",  // S
    "\x80\x80\xfe\x80\x80\x00",  // T
    "\xfc\x02\x02\x02\xfc\x00",  // U
    "\xf8\x04\x02\x04\xf8\x00",  // V
    "\xfc\x02\x3c\x02\xfc\x00",  // W
    "\xc6\x28\x10\x28\xc6\x00",  // X
    "\xe0\x10\x0e\x10\xe0\x00",  // Y
    "\x86\x8a\x92\xa2\xc2\x00",  // Z - Ascii 90
    "\xfe\x82\x82\x00",          // [
    "\x40\x20\x10\x08\x04\x00",  // forward slash
    "\x82\x82\xfe\x00",          // ]
    "\x20\x40\x80\x40\x20\x00",  // ^
    "\x02\x02\x02\x02\x02\x00",  // _
    "\xc0\xe0\x00",              // '
    "\x04\x2a\x2a\x1e\x00",      // a - Ascii 97
    "\xfe\x22\x22\x1c\x00",      // b
    "\x1c\x22\x22\x22\x00",      // c
    "\x1c\x22\x22\xfc\x00",      // d
    "\x1c\x2a\x2a\x10\x00",      // e
    "\x10\x7e\x90\x80\x00",      // f
    "\x18\x25\x25\x3e\x00",      // g
    "\xfe\x20\x20\x1e\x00",      // h
    "\xbc\x02\x00",              // i
    "\x02\x01\x21\xbe\x00",      // j
    "\xfe\x08\x14\x22\x00",      // k
    "\xfc\x02\x00",              // l
    "\x3e\x20\x18\x20\x1e\x00",  // m
    "\x3e\x20\x20 \x1e\x00",     // n
    "\x1c\x22\x22\x1c\x00",      // o
    "\x3f\x22\x22\x1c\x00",      // p
    "\x1c\x22\x22\x3f\x00",      // q
    "\x22\x1e\x20\x10\x00",      // r
    "\x12\x2a\x2a\x04\x00",      // s
    "\x20\x7c\x22\x04\x00",      // t
    "\x3c\x02\x02\x3e\x00",      // u
    "\x38\x04\x02\x04\x38\x00",  // v
    "\x3c\x06\x0c\x06\x3c\x00",  // w
    "\x22\x14\x08\x14\x22\x00",  // x
    "\x39\x05\x06\x3c\x00",      // y
    "\x26\x2a\x2a\x32\x00",      // z - Ascii 122
    "\x10\x7c\x82\x82\x00",      //
    "\xee\x00",                  // |
    "\x82\x82\x7c\x10\x00",      //
    "\x40\x80\x40\x80\x00",      // ~
    "\x60\x90\x90\x60\x00"       // Degrees sign - Ascii 127
};

// Display buffer
uint8_t display_buffer[8];
uint8_t display_angle = 0;


/**
 * @brief Power the display on or off
 *
 * @param is_on:    Whether to power up (`true`) the display or
 *                shut it down (`false`).
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
 * @brief Set the diplay's angle of rotation.
 *
 *  @param angle: The angle of rotation as an integer multiple of 90 degrees.
 */
void HT16K33_set_angle(uint8_t angle) {
    if (angle != 0) {
        if (angle > 0 && angle < 4) {
            display_angle = angle;
        }
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
    for (uint8_t i = 0 ; i < 8 ; ++i) {
        display_buffer[i] = 0;
    }
}


/**
 * @brief Write the display buffer out to the LED.
 */
void HT16K33_draw(void) {
    // Set up the buffer holding the data to be
    // transmitted to the LED
    uint8_t tx_buffer[17] = { 0 };

    if (display_angle != 0) {
        HT16K33_rotate(display_angle);
    }

    // Span the 8 bytes of the graphics buffer
    // across the 16 bytes of the LED's buffer
    for (uint8_t i = 0 ; i < 8 ; ++i) {
        uint8_t a = display_buffer[i];
        tx_buffer[i * 2 + 1] = (a >> 1) + ((a << 7) & 0xFF);
    }

    // Display the buffer and flash the LED
    i2c_start(&i2c, i2c_address, 0);
    i2c_write(&i2c, tx_buffer, 17);
    i2c_stop(&i2c);
}


/**
 *  @brief Set or unset a pixel on the display.
 *
 *  @param x:      The pixel's X co-ordinate.
 *  @param y:      The pixel's Y co-ordinate.
 *  @param is_set: Whether to set the pixel (`true`) ior clear it.
 */
void HT16K33_plot(uint8_t x, uint8_t y, bool is_set) {
    // Set or unset the specified pixel
    if (is_set) {
        display_buffer[x] |= (1 << y);
    } else {
        display_buffer[x] &= ~(1 << y);
    }
}


void HT16K33_set_char(uint8_t ascii, bool is_centred) {
    uint8_t delta = 0;
    if (is_centred) {
        delta = (8 - strlen(CHARSET[ascii - 32])) >> 1;
    }

    for (uint8_t i = 0 ; i < 8 ; ++i) {
        if (CHARSET[ascii - 32][i] == 0) break;
        display_buffer[i + delta] = CHARSET[ascii - 32][i];
    }
}


void HT16K33_set_glyph(uint8_t* bytes) {
    for (uint8_t i = 0 ; i < 8 ; ++i) {
        display_buffer[i] = bytes[i];
    }
}


/**
 * @brief Scroll the supplied text horizontally across the 8x8 matrix.
 *
 * @param text:     Pointer to a text string to display.
 * @param delay_ms: The scroll delay in ms.
 */
void HT16K33_print(const char *text, uint32_t delay_ms) {
    // Get the length of the text: the number of columns it encompasses
    uint64_t length = 0;
    for (size_t i = 0 ; i < strlen(text) ; ++i) {
        uint8_t asc_val = text[i] - 32;
        length += (asc_val == 0 ? 2: strlen(CHARSET[asc_val]));
        if (asc_val > 0) length++;
    }

    // Make the output buffer to match the required number of columns
    uint8_t src_buffer[length];
    for (uint64_t i = 0 ; i < length ; ++i) src_buffer[i] = 0x00;

    // Write each character's glyph columns into the output buffer
    uint64_t col = 0;
    for (size_t i = 0 ; i < strlen(text) ; ++i) {
        uint8_t asc_val = text[i] - 32;
        if (asc_val == 0) {
            // It's a space, so just add two blank columns
            col += 2;
        } else {
            // Get the character glyph and write it to the buffer
            uint8_t glyph_len = strlen(CHARSET[asc_val]);

            for (uint64_t j = 0 ; j < glyph_len ; ++j) {
                src_buffer[col] = CHARSET[asc_val][j];
                ++col;
            }

            ++col;
        }
    }

    // Finally, animate the line by repeatedly sending 8 columns
    // of the output buffer to the matrix
    uint64_t cursor = 0;
    while (1) {
        uint64_t a = cursor;
        for (uint8_t i = 0 ; i < 8 ; ++i) {
            display_buffer[i] = src_buffer[a];
            a += 1;
        }

        HT16K33_draw();
        cursor++;
        if (cursor > length - 8) break;
        
        HT16K33_sleep_ms(display_angle == 0 ? delay_ms : (delay_ms * 2 / 3));
    };
}


/**
 *  @brief Rotate the display. Not a public function.
 *
 *  @param angle: The angle of rotation as an integer multiple of 90 degrees.
 */
void HT16K33_rotate(uint8_t angle) {
    uint8_t temp[8] = { 0 };
    uint8_t a = 0;
    uint8_t line_value = 0;

    for (int32_t y = 0 ; y < 8 ; ++y) {
        line_value = display_buffer[y];
        for (int32_t x = 7 ; x > -1 ; x--) {
            a = line_value & (1 << x);
            if (a != 0) {
                if (angle == 1) {
                    temp[7 - x] |= (1 << y);
                } else if (angle == 2) {
                    temp[7 - y] |= (1 << (7 - x));
                } else {
                    temp[x] |= (1 << (7 - y));
                }
            }
        }
    }

    // Swap the matrices
    memcpy(display_buffer, temp, 8);
}


/**
 * @brief Sleep the thread for the specified period in milliseconds.
 *
 * @param ms: The sleep period.
 */
void HT16K33_sleep_ms(int ms) {
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
void HT16K33_write_cmd(uint8_t cmd) {
    // NOTE Already connected at this stage
    i2c_start(&i2c, i2c_address, 0);
    i2c_write(&i2c, &cmd, 1);
    i2c_stop(&i2c);
}
