/*
 * Bus Host Firmware - Primary serial and command functions
 *
 * @version     2.0.0
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
#include <string.h>
// Pico SDK Includes
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "pico/unique_id.h"
// App Includes
#include "led.h"
#include "i2c.h"
#include "spi.h"
#include "gpio.h"
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

#define WRITE_LENGTH_BASE                       0xC0
#define READ_LENGTH_BASE                        0x80

#define HW_MODEL_NAME_SIZE_MAX                  24

#ifndef DEBUG_SEG_ADDR
// Just in case the user comments out the CMakeLists.txt define
#define DEBUG_SEG_ADDR                          0x70
#endif

#define ACK                                     0x0F
#define ERR                                     0xF0

// FROM 2.0.0
#define MODE_NONE                               0
#define MODE_I2C                                1
#define MODE_SPI                                2
#define MODE_UART                               3
#define MODE_ONE_WIRE                           4

#define COLOUR_MODE_I2C                         0x001010
#define COLOUR_MODE_SPI                         0x100010
#define COLOUR_MODE_UART                        0x001000
#define COLOUR_MODE_ONE_WIRE                    0x102000


/*
 * PROTOTYPES
 */
void    rx_loop(void);
void    tx(uint8_t* buffer, uint32_t byte_count);
bool    pin_check(uint8_t* pins, uint8_t pin, uint8_t delta);


#endif  // _MONITOR_HEADER_
