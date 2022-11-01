/*
 * RP2040 Bus Host Firmware - SPI functions
 *
 * @version     1.2.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#ifndef _SPI_HEADER_
#define _SPI_HEADER_


/*
 * INCLUDES
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Pico SDK Includes
#include "pico/stdlib.h"
#include "hardware/spi.h"
// App Includes
#include "serial.h"


/*
 * STRUCTURES
 */
typedef struct {
    bool        is_ready;
    bool        is_started;
    bool        is_read_op;
    uint32_t    read_byte_count;
    uint32_t    write_byte_count;
    spi_inst_t* bus;
    uint8_t     rx_pin;
    uint8_t     tx_pin;
    uint8_t     cs_pin;
    uint8_t     sck_pin;
    uint        baudrate;
} SPI_State;


/*
 * PROTOTYPES
 */
void    init_spi(SPI_State* spr);
void    reset_spi(SPI_State* spr);
bool    configure_spi(SPI_State* sps, uint8_t* data);
bool    check_spi_pins(uint8_t* data);
void    get_spi_state(SPI_State* sps, char* output);
bool    spi_is_pin_in_use(SPI_State* sps, uint8_t pin);


#endif  // _SPI_HEADER_