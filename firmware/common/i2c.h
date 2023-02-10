/*
 * I2C Host Firmware - I2C functios
 *
 * @version     1.1.2
 * @author      Tony Smith (@smittytone)
 * @copyright   2023
 * @licence     MIT
 *
 */
#ifndef _HEADER_I2C_
#define _HEADER_I2C_


/*
 * INCLUDES
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
// Pico SDK Includes
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"


/*
 * CONSTANTS
 */
#ifndef DEFAULT_I2C_BUS
#define DEFAULT_I2C_BUS                         1
#endif


/*
 * STRUCTURES
 */
typedef struct {
    bool        is_ready;
    bool        is_started;
    bool        is_read_op;
    uint8_t     address;
    uint8_t     sda_pin;
    uint8_t     scl_pin;
    uint32_t    frequency;
    uint32_t    read_byte_count;
    uint32_t    write_byte_count;
    i2c_inst_t* bus;
} I2C_State;


/*
 * PROTOTYPES
 */
void    init_i2c(I2C_State* itr);
void    reset_i2c(I2C_State* itr);


#endif  // _HEADER_LED_
