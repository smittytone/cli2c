/*
 * e6809 for Raspberry Pi Pico
 * Monitor code
 *
 * @version     1.0.0
 * @author      smittytone
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "serial.h"


/**
 * @brief Listen on the USB-fed stdin for signals from the driver.
 */
void rx_loop() {

    // Prepare a UART RX buffer
    uint8_t rx_buffer[128] = {0};
    uint32_t bytes_read = 0;

    // Prepare a transaction record with default data
    I2C_Trans transaction;
    transaction.is_started = false;
    transaction.is_ready = false;
    transaction.frequency = 100;
    transaction.address = 0xFF;

    led_flash(5);

    bool state = true;
    int led_flash_count = 0;

    while(1) {
        // Listen for input
        bytes_read = get_tx_block(rx_buffer);

        // Did we receive anything?
        if (bytes_read > 0) {
            // Are we expecting write data or a read op next?
            uint8_t status_byte = rx_buffer[0];
            if (status_byte >= READ_LENGTH_BASE && transaction.is_started) {
                // We have data or a read op
                if (status_byte > WRITE_LENGTH_BASE) {
                    // Write data received, so send it and ACK
                    transaction.write_byte_count = status_byte - WRITE_LENGTH_BASE;
                    write_i2c(transaction.address, &rx_buffer[1], transaction.write_byte_count, false);
                    send_ack();
                } else {
                    // Read length received only
                    transaction.read_byte_count = status_byte - READ_LENGTH_BASE;
                    uint8_t buffer[65] = {0};
                    read_i2c(transaction.address, buffer, 64, false);

                    // Return the read data
                    printf("%s\r\n", buffer);
                }
            } else {
                // Maybe we received a command
                char cmd = (char)status_byte;
                switch(cmd) {
                    case '1':   // SET BUS TO 100kHz
                        if (transaction.frequency != 100) {
                            transaction.frequency = 100;

                            // If the bus is active, reset it
                            if (transaction.is_ready) {
                                reset_i2c(transaction.frequency);
                                transaction.is_started = false;
                            }
                        }
                        send_ack();
                        break;

                    case '4':   // SET BUS TO 400kHZ
                        if (transaction.frequency != 400) {
                            transaction.frequency = 400;

                            // If the bus is active, reset it
                            if (transaction.is_ready) {
                                reset_i2c(transaction.frequency);
                                transaction.is_started = false;
                            }
                        }
                        send_ack();
                        break;

                    case '?':   // GET STATUS
                        send_status(&transaction);
                        break;

                    case 'd':   //SCAN
                        if (transaction.is_ready) {
                            send_scan();
                        } else {
                            send_err();
                        }
                        break;

                    case 'i':   // INITIALISE I2C
                        if (!transaction.is_ready) {
                            init_i2c(transaction.frequency);
                            transaction.is_ready = true;
                            send_ack();
                        } else {
                            send_err();
                        }
                        break;

                    case 'p':   // I2C STOP
                        if (transaction.is_ready && transaction.is_started) {
                            // Send no bytes and STOP
                            write_i2c(transaction.address, rx_buffer, 0, true);

                            // Reset state
                            transaction.is_started = false;
                            transaction.is_read_op = false;
                            transaction.address = 0xFF;
                            send_ack();
                        } else {
                            send_err();
                        }
                        break;

                    case 's':   // START I2C TRANSACTION
                        if (transaction.is_ready) {
                            // Received data is in the form ['s', (address << 1) | op];
                            transaction.address = rx_buffer[1] & 0xFE;
                            transaction.is_read_op = ((rx_buffer[1] & 0x01) == 1);
                            transaction.is_started = true;

                            // Acknowledge
                            send_ack();
                        } else {
                            // Issue error
                            send_err();
                        }
                        break;

                    case 'x':   // RESET BUS
                        transaction.is_started = false;
                        reset_i2c(transaction.frequency);
                        send_ack();
                        break;

                    case 'z':   // CONNECTION TEST DATA
                        printf("OK\r\n");
                        break;

                    default:    // UNKNOWN COMMAND -- FAIL
                        send_err();

                }
            }

            // Clear buffer for next time
            bytes_read = 0;
        }

        led_flash_count++;
        if (led_flash_count > 50) {
            led_flash_count = 0;
            state = !state;
            led_set_state(state);
        }

        // Pause? May not be necessary or might be bad
        sleep_ms(10);
    }

    // Should not get here, but just in case...
    // Signal an error on the host's LED
    led_set_colour(0xFF0000);
    led_flash(5);
}


/**
 * @brief Initialise the host's I2C bus.
 *
 * @param frequency_khz: The bus speed in kHz.
 */
void init_i2c(int frequency_khz) {

    // Intialise I2C via SDK
    i2c_init(I2C_PORT, frequency_khz * 1000);

    // Initialise pins
#ifdef DEBUG
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
#else
    gpio_set_function(SDA_STEMMA, GPIO_FUNC_I2C);
    gpio_set_function(SCL_STEMMA, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_STEMMA);
    gpio_pull_up(SCL_STEMMA);
#endif
}


/**
 * @brief Write data to the host's I2C bus.
 *
 * @param address: The target I2C address.
 * @param data:    A pointer to the data.
 * @param count:   The number of bytes to send.
 * @param do_stop: Issue a STOP after sending.
 */
void write_i2c(uint8_t address, uint8_t* data, uint32_t count, bool do_stop) {

    i2c_write_blocking(I2C_PORT, address, data, count, do_stop);
}


/**
 * @brief Read data from the host's I2C bus.
 *
 * @param address: The target I2C address.
 * @param data:    A pointer to the data store
 * @param count:   The number of bytes to receive.
 * @param do_stop: Issue a STOP after receiving.
 */
void read_i2c(uint8_t address, uint8_t* data, uint32_t count, bool do_stop) {

    int r = i2c_read_blocking(I2C_PORT, address, data, count, do_stop);
}


/**
 * @brief Reset the host's I2C bus.
 *
 * @param frequency_khz: The bus speed in kHz.
 */
void reset_i2c(int frequency_khz) {

    i2c_deinit(I2C_PORT);
    sleep_ms(10);
    init_i2c(frequency_khz);
}


/**
 * @brief Scan the host's I2C bus for devices, and send the results.
 */
void send_scan() {

    uint8_t rxdata;
    int ret;
    char scan_buffer[363] = {0};
    char* buffer_ptr = scan_buffer;

    // Generate a list if devices by their addresses.
    // List in the form "13.71.A0."
    for (uint32_t i = 0 ; i < 0x78 ; ++i) {
        ret = i2c_read_blocking(I2C_PORT, i, &rxdata, 1, false);
        if (ret >= 0) {
            sprintf(buffer_ptr, "%02X.", i);
        } else {
            sprintf(buffer_ptr, "00.", i);
        }

        buffer_ptr += 3;
    }

    // Send the scan data back
    printf("%s\r\n", scan_buffer);
}


/**
 * @brief Scan the host's I2C bus for devices, and send the results.
 *
 * @param t: A pointer to the current I2C transaction record.
 */
void send_status(I2C_Trans* t) {

    // Generate the status data string.
    // Data in the form: "1.1.100.110.QTPY-RP2040"
    printf("%s.%s.%i.%i.%s\r\n",
            (t->is_ready ? "1" : "0"),          // 2 chars
            (t->is_started ? "1" : "0"),        // 2 chars
            t->frequency,                       // 4 chars
            t->address,                         // 2-4 chars
            HW_MODEL);                          // 2-17 chars
                                                // == 10-27 chars

    // Send the status data back
    // NOTE `HW_MODEL` set by CMake according to the device
    //      we're building for
    //printf("%s.%s\r\n", status_buffer, HW_MODEL);
}


/**
 * @brief Send a single-byte ACK.
 */
void send_ack() {
#ifdef BUILD_FOR_TERMINAL_TESTING
    printf("OK\r\n");
#else
    putchar(0x01);
#endif
}


/**
 * @brief Send a single-byte ERR.
 */
void send_err() {
#ifdef BUILD_FOR_TERMINAL_TESTING
    printf("ERR\r\n");
#else
    putchar(0xFF);
#endif
}


/**
 * @brief Read in a single transmitted block (up to 4 kbytes).
 *
 * @param buffer: A pointer to the byte store buffer,
 *
 * @retval The number of bytes to process.
 */
uint32_t get_tx_block(uint8_t *buffer) {

    uint32_t data_count = 0;

    while (data_count < 4096) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT || data_count > 4095) break;
        buffer[data_count++] = (c & 0xFF);
    }

    return data_count;
}
