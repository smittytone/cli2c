/*
 * Generic macOS I2C driver
 *
 * Version 1.2.0
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#ifndef _I2C_DRIVER_H
#define _I2C_DRIVER_H


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
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <memory.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#ifndef BUILD_FOR_LINUX
#include <IOKit/serial/ioss.h>
#endif

#define __STDC_FORMAT_MACROS



/*
 * CONSTANTS
 */
#define PREFIX_BYTE_READ                0x80
#define PREFIX_BYTE_WRITE               0xC0

#define EXIT_OK                         0
#define EXIT_ERR                        1

#define CONNECTED_DEVICES_MAX_B         120

#define ACK                             0x0F
#define ERR                             0xF0

// FROM 1.2.0
#define MODE_NONE                       0
#define MODE_I2C                        1
#define MODE_SPI                        2
#define MODE_UART                       3
#define MODE_ONE_WIRE                   4

#define HOST_RX_OK_MESSAGE_B            4
#define HOST_RX_ACK_OR_ERR_B            1
#define HOST_RX_HOST_INFO_B             64
#define HOST_RX_SCAN_RESULTS_B          512
#define HOST_RX_ERR_CODE_B              5

#define INTRA_COMMAND_PAUSE_MS          0.010

#define RP2040_TOP_GPIO_PIN             29


/*
 * STRUCTURES
 */
typedef struct {
    bool            connected;          // Set to true when connected
    int             port;               // OS file descriptor for host
    unsigned int    speed;              // I2C line speed (in kHz)
    // FROM 1.2.0
    bool            started;            // Transaction in process
    unsigned int    address;            // Transaction's I2C address
    char*           portname;           // Port's path, eg. /dev/cu.usbmodem101
} I2CDriver;


/*
 * PROTOTYPES
 */
// Serial Port Control Functions
void            flush_and_close_port(int fd);

// I2C Driver Functions
void            i2c_connect(I2CDriver *sd);
bool            i2c_init(I2CDriver *sd);

bool            i2c_start(I2CDriver *sd, uint8_t address);
bool            i2c_stop(I2CDriver *sd);

size_t          i2c_write(I2CDriver *sd, const uint8_t bytes[], size_t nn);
void            i2c_read(I2CDriver *sd, uint8_t bytes[], size_t nn);

// User-feedback Functions
void            show_commands(void);

// Command Parsing and Processing
int             process_commands(I2CDriver *sd, int argc, char *argv[], int delta);


#endif  // I2C_DRIVER_H
