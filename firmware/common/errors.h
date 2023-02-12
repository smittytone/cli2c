/*
 * RP2040 Bus Host Firmware - Errors list
 *
 * @version     1.1.3
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#ifndef _ERRORS_HEADER_
#define _ERRORS_HEADER_


enum HOST_ERRORS {
    GEN_NO_ERROR                = 0,
    GEN_TOO_FEW_KEY_BYTES       = 1,
    GEN_UNKNOWN_MODE            = 2,
    GEN_UNKNOWN_COMMAND         = 3,
    GEN_LED_NOT_ENABLED         = 4,
    GEN_CANT_CONFIG_BUS         = 10,
    GEN_CANT_GET_BUS_INFO       = 11,

    I2C_NOT_READY               = 20,
    I2C_NOT_STARTED             = 21,
    I2C_COULD_NOT_WRITE         = 22,
    I2C_COULD_NOT_READ          = 23,
    I2C_ALREADY_STOPPED         = 24,

    SPI_NOT_STARTED             = 40,
    SPI_COULD_NOT_WRITE         = 41,
    SPI_COULD_NOT_READ          = 42,
    SPI_UNAVAILABLE_ON_BOARD    = 43,

    GPIO_CANT_SET_PIN           = 60,
};


#endif  // _ERRORS_HEADER_
