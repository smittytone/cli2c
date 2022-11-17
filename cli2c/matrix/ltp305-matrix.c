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
static void     LTP305_sleep_ms(int ms);
static uint8_t  LTP305_swap(uint8_t byte);


/*
 * GLOBALS
 */
static uint8_t left_buffer[9]  = { MATRIX_LEFT_ADDR,  0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t right_buffer[9] = { MATRIX_RIGHT_ADDR, 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t main_buffer[10] = {0};

// The I2C bus
static I2CDriver* host_i2c;
static int i2c_address = IS31FL3730_I2C_ADDR;
static int brightness  = IS31FL3730_DEFAULT_BRIGHT;
static bool is_flipped = false;
extern const char LTPCHARSET[128][6];

// The Ascii character set
const char LTPCHARSET[128][6] = {
    "\x00",                      // space - Ascii 32
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
    "\x10\x2A\x2A\x3C\x00",      // g
    "\xfe\x20\x20\x1e\x00",      // h
    "\x5E\x00",                  // i
    "\x04\x02\x12\x5C\x00",      // j
    "\xfe\x08\x14\x22\x00",      // k
    "\xfc\x02\x00",              // l
    "\x3e\x20\x18\x20\x1e\x00",  // m
    "\x3e\x20\x20 \x1e\x00",     // n
    "\x1c\x22\x22\x1c\x00",      // o
    "\x3E\x24\x24\x18\x00",      // p
    "\x18\x24\x24\x3E\x00",      // q
    "\x22\x1e\x20\x10\x00",      // r
    "\x12\x2a\x2a\x04\x00",      // s
    "\x20\x7c\x22\x04\x00",      // t
    "\x3c\x02\x02\x3e\x00",      // u
    "\x38\x04\x02\x04\x38\x00",  // v
    "\x3c\x06\x0c\x06\x3c\x00",  // w
    "\x22\x14\x08\x14\x22\x00",  // x
    "\x32\x0A\x0C\x38\x00",      // y
    "\x26\x2a\x2a\x32\x00",      // z - Ascii 122
    "\x10\x7c\x82\x82\x00",      //
    "\xee\x00",                  // |
    "\x82\x82\x7c\x10\x00",      //
    "\x40\x80\x40\x80\x00",      // ~
    "\x60\x90\x90\x60\x00"       // Degrees sign - Ascii 127
};


/**
 * @brief Set up the data the driver needs.
 *
 * @param sd:       Pointer to the main I2C driver data structure.
 * @param address:  A non-standard HT16K33 address, or -1.
 */
void LTP305_init(I2CDriver* sd, int address) {

    if (address != -1) i2c_address = address;
    host_i2c = sd;
}


/**
 * @brief Power on the display.
 */
void LTP305_power_on(void) {
    
    LTP305_write_register(IS31FL3730_CONFIG_REG, 0x18, false);
    LTP305_write_register(IS31FL3730_LIGHT_EFFECT_REG, 0x0E, false);
    LTP305_write_register(IS31FL3730_PWM_REG, brightness, false);
    LTP305_write_register(IS31FL3730_UPDATE_COL_REG, 0x00, true);
}


/**
 * @brief Set the .
 *
 * @param text:     Pointer to a text string to display.
 * @param delay_ms: The scroll delay in ms.
 */
void LTP305_flip(bool do_flip) {
    
    is_flipped = do_flip;
}


/**
 * @brief Set the display brightness.
 *
 * @param brightness: The display brightness (1-127).
 */
void LTP305_set_brightness(int new_brightess) {
    
    brightness = new_brightess;
    LTP305_write_register(IS31FL3730_PWM_REG, brightness, false);
    LTP305_write_register(IS31FL3730_UPDATE_COL_REG, 0x00, true);
}


/**
 * @brief Zero the display's primary buffers.
 */
void LTP305_clear_buffers(void) {
    
    memset(main_buffer, 0x00, 10);
}


/**
 * @brief Set a user-defined character on the display.
 *
 * @param row:   The row at which the glyph will be drawn.
 * @param glyph: Pointer to an array of bytes, one per column.
 *               A row's bit 0 is the top.
 * @param width: The number of rows in the glyph (1-10).
 */
void LTP305_set_glyph(size_t row, uint8_t* glyph, size_t width) {

    assert(width < 11);
    assert(row < 10);
    
    size_t lwidth = width;
    if (width + row > 10) lwidth = 10 - row;
    
    for (size_t x = 0 ; x < lwidth ; ++x) {
        uint8_t c = glyph[x];
        main_buffer[x] = c;
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
    size_t cols = strlen(LTPCHARSET[ascii - 32]);
    if (cols < 5) d = (5 - cols) >> 1;
    if (cols % 2 == 0) d +=1;
    
    switch(led) {
        case LEFT:
            memset(main_buffer, 0x00, 5);
            for (size_t x = 0 ; x < cols ; ++x) {
                main_buffer[x + d] = LTP305_swap(LTPCHARSET[ascii - 32][x] >> 1);
            }
        case RIGHT:
            memset(&main_buffer[5], 0x00, 5);
            for (size_t x = 0 ; x < cols ; ++x) {
                main_buffer[5 + x + d] = LTP305_swap(LTPCHARSET[ascii - 32][x] >> 1);
            }
    }
}


/**
 * @brief Plot a point on the display. The two LEDs are treated
 *        as a single 10 x 7 array. Origin is top left
 *
 * @param x:   The X co-ordinate.
 * @param y:   The Y co-orindate.
 * @param ink: Whether to set (`true`) or clear (`false`) the pixel.
 */
void LTP305_plot(uint8_t x, uint8_t y, bool ink) {

    assert (x < 10);
    assert (y < 7);
    
    if (ink) {
        main_buffer[x] |= (1 << y);
    } else {
        main_buffer[x] &= ~(1 << y);
    }
}


/**
 * @brief Scroll the supplied text horizontally across the 8x8 matrix.
 *
 * @param text:     Pointer to a text string to display.
 * @param delay_ms: The scroll delay in ms.
 */
void LTP305_print(const char *text, uint32_t delay_ms) {

    // Get the length of the text: the number of columns it encompasses
    int cols = 0;
    for (size_t i = 0 ; i < strlen(text) ; ++i) {
        uint8_t asc_val = text[i] - 32;
        cols += (asc_val == 0 ? 2 : strlen(LTPCHARSET[asc_val]));
        if (asc_val > 0) cols++;
    }

    // Make the output buffer to match the required number of columns
    uint8_t src_buffer[cols];

    // Write each character's glyph columns into the output buffer
    int col = 0;
    for (size_t i = 0 ; i < strlen(text) ; ++i) {
        uint8_t asc_val = text[i] - 32;
        if (asc_val == 0) {
            // It's a space, so just add two blank columns
            src_buffer[col++] = 0x00;
            src_buffer[col++] = 0x00;
        } else {
            // Get the character glyph and write it to the buffer
            size_t glyph_len = strlen(LTPCHARSET[asc_val]);
            for (size_t j = 0 ; j < glyph_len ; ++j) {
                src_buffer[col++] = LTP305_swap(LTPCHARSET[asc_val][j] >> 1);
            }

            // Space between lines
            src_buffer[col++] = 0x00;
        }
    }

    // Finally, animate the line by repeatedly sending 10 columns
    // of the output buffer to the matrix
    uint cursor = 0;
    while (1) {
        LTP305_set_glyph(0, &src_buffer[cursor++], 10);
        if (cursor > cols - 10) break;
        LTP305_draw();
        LTP305_sleep_ms(delay_ms);
    };
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
void LTP305_clear(void) {

    LTP305_clear_buffers();
    LTP305_draw();
}


/**
 * @brief Update the display.
 */
void LTP305_draw(void) {
    
    if (is_flipped) {
        for (size_t x = 0 ; x < 10 ; ++x) {
            uint8_t c = main_buffer[x];
            uint8_t b = 0;
            size_t dx = 9 - x;

            if (dx > 4) {
                //      dx  98765
                //   inset  43210
                //       00011111 y 0
                //       00011111 y 1
                //       00011111 y 2
                //       00011111 y 3
                //       00011111 y 4
                //       00011111 y 5
                //       10011111 y 6 (bit 8 = decimal point)
                size_t inset = dx - 5;
                for (size_t y = 0 ; y < 7 ; ++y) {
                    b = c & (1 << (6 - y));
                    if (b > 0) {
                        right_buffer[y + 1] |= (1 << inset);
                    } else {
                        right_buffer[y + 1] &= ~(1 << inset);
                    }
                }
            } else {
                //      y 6543210
                //       01111111 x 9
                //       01111111 x 8
                //       01111111 x 7
                //       01111111 x 6
                //       01111111 x 5
                b = 0;
                for (size_t y = 0 ; y < 7 ; ++y) {
                    b |= ((c & (1 << y)) >> y) << (6 - y);
                }
                left_buffer[dx + 1] = b;
            }
        }
    } else {
        // Not flipped: (0,0) at top left
        for (size_t x = 0 ; x < 10 ; ++x) {
            uint8_t c = main_buffer[x];
            uint8_t b = 0;
            
            if (x < 5) {
                //      y 6543210
                //       01111111 x 0
                //       01111111 x 1
                //       01111111 x 2
                //       01111111 x 3
                //       01111111 x 4
                b = 0;
                for (size_t y = 0 ; y < 7 ; ++y) {
                    b |= c & (1 << y);
                }
                left_buffer[x + 1] = b;
            } else {
                //       x  98765
                //   inset  43210
                //       00011111 y 6
                //       00011111 y 5
                //       00011111 y 4
                //       00011111 y 3
                //       00011111 y 2
                //       00011111 y 1
                //       10011111 y 0 (bit 8 = decimal point)
                size_t inset = x - 5;
                for (size_t y = 0 ; y < 7 ; ++y) {
                    b = ((c & (1 << y)) >> y) << (6 - y);
                    if (b > 0) {
                        right_buffer[y + 1] |= (1 << inset);
                    } else {
                        right_buffer[y + 1] &= ~(1 << inset);
                    }
                }
            }
        }
    }
    
    LTP305_write_buffers();
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
static void LTP305_write_buffers(void) {
    
    i2c_start(host_i2c, i2c_address);
    i2c_write(host_i2c, left_buffer, 9);
    i2c_write(host_i2c, right_buffer, 9);
}


/**
 * @brief Sleep the thread for the specified period in milliseconds.
 *
 * @param ms: The sleep period.
 */
static void LTP305_sleep_ms(int ms) {
    
    long delta = 0;
    struct timespec now, then;
    clock_gettime(CLOCK_MONOTONIC_RAW, &then);
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    while (now.tv_nsec - then.tv_nsec < ms * 1000000 - delta) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        if (now.tv_nsec < then.tv_nsec) {
            // Roll over
            delta = LONG_MAX - then.tv_nsec;
            then.tv_nsec = 0;
        }
    }
}


static uint8_t  LTP305_swap(uint8_t b) {

    uint8_t o = 0;
    for (size_t i = 0 ; i < 7 ; ++i) {
        o |= ((b & (1 << i)) >> i) << (6 - i);
    }

    return o;
}
