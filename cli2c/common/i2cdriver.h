/*
 * Generic macOS I2C driver
 *
 * Version 0.1.1
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#ifndef _I2CDRIVER_H
#define _I2CDRIVER_H


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
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <memory.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


/*
 * CONSTANTS
 */
#define PREFIX_BYTE_READ                0x80
#define PREFIX_BYTE_WRITE               0xC0


/*
 * STRUCTURES
 */
typedef struct {
    bool            connected;          // Set to true when connected
    int             port;
    unsigned int    speed;              // I2C line speed (in kHz)
    char            model[16];
} I2CDriver;


/*
 * PROTOTYPES
 */

int         openSerialPort(const char *portname);
size_t      readFromSerialPort(int fd, uint8_t* b, size_t s);
void        writeToSerialPort(int fd, const uint8_t* b, size_t s);

void        i2c_connect(I2CDriver *sd, const char* portname);
bool        i2c_init(I2CDriver *sd);
bool        i2c_start(I2CDriver *sd, uint8_t address, uint8_t op);
bool        i2c_reset(I2CDriver *sd);
bool        i2c_stop(I2CDriver *sd);
void        i2c_get_info(I2CDriver *sd, bool do_print);
size_t      i2c_write(I2CDriver *sd, const uint8_t bytes[], size_t nn);
void        i2c_read(I2CDriver *sd, uint8_t bytes[], size_t nn);
int         i2c_commands(I2CDriver *sd, int argc, char *argv[], uint32_t delta);
static bool i2c_ack(I2CDriver *sd);

static void send_command(I2CDriver *sd, char c);
static void print_bad_command_help(char* token);

void        show_commands(void);


#endif  // I2CDRIVER_H
