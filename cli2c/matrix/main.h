/**
 *
 * I2C driver for HT16K33
 * Version 0.1.0
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#ifndef _MAIN_H_
#define _MAIN_H_


/*
 * INCLUDES
 */
#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <assert.h>
#include <memory.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
// App includes
#include "i2cdriver.h"
#include "ht16k33.h"


/*
 * CONSTANTS
 */
#define VERBOSE 1


/*
 * PROTOTYPES
 */
int     matrix_commands(int argc, char* argv[], int delta);
void    show_help(void);
void    print_error(char* format_string, ...);
void    print_output(bool is_err, char* format_string, va_list args);


#endif      // _MAIN_H_
