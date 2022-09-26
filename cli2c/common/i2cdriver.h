/**
 *
 * I2C driver w. HT16K33
 * Version 1.0.0
 * Copyright © 2019, James Bowman
 * Licence: BSD 3-Clause
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
#define HANDLE int


/*
 * STRUCTURES
 */
typedef struct {
    int             connected;          // Set to 1 when connected
    HANDLE          port;
    char            model[16],
                    serial[9];          // Serial number of USB device
    uint64_t        uptime;             // time since boot (seconds)
    float           voltage_v,          // USB voltage (Volts)
                    current_ma,         // device current (mA)
                    temp_celsius;       // temperature (C)
    unsigned int    mode;               // I2C 'I' or bitbang 'B' mode
    unsigned int    sda;                // SDA state, 0 or 1
    unsigned int    scl;                // SCL state, 0 or 1
    unsigned int    speed;              // I2C line speed (in kHz)
    unsigned int    pullups;            // pullup state (6 bits, 1=enabled)
    unsigned int    ccitt_crc,          // Hardware CCITT CRC
                    e_ccitt_crc;        // Host CCITT CRC, should match
} I2CDriver;


/*
 * PROTOTYPES
 */

int         openSerialPort(const char *portname);
size_t      readFromSerialPort(int fd, uint8_t* b, size_t s);
void        writeToSerialPort(int fd, const uint8_t* b, size_t s);

void        i2c_connect(I2CDriver *sd, const char* portname);
void        i2c_getstatus(I2CDriver *sd);
size_t      i2c_write(I2CDriver *sd, const uint8_t bytes[], size_t nn);
void        i2c_read(I2CDriver *sd, uint8_t bytes[], size_t nn);
bool        i2c_start(I2CDriver *sd, uint8_t address, uint8_t op);
void        i2c_stop(I2CDriver *sd);
void        i2c_monitor(I2CDriver *sd, int enable);
void        i2c_capture(I2CDriver *sd);
int         i2c_commands(I2CDriver *sd, int argc, char *argv[]);

bool        i2c_start(I2CDriver *sd, uint8_t address, uint8_t op);
bool        i2c_reset(I2CDriver *sd);
static int  i2c_ack(I2CDriver *sd);

static void send_command(I2CDriver *sd, char c);
static void print_bad_command_help(char* token);


#endif  // I2CDRIVER_H
