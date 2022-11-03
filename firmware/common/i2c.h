/*
 * RP2040 Bus Host Firmware - I2C functions
 *
 * @version     1.2.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#ifndef _I2C_HEADER_
#define _I2C_HEADER_


/*
 * INCLUDES
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Pico SDK Includes
#include "pico/stdlib.h"
#include "hardware/i2c.h"
// App Includes
#include "serial.h"


/*
 * STRUCTURES
 */
typedef struct {
    bool        is_ready;
    bool        is_started;
    //bool        is_read_op;
    uint32_t    read_byte_count;
    uint32_t    write_byte_count;
    i2c_inst_t* bus;
    uint8_t     address;
    uint8_t     sda_pin;
    uint8_t     scl_pin;
    uint32_t    frequency;
} I2C_State;


/*
 * PROTOTYPES
 */
void    init_i2c(I2C_State* its);
void    reset_i2c(I2C_State* its);
void    set_i2c_frequency(I2C_State* its, uint32_t frequency_khz);
bool    configure_i2c(I2C_State* its, uint8_t* data);

void    send_scan(I2C_State* its);
bool    check_i2c_pins(uint8_t* data);

bool    start_i2c(I2C_State* its, uint8_t* data);
bool    stop_i2c(I2C_State* its);

void    get_i2c_state(I2C_State* its, char* output);
bool    i2c_is_pin_in_use(I2C_State* its, uint8_t pin);


#endif  // _I2C_HEADER_