/*
 * I2C Host
 *
 * @version     0.1.5
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#ifndef _MONITOR_HEADER_
#define _MONITOR_HEADER_


/*
 * INCLUDES
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// Pico SDK Includes
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "pico/unique_id.h"
// App Includes
#include "led.h"
#ifdef DO_DEBUG
#include "segment.h"
#endif


/*
 * CONSTANTS
 */
#define SERIAL_READ_TIMEOUT_US                  10
#define RX_LOOP_DELAY_MS                        50

#define DEFAULT_I2C_PORT                        i2c1

#define WRITE_LENGTH_BASE                       0xC0
#define READ_LENGTH_BASE                        0x80

#define HW_MODEL_NAME_SIZE_MAX                  16

#ifndef DEBUG_SEG_ADDR
// Just in case the user comments out the CMakeLists.txt define
#define DEBUG_SEG_ADDR                          0x70
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
} I2C_Trans;


/*
 * PROTOTYPES
 */
void        rx_loop(void);

void        init_i2c(I2C_Trans* itr);
void        reset_i2c(I2C_Trans* itr);

void        send_ack(void);
void        send_err(void);
void        send_scan(I2C_Trans* itr);
void        send_status(I2C_Trans* itr);
void        send_commands(void);

uint32_t    rx(uint8_t *buffer);
void        tx(uint8_t* buffer, uint32_t byte_count);

#endif  // _MONITOR_HEADER_
