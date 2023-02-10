/*
 * I2C Host Firmware - I2C functios
 *
 * @version     1.1.2
 * @author      Tony Smith (@smittytone)
 * @copyright   2023
 * @licence     MIT
 *
 */
#include "i2c.h"


/**
 * @brief Initialise the host's I2C bus.
 *
 * @param frequency_khz: The bus speed in kHz.
 */
void init_i2c(I2C_State* itr) {

    // Initialise I2C via SDK
    i2c_init(itr->bus, itr->frequency * 1000);

    // Initialise pins
    // The values of SDA_PIN and SCL_PIN are set
    // in the board's individual CMakeLists.txt file.
    gpio_set_function(itr->sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(itr->scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(itr->sda_pin);
    gpio_pull_up(itr->scl_pin);

    // Mark bus as ready for use
    itr->is_ready = true;
}


/**
 * @brief Reset the host's I2C bus.
 *
 * @param frequency_khz: The bus speed in kHz.
 */
void reset_i2c(I2C_State* itr) {

    i2c_deinit(itr->bus);
    sleep_ms(10);
    i2c_init(itr->bus, itr->frequency * 1000);
}
