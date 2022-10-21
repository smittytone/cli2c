/*
 * I2C driver for an HT16K33 8x8 Matrix Display
 *
 * Version 1.1.0
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
#include "utils.h"
#include "ht16k33-matrix.h"


/*
 * PROTOTYPES
 */
int     matrix_commands(I2CDriver* sd, int argc, char* argv[], int delta);
void    show_help(void);


#endif      // _MAIN_H_
