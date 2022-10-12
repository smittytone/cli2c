/*
 * I2C Host
 *
 * @version     0.1.3
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

#define I2C_PORT                                i2c1
#define I2C_FREQUENCY                           400000

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
    uint32_t    frequency;
    uint32_t    read_byte_count;
    uint32_t    write_byte_count;
} I2C_Trans;

typedef struct {
    uint8_t     bus_count;
    uint8_t     bus_0_pair_count;
    uint8_t     bus_1_pair_count;
    uint8_t     bus_0_pins[20];
    uint8_t     bus_1_pins[20];
} I2C_PINS;


/*
 * PROTOTYPES
 */
void        rx_loop(void);

void        init_i2c(int frequency_khz);
void        reset_i2c(int frequency_khz);

void        send_ack(void);
void        send_err(void);
void        send_scan(void);
void        send_status(I2C_Trans* t);
void        send_commands(void);
void        send_list(void);

uint32_t    rx(uint8_t *buffer);
void        tx(uint8_t* buffer, uint32_t byte_count);

#endif  // _MONITOR_HEADER_
