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
 * @brief Power on the display.
 */
void LTP305_power_on(void) {
    
    LTP305_write_register(IS31FL3730_CONFIG_REG, 0x18, false);
    LTP305_write_register(IS31FL3730_LIGHT_EFFECT_REG, 0x0E, false);
    LTP305_write_register(IS31FL3730_PWM_REG, brightness, false);
    LTP305_write_register(IS31FL3730_UPDATE_COL_REG, 0x01, true);
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
    LTP305_write_register(IS31FL3730_UPDATE_COL_REG, 0x01, true);
}


/**
 * @brief Zero the display's primary buffers.
 */
void LTP305_clear_buffers(void) {
    
    memset(main_buffer, 0x00, 10);
   // memset(&left_buffer[1], 0x00, 8);
    //memset(&right_buffer[1], 0x00, 8);
}


/**
 * @brief Set a user-defined character on the display.
 *
 * @param row:   The row at which the glyph will be drawn.
 * @param glyph: Pointer to an array of bytes, one per column.
 *               A row's bit 0 is the bottom.
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
    size_t cols = strlen(CHARSET[ascii - 32]);
    if (cols < 5) d = (5 - cols) >> 1;
    if (cols % 2 == 0) d +=1;
    
    switch(led) {
        case LEFT:
            memset(main_buffer, 0x00, 5);
            for (size_t x = 0 ; x < cols ; ++x) {
                main_buffer[x + d] = CHARSET[ascii - 32][x];
            }
        case RIGHT:
            memset(&main_buffer[5], 0x00, 5);
            for (size_t x = 0 ; x < cols ; ++x) {
                main_buffer[5 + x + d] = CHARSET[ascii - 32][x];;
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
    
    if (ink) {
        main_buffer[x] |= (1 << (7 - y));
    } else {
        main_buffer[x] &= ~(1 << (7 - y));
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
        cols += (asc_val == 0 ? 2 : strlen(CHARSET[asc_val]));
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
            size_t glyph_len = strlen(CHARSET[asc_val]);
            for (size_t j = 0 ; j < glyph_len ; ++j) {
                src_buffer[col++] = CHARSET[asc_val][j];
            }

            // Space between lines
            src_buffer[col++] = 0x00;
        }
    }

    // Finally, animate the line by repeatedly sending 10 columns
    // of the output buffer to the matrix
    uint cursor = 0;
    while (1) {
        LTP305_set_glyph(0, &src_buffer[cursor++], 9);
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
                for (size_t y = 0 ; y < 8 ; ++y) {
                    b = (c & (1 << y) >> 1);
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
                for (size_t y = 0 ; y < 8 ; ++y) {
                    b |= ((c & (1 << y)) >> 1);
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
                for (size_t y = 0 ; y < 8 ; ++y) {
                    b |= (((c & (1 << y)) >> y) << (7 - y));
                }
                left_buffer[x + 1] = b;
            } else {
                //       x  98765
                //          43210
                //       00011111 y 0
                //       00011111 y 1
                //       00011111 y 2
                //       00011111 y 3
                //       00011111 y 4
                //       00011111 y 5
                //       10011111 y 6 (bit 8 = decimal point)
                size_t rx = x - 5;
                for (size_t y = 0 ; y < 8 ; ++y) {
                    b = (c & (1 << (7 - y)));
                    if (b > 0) {
                        right_buffer[y + 1] |= (1 << rx);
                    } else {
                        right_buffer[y + 1] &= ~(1 << rx);
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
