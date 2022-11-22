/*
 * Generic macOS I2C driver
 *
 * Version 1.1.2
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

#define HOST_INFO_BUFFER_MAX_B          129
#define CONNECTED_DEVICES_MAX_B         120
#define SCAN_BUFFER_MAX_B               512

#define ACK                             0x0F
#define ERR                             0xF0


/*
 * STRUCTURES
 */
typedef struct {
    bool            connected;          // Set to true when connected
    int             port;               // OS file descriptor for host
    unsigned int    speed;              // I2C line speed (in kHz)
} I2CDriver;


/*
 * PROTOTYPES
 */
// Serial Port Control Functions
void            flush_and_close_port(int fd);

// I2C Driver Functions
void            i2c_connect(I2CDriver *sd, const char* portname);
bool            i2c_init(I2CDriver *sd);

bool            i2c_start(I2CDriver *sd, uint8_t address, uint8_t op);
bool            i2c_stop(I2CDriver *sd);

size_t          i2c_write(I2CDriver *sd, const uint8_t bytes[], size_t nn);
void            i2c_read(I2CDriver *sd, uint8_t bytes[], size_t nn);

// Command Parsing and Processing
int             process_commands(I2CDriver *sd, int argc, char *argv[], uint32_t delta);


#endif  // I2C_DRIVER_H
