/*
 * I2C Host Firmware - debugging LED segment driver
 *
 * @version     0.1.1
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#ifndef _HT16K33_SEG_HEADER_
#define _HT16K33_SEG_HEADER_


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// Pico SDK Includes
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"


/*
 * CONSTANTS
 */
// Display I2C address
#define     HT16K33_I2C_ADDR            0x70


/*
 * PROTOTYPES
 */
void        HT16K33_init();
void        HT16K33_write_cmd(uint8_t cmd);
void        HT16K33_draw();
void        HT16K33_clear_buffer();
void        HT16K33_set_number(uint8_t number, uint8_t digit, bool has_dot);
void        HT16K33_set_glyph(uint8_t glyph, uint8_t digit, bool has_dot);
void        HT16K33_show_value(int16_t value, bool has_decimal);
uint32_t    bcd(uint32_t value);


#endif  // _HT16K33_SEG_HEADER_