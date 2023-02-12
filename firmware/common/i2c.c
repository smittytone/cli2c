/*
 * I2C Host Firmware - I2C functions
 *
 * @version     1.1.3
 * @author      Tony Smith (@smittytone)
 * @copyright   2023
 * @licence     MIT
 *
 */
#include "i2c.h"


/*
 * STATIC PROTOTYPES
 */



/*
 * GLOBALS
 */
// FROM 1.1.3
// Access individual boards' pin arrays
extern uint8_t I2C_PIN_PAIRS_BUS_0[];
extern uint8_t I2C_PIN_PAIRS_BUS_1[];


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


/**
 * @brief Set the frequency of the host's I2C bus.
 *
 * @param its:           The I2C state record.
 * @param frequency_khz: The Frequency in kHz.
 */
void set_i2c_frequency(I2C_State* its, uint32_t frequency_khz) {

    if (frequency_khz != 100 && frequency_khz != 400) return;

    if (its->frequency != frequency_khz) {
        its->frequency = frequency_khz;

        // If the bus is active, reset it
        if (its->is_ready) {
            reset_i2c(its);
            its->is_started = false;
        }
    }
}


/**
 * @brief Scan the host's I2C bus for devices, and send the results.
 */
void send_i2c_scan(I2C_State* itr) {

    uint8_t rx_data;
    int reading;
    char scan_buffer[1024] = {0};
    uint32_t device_count = 0;;

    // Generate a list if devices by their addresses.
    // List in the form "13.71.A0."
    for (uint32_t i = 0 ; i < 0x78 ; ++i) {
        reading = i2c_read_timeout_us(itr->bus, i, &rx_data, 1, false, 1000);
        if (reading > 0) {
            sprintf(scan_buffer + (device_count * 3), "%02X.", i);
            device_count++;
        }
    }

    // Write 'Z' if there are no devices,
    // or send the device list string
    if (strlen(scan_buffer) == 0) {
        sprintf(scan_buffer, "Z\r\n");
    } else {
        sprintf(scan_buffer + (device_count * 3), "\r\n");
    }

    // Send the scan data back
    tx(scan_buffer, strlen(scan_buffer));
}
