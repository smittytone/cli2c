/*
 * I2C Host
 *
 * @version     0.1.0
 * @author      Tony Smith (@smittytone)
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
    uint32_t read_count = 0;

    // Prepare a transaction record with default data
    I2C_Trans transaction;
    transaction.is_started = false;
    transaction.is_ready = false;
    transaction.frequency = 100;
    transaction.address = 0xFF;

    // Loop-related variables
    bool state = false;
    int led_flash_count = 0;

    led_off();

    while(1) {
        // Clear buffer and listen for input
        memset(rx_buffer, 0, sizeof(rx_buffer));
        read_count = rx(rx_buffer);

        // Did we receive anything?
        if (read_count > 0) {
            // Are we expecting write data or a read op next?
            uint8_t status_byte = rx_buffer[0];
            if (transaction.is_started && status_byte >= READ_LENGTH_BASE) {
                // We have data or a read op
                if (status_byte > WRITE_LENGTH_BASE) {
                    // Write data received, so send it and ACK
                    transaction.write_byte_count = status_byte - WRITE_LENGTH_BASE;
                    int bytes_sent = write_i2c(transaction.address, &rx_buffer[1], transaction.write_byte_count, false);
                    
                    // Send an ACK to say we wrote the data -- or an ERR if we didn't
                    if (bytes_sent != PICO_ERROR_GENERIC) {
                        send_ack();
                    } else {
                        send_err();
                    }
                } else {
                    // Read length received only
                    transaction.read_byte_count = status_byte - READ_LENGTH_BASE;
                    uint8_t i2x_rx_buffer[65] = {0};
                    int bytes_read = read_i2c(transaction.address, i2x_rx_buffer, transaction.read_byte_count, false);

                    // Return the read data
                    if (bytes_read != PICO_ERROR_GENERIC) {
                        // printf("%s\r\n", buffer);
                        tx(i2x_rx_buffer, transaction.read_byte_count);
                    }

                    // TODO --  report read error? 
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

                    case '!':   // GET COMMANDS
                        send_commands();
                        break;
                    
                    case 'd':   //SCAN
                        if (transaction.is_ready) {
                            send_scan();
                        } else {
                            send_err();
                        }
                        break;

                    case 'i':   // INITIALISE I2C
                        /*
                        if (!transaction.is_ready) {
                            init_i2c(transaction.frequency);
                            transaction.is_ready = true;
                            send_ack();
                        } else {
                            send_err();
                        }
                        */
                        init_i2c(transaction.frequency);
                        transaction.is_ready = true;
                        send_ack();
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
        }

        led_flash_count++;
        if (led_flash_count > 100) {
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

    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    //gpio_set_function(SDA_STEMMA, GPIO_FUNC_I2C);
    //gpio_set_function(SCL_STEMMA, GPIO_FUNC_I2C);

    //gpio_pull_up(SDA_STEMMA);
    //gpio_pull_up(SCL_STEMMA);

}


/**
 * @brief Write data to the host's I2C bus.
 *
 * @param address: The target I2C address.
 * @param data:    A pointer to the data.
 * @param count:   The number of bytes to send.
 * @param do_stop: Issue a STOP after sending.
 */
int write_i2c(uint8_t address, uint8_t* data, uint32_t count, bool do_stop) {

    return i2c_write_blocking(I2C_PORT, address >> 1, data, count, do_stop);
}


/**
 * @brief Read data from the host's I2C bus.
 *
 * @param address: The target I2C address.
 * @param data:    A pointer to the data store
 * @param count:   The number of bytes to receive.
 * @param do_stop: Issue a STOP after receiving.
 */
int read_i2c(uint8_t address, uint8_t* data, uint32_t count, bool do_stop) {

    return i2c_read_blocking(I2C_PORT, address >> 1, data, count, do_stop);
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

    uint8_t rx_data;
    int reading;
    char scan_buffer[1024] = {0};
    uint32_t device_count = 0;;

    // Generate a list if devices by their addresses.
    // List in the form "13.71.A0."
    for (uint32_t i = 0 ; i < 0x78 ; ++i) {
        reading = i2c_read_blocking(I2C_PORT, i, &rx_data, 1, false);
        if (reading >= 0) {
            sprintf(scan_buffer + (device_count * 3), "%02X.", i);
            device_count++;
        }
    }

    if (strlen(scan_buffer) == 0) {
        sprintf(scan_buffer, "Z\r\n");
    } else {
        sprintf(scan_buffer + (device_count * 3), "\r\n");
    }

    // Send the scan data back
    tx(scan_buffer, strlen(scan_buffer));
}


/**
 * @brief Scan the host's I2C bus for devices, and send the results.
 *
 * @param t: A pointer to the current I2C transaction record.
 */
void send_status(I2C_Trans* t) {

    // Generate and return the status data string.
    // Data in the form: "1.1.100.110.QTPY-RP2040" or "1.1.100.110.PI-PICO"
    char out_buffer[48] = {0};
    sprintf(out_buffer, "%s.%s.%i.%i.%s\r\n",
            (t->is_ready   ? "1" : "0"),        // 2 chars
            (t->is_started ? "1" : "0"),        // 2 chars
            t->frequency,                       // 4 chars
            t->address,                         // 2-4 chars
            HW_MODEL);                          // 2-17 chars
                                                // == 10-27 chars

    // Send the data
    tx(out_buffer, strlen(out_buffer));
}


void send_commands() {

    char out_buffer[] = "Commands: ? ! d 1 4 i s p x\r\n";
    tx(out_buffer, strlen(out_buffer));
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
 * @brief Read in a single transmitted block.
 *
 * @param buffer: A pointer to the byte store buffer,
 *
 * @retval The number of bytes to process.
 */
uint32_t rx(uint8_t* buffer) {

    uint32_t data_count = 0;
    while (data_count < 128) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT || data_count > 127) break;
        buffer[data_count++] = (uint8_t)(c & 0xFF);
    }

    return data_count;

    /*
    uint64_t now = time_us_64();
    int c = -1;

    while (time_us_64() - now > 200000 && data_count < 128) {   // 20ms, 500000us -- can be lower?
        c = getchar_timeout_us(0);  
        if (c != PICO_ERROR_TIMEOUT) {
            buffer[data_count++] = (uint8_t)(c & 0xFF);
        }
    }

    return data_count;
    */
}


void tx(uint8_t* buffer, uint32_t byte_count) {
    
    for (uint32_t i = 0 ; i < byte_count ; ++i) {
        putchar((buffer[i]));;
    }

    stdio_flush();
}