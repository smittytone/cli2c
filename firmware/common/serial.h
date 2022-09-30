/*
 * I2C Host
 *
 * @version     0.1.1
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
// App Includes
#include "led.h"
#ifdef DO_DEBUG
#include "segment.h"
#endif


/*
 * CONSTANTS
 */
#define SERIAL_READ_TIMEOUT_US              10          // 10us

#define I2C_PORT                              i2c1
#define I2C_FREQUENCY                           400000

#define WRITE_LENGTH_BASE                       0xC0
#define READ_LENGTH_BASE                        0x80


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


/*
 * PROTOTYPES
 */
void        rx_loop(void);

void        init_i2c(int frequency_khz);
int         write_i2c(uint8_t address, uint8_t* data, uint32_t count, bool do_stop);
int         read_i2c(uint8_t address, uint8_t* data, uint32_t count, bool do_stop);
void        reset_i2c(int frequency_khz);

void        send_ack(void);
void        send_err(void);
void        send_scan(void);
void        send_status(I2C_Trans* t);
void        send_commands(void);

uint32_t    rx(uint8_t *buffer);
void        tx(uint8_t* buffer, uint32_t byte_count);

#endif  // _MONITOR_HEADER_
