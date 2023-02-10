/*
 * I2C Host Firmware - Primary serial and command functions
 *
 * @version     1.1.2
 * @author      Tony Smith (@smittytone)
 * @copyright   2023
 * @licence     MIT
 *
 */
#ifndef _MONITOR_HEADER_
#define _MONITOR_HEADER_


/*
 * INCLUDES
 */
#include <signal.h>
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
#include "gpio.h"
#include "i2c.h"
#ifdef DO_UART_DEBUG
#include "segment.h"
#include "debug.h"
#endif


/*
 * CONSTANTS
 */
#define SERIAL_READ_TIMEOUT_US                  10
#define RX_LOOP_DELAY_MS                        5
#define HEARTBEAT_PERIOD_US                     2000000
#define HEARTBEAT_FLASH_US                      50000

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

// FROM 1.1.2
#define UART_LOOP_DELAY_MS                      1
#define RX_BUFFER_LENGTH_B                      128


/*
 * PROTOTYPES
 */
void        rx_loop(void);


#endif  // _MONITOR_HEADER_
