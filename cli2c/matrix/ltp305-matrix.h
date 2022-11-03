/*
 * LTP305 2x5x7 matrix driver
 *
 * Version 1.2.0
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#ifndef _LTP305_MATRIX_HEADER_
#define _LTP305_MATRIX_HEADER_


/*
 * INCLUDES
 */
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>


/*
 * CONSTANTS
 */
#define IS31FL3730_I2C_ADDR             0x61

#define MATRIX_LEFT_ADDR                0x0E
#define MATRIX_RIGHT_ADDR               0x01

#define IS31FL3730_DEFAULT_BRIGHT       0x40
#define IS31FL3730_MAX_BRIGHT           0x7F

#define IS31FL3730_CONFIG_REG           0x00
#define IS31FL3730_UPDATE_COL_REG       0x0C
#define IS31FL3730_LIGHT_EFFECT_REG     0x0D
#define IS31FL3730_PWM_REG              0x19
#define IS31FL3730_RESET_REG            0xFF



/*
 * ENUMS
 */
enum {
    LEFT  = 0,
    RIGHT = 1
};


/*
 *PROTOTYPES
 */
void LTP305_init(I2CDriver *sd, int address);
void LTP305_set_brightness(int new_brightess);
void LTP305_clear_buffers(void);
void LTP305_set_glyph(uint8_t led, uint8_t* glyph);
void LTP305_set_char(uint8_t led, uint8_t ascii);
void LTP305_plot(uint8_t led, uint8_t x, uint8_t y, bool ink);
void LTP305_set_point(uint8_t led);
void LTP305_clear(void);
void LTP305_draw(void);


#endif  // _LTP305_MATRIX_HEADER_
