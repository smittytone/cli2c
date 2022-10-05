/*
 * I2C driver for an HT16K33 4-digit, 7-segment display
 *
 * Version 0.1.2
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
#include "ht16k33-segment.h"


/*
 * PROTOTYPES
 */
int     segment_commands(I2CDriver* sd, int argc, char* argv[], int delta);
void    show_help(void);
void    print_error(char* format_string, ...);
void    print_output(bool is_err, char* format_string, va_list args);
void    ctrl_c_handler(int dummy);


#endif      // _MAIN_H_