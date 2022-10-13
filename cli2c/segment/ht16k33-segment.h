/*
 * HT16K33 4-digit, 7-segment driver
 *
 * Version 0.1.5
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#ifndef _HT16K33_SEGMENT_HEADER_
#define _HT16K33_SEGMENT_HEADER_


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
#define     HT16K33_I2C_ADDR                0x70
#define     HT16K33_CMD_POWER_ON            0x21
#define     HT16K33_CMD_POWER_OFF           0x20
#define     HT16K33_CMD_DISPLAY_ON          0x81
#define     HT16K33_CMD_DISPLAY_OFF         0x80
#define     HT16K33_CMD_BRIGHTNESS          0xE0


/*
 * PROTOTYPES
 */
void            HT16K33_init(I2CDriver *sd, int address);
void            HT16K33_power(bool is_on);
void            HT16K33_flip(void);
void            HT16K33_draw(void);
void            HT16K33_clear_buffer(void);
void            HT16K33_set_brightness(uint8_t brightness);
void            HT16K33_set_number(uint8_t number, uint8_t digit, bool has_dot);
void            HT16K33_set_glyph(uint8_t glyph, uint8_t digit, bool has_dot);
void            HT16K33_show_value(int value, bool decimal);
void            HT16K33_set_point(uint8_t digit);

static uint32_t bcd(uint32_t base);
static void     HT16K33_sleep_ms(int ms);
static void     HT16K33_write_cmd(uint8_t cmd);


#endif  // _HT16K33_SEGMENT_HEADER_
