/*
 * I2C driver for an HT16K33 4-digit, 7-segment display
 *
 * Version 0.1.5
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#ifndef _MAIN_H_
#define _MAIN_H_


/*
 * INCLUDES
 */
#include "i2cdriver.h"
#include "ht16k33-segment.h"


/*
 * PROTOTYPES
 */
int     segment_commands(I2CDriver* sd, int argc, char* argv[], int delta);
void    show_help(void);
void    print_warning(char* format_string, ...);
void    print_error(char* format_string, ...);
void    print_output(bool is_err, char* format_string, va_list args);
void    ctrl_c_handler(int dummy);


#endif      // _MAIN_H_
