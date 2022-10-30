/*
 * Bus Host Firmware - SPI functions
 *
 * @version     2.0.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "spi.h"


/*
 * GLOBALS
 */
extern uint8_t SPI_PIN_QUADS_BUS_0[];
extern uint8_t SPI_PIN_QUADS_BUS_1[];


/**
 * @brief Initialise the host's SPI bus.
 *
 * @param sps: The SPI state record.
 */
void init_spi(SPI_State* sps) {

    // Initialise SPI via SDK
    spi_init(sps->bus, sps->baudrate * 1000);

    // Initialise pins
    gpio_set_function(sps->tx_pin, GPIO_FUNC_SPI);
    gpio_set_function(sps->rx_pin, GPIO_FUNC_SPI);
    gpio_set_function(sps->sck_pin,  GPIO_FUNC_SPI);

    // Chip select is active-low, so initialise it high
    gpio_init(sps->cs_pin);
    gpio_set_dir(sps->cs_pin, GPIO_OUT);
    gpio_put(sps->cs_pin, 1);

    // Mark bus as ready for use
    sps->is_ready = true;
}


/**
 * @brief Reset the host's SPI bus.
 *
 * @param sps: The SPI state record.
 */
void reset_spi(SPI_State* sps) {

    spi_deinit(sps->bus);
    sleep_ms(10);
    spi_init(sps->bus, sps->baudrate * 1000);
}


/**
 * @brief Configure the SPI bus: its ID and pins.
 *
 * @param data: The received data. Byte 1 is the bus ID,
 *              byte 2 the RX pin, byte 3 the TX pin,
 *              byte 4 is the CS pin, byte 5 is the SCK
 *              pin, and bytes 6-8 are the baudrate.
 *
 * @retval Whether the config was set successfully (`true`) or not (`false`).
 */
bool configure_spi(SPI_State* sps, uint8_t* data) {

    if (sps->is_ready || !check_spi_pins(&data[1])) {
        return false;
    }

    // Store the values
    uint8_t bus_index = data[1] & 0x01;
    sps->bus = bus_index == 0 ? spi0 : spi1;
    sps->rx_pin   = data[2];
    sps->tx_pin   = data[3];
    sps->cs_pin   = data[4];
    sps->sck_pin  = data[5];
    sps->baudrate = (data[6] << 16) | (data[7] << 8) | data[8];
}


bool check_spi_pins(uint8_t* data) {

    // Get the bus ID
    uint8_t bus_index = data[0] & 0x01;

    // Check the TX pin
    uint8_t* pin_quads = bus_index == 0 ? &SPI_PIN_QUADS_BUS_0[0] : &SPI_PIN_QUADS_BUS_1[0];
    if (!pin_check(pin_quads, data[2], 4)) return false;

    // Check the TX pin
    pin_quads = bus_index == 0 ? &SPI_PIN_QUADS_BUS_0[1] : &SPI_PIN_QUADS_BUS_1[1];
    if (!pin_check(pin_quads, data[3], 4)) return false;

    // Check the CS pin
    pin_quads = bus_index == 0 ? &SPI_PIN_QUADS_BUS_0[2] : &SPI_PIN_QUADS_BUS_1[2];
    if (!pin_check(pin_quads, data[4], 4)) return false;

    // Check the SCK pin
    pin_quads = bus_index == 0 ? &SPI_PIN_QUADS_BUS_0[3] : &SPI_PIN_QUADS_BUS_1[3];
    if (!pin_check(pin_quads, data[5], 4)) return false;

    return true;
}