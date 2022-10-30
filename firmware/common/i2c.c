/*
 * Bus Host Firmware - I2C functions
 *
 * @version     2.0.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "i2c.h"


/*
 * GLOBALS
 */
// FROM 1.1.0
// Access individual boards' pin arrays
extern uint8_t I2C_PIN_PAIRS_BUS_0[];
extern uint8_t I2C_PIN_PAIRS_BUS_1[];


/**
 * @brief Initialise the host's I2C bus.
 *
 * @param its: The I2C state record.
 */
void init_i2c(I2C_State* its) {

    // Initialise I2C via SDK
    i2c_init(its->bus, its->frequency * 1000);

    // Initialise pins
    // The values of SDA_PIN and SCL_PIN are set
    // in the board's individual CMakeLists.txt file.
    gpio_set_function(its->sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(its->scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(its->sda_pin);
    gpio_pull_up(its->scl_pin);

    // Mark bus as ready for use
    its->is_ready = true;
}


/**
 * @brief Reset the host's I2C bus.
 *
 * @param its: The I2C state record.
 */
void reset_i2c(I2C_State* its) {

    i2c_deinit(its->bus);
    sleep_ms(10);
    i2c_init(its->bus, its->frequency * 1000);
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
 * @brief Configure the I2C bus: its ID and pins.
 *
 * @param data: The received data. Byte 1 is the bus ID,
 *              byte 2 the SDA pin, byte 3 the SCL pin
 *
 * @retval Whether the config was set successfully (`true`) or not (`false`).
 */
bool configure_i2c(I2C_State* its, uint8_t* data) {

    // Make sure we have valid data
    if (its->is_ready || !check_i2c_pins(&data[1])) {
        return false;
    }

    // Store the values
    uint8_t bus_index = data[1] & 0x01;
    its->bus = bus_index == 0 ? i2c0 : i2c1;
    its->sda_pin = data[2];
    its->scl_pin = data[3];
    return true;
}


/**
 * @brief Scan the host's I2C bus for devices, and send the results.
 *
 * @param its: The I2C state record.
 */
void send_scan(I2C_State* its) {

    uint8_t rx_data;
    int reading;
    char scan_buffer[1024] = {0};
    uint32_t device_count = 0;;

    // Generate a list if devices by their addresses.
    // List in the form "13.71.A0."
    for (uint32_t i = 0 ; i < 0x78 ; ++i) {
        reading = i2c_read_blocking(its->bus, i, &rx_data, 1, false);
        if (reading >= 0) {
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


/**
 * @brief Check that supplied SDA and SCL pins are valid for the
 *        board we're using.
 *
 * @param data: The received data. Byte 0 is the bus ID,
 *              byte 1 the SDA pin, byte 2 the SCL pin
 *
 * @retval Whether the pins are good (`true`) or not (`false`).
 */
bool check_i2c_pins(uint8_t* data) {

    // Get the bus ID
    uint8_t bus_index = data[0] & 0x01;

    // Same pin? Bail
    if (data[1] == data[2]) return false;

    // Check the SDA pin
    uint8_t* pin_pairs = bus_index == 0 ? &I2C_PIN_PAIRS_BUS_0[0] : &I2C_PIN_PAIRS_BUS_1[0];
    if (!pin_check(pin_pairs, data[1], 2)) return false;

    // Check the SCL pin
    pin_pairs = bus_index == 0 ? &I2C_PIN_PAIRS_BUS_0[1] : &I2C_PIN_PAIRS_BUS_1[1];
    if (!pin_check(pin_pairs, data[2], 2)) return false;
    return true;
}


/**
 * @brief Start an I2C transaction.
 *
 * @param its:  The I2C state record.
 * @param data: The received command data.
 *
 * @retval Whether the op succeeded (`true`) or failed (`false`).
 */
bool start_i2c(I2C_State* its, uint8_t* data) {

    if (its->is_ready && !its->is_started) {
        // Received data is in the form ['s', (address << 1) | op];
        its->address = (data[1] & 0xFE) >> 1;
        its->is_read_op = ((data[1] & 0x01) == 1);
        its->is_started = true;

        // Acknowledge
        return true;
    }

    // Issue error
    return false;
}


/**
 * @brief Stop an I2C transaction.
 *
 * @param its:  The I2C state record.
 *
 * @retval Whether the op succeeded (`true`) or failed (`false`).
 */
bool stop_i2c(I2C_State* its) {

    if (its->is_ready && its->is_started) {
        // Send no bytes and STOP
        uint8_t data = 0;
        i2c_write_blocking(its->bus, its->address, &data, 0, true);

        // Reset state
        its->is_started = false;
        its->is_read_op = false;

        // Acknowledge
        return true;
    }

    // Issue error
    return false;
}


/**
 * @brief Write out I2C state data.
 *
 * @param its:    The I2C state record.
 * @param output: A pointer to the string storage into which to write the info.
 */
void get_i2c_state(I2C_State* its, char* output) {

    sprintf(output, "%s.%s.%s.%i.%i.%i.%i",
        (its->is_ready   ? "1" : "0"),          // 2 chars
        (its->is_started ? "1" : "0"),          // 2 chars
        (its->bus == i2c0 ? "0" : "1"),         // 2 chars
        its->sda_pin,                           // 2-3 chars
        its->scl_pin,                           // 2-3 chars
        its->frequency,                         // 2 chars
        its->address);                          // 2-4 chars
}