/*
 * Generic macOS I2C driver - Utility Functions
 *
 * Version 1.1.3
 * Copyright © 2023, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#ifndef _UTILS_H
#define _UTILS_H


/*
 * INCLUDES
 */
#include <signal.h>
#include "i2cdriver.h"


/*
 * PROTOTYPES
 */
void    print_warning(char* format_string, ...);
void    print_error(char* format_string, ...);
void    print_output(bool is_err, char* format_string, va_list args);
void    ctrl_c_handler(int dummy);
void    lower(char* s);


#endif  // _UTILS_H
