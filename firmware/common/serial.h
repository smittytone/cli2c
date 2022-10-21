/*
 * I2C Host Firmware - Primary serial and command functions
 *
 * @version     1.1.0
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
#define HEARTBEAT_PERIOD_US                     2000000
#define HEARTBEAT_FLASH_US                      100000

#ifndef DEFAULT_I2C_BUS
#define DEFAULT_I2C_BUS                         1
#endif

#define WRITE_LENGTH_BASE                       0xC0
#define READ_LENGTH_BASE                        0x80

#define HW_MODEL_NAME_SIZE_MAX                  24

#ifndef DEBUG_SEG_ADDR
// Just in case the user comments out the CMakeLists.txt define
#define DEBUG_SEG_ADDR                          0x70
#endif

#define GPIO_PIN_DIRN_BIT                       1
#define GPIO_PIN_STATE_BIT                      0

#define ACK                                     0x0F
#define ERR                                     0xF0


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

typedef struct {
    uint8_t     state_map[32];
} GPIO_State;

/*
 * PROTOTYPES
 */
void        rx_loop(void);

void        init_i2c(I2C_State* itr);
void        reset_i2c(I2C_State* itr);

void        send_ack(void);
void        send_err(void);
void        send_scan(I2C_State* itr);
void        send_status(I2C_State* itr);
void        send_commands(void);

uint32_t    rx(uint8_t *buffer);
void        tx(uint8_t* buffer, uint32_t byte_count);

// FROM 1.1.0
bool        check_pins(uint8_t bus, uint8_t sda, uint8_t scl);
bool        pin_check(uint8_t* pins, uint8_t pin);


#endif  // _MONITOR_HEADER_
