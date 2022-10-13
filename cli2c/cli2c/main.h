/*
 * Generic macOS I2C driver
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


/*
 * PROTOTYPES
 */
void    print_warning(char* format_string, ...);
void    print_error(char* format_string, ...);
void    print_output(bool is_err, char* format_string, va_list args);
void    show_help(void);
void    ctrl_c_handler(int dummy);


#endif      // _MAIN_H_
