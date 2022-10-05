/*
 * I2C Host
 *
 * @version     0.1.1
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "serial.h"


/**
 * @brief Listen on the USB-fed stdin for signals from the driver.
 */
void rx_loop(void) {

    // Prepare a UART RX buffer
    uint8_t rx_buffer[128] = {0};
    uint32_t read_count = 0;

    // Prepare a transaction record with default data
    I2C_Trans transaction;
    transaction.is_started = false;     // No transaction taking place
    transaction.is_ready = false;       // I2C bus not yet initialised
    transaction.frequency = 400;        // The bud frequency in use
    transaction.address = 0xFF;         // The target I2C address

    // Loop-related variables
    bool state = false;
    int led_flash_count = 0;

#ifdef DO_DEBUG
    // Set up a parallel segment display to assist
    // with debugging
    init_i2c(transaction.frequency);
    HT16K33_init();
    HT16K33_clear_buffer();
    HT16K33_draw();
#endif

    while(1) {
        // Scan for input
        read_count = rx(rx_buffer);

        // Did we receive anything?
        if (read_count > 0) {
            // Are we expecting write data or a read op next?
            // NOTE The first byte will always be:
            //      32-127  (ascii char as a command), 
            //      128-191 (read 1-64 bytes), or 
            //      192-255 (write 1-64 bytes)
            uint8_t status_byte = rx_buffer[0];
            if (transaction.is_started && status_byte >= READ_LENGTH_BASE) {
                // We have data or a read op
                if (status_byte >= WRITE_LENGTH_BASE) {
                    // Write data received, so send it and ACK
                    transaction.write_byte_count = status_byte - WRITE_LENGTH_BASE + 1;
                    int bytes_sent = i2c_write_blocking(I2C_PORT, transaction.address, &rx_buffer[1], transaction.write_byte_count, false);

                    // Send an ACK to say we wrote the data -- or an ERR if we didn't
                    if (bytes_sent != PICO_ERROR_GENERIC) {
                        send_ack();
                    } else {
                        send_err();
                    }
                } else {
                    // Read length received only
                    transaction.read_byte_count = status_byte - READ_LENGTH_BASE + 1;
                    uint8_t i2x_rx_buffer[65] = {0};
                    int bytes_read = i2c_read_blocking(I2C_PORT, transaction.address, i2x_rx_buffer, transaction.read_byte_count, false);

                    // Return the read data
                    if (bytes_read != PICO_ERROR_GENERIC) {
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

                    case '!':   // GET COMMANDS LIST
                        send_commands();
                        break;

                    case 'd':   // SCAN THE I2C BUS FOR DEVICES
                        if (transaction.is_ready) {
                            send_scan();
                        } else {
                            send_err();
                        }
                        break;

                    case 'i':   // INITIALISE THE I2C BUS
                        init_i2c(transaction.frequency);
                        transaction.is_ready = true;
                        send_ack();
                        break;

                    case 'p':   // SEND AN I2C STOP
                        if (transaction.is_ready && transaction.is_started) {
                            // Send no bytes and STOP
                            i2c_write_blocking(I2C_PORT, transaction.address, rx_buffer, 0, true);

                            // Reset state
                            transaction.is_started = false;
                            transaction.is_read_op = false;
                            send_ack();
                        } else {
                            send_err();
                        }
                        break;

                    case 's':   // START I2C TRANSACTION
                        if (transaction.is_ready) {
                            // Received data is in the form ['s', (address << 1) | op];
                            transaction.address = (rx_buffer[1] & 0xFE) >> 1;
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
                        {
                            char tx_buffer[4] = "OK\r\n";
                            tx(tx_buffer, 4);
                        }
                        break;

                    default:    // UNKNOWN COMMAND -- FAIL
                        send_err();
                }
            }

            // Clear buffer and listen for input
            memset(rx_buffer, 0, read_count);
        }

#ifdef SHOW_HEARTBEAT
        // One-second heartbeat LED blink for debugging
        led_flash_count++;
        if (led_flash_count > 500 / RX_LOOP_DELAY_MS) {
            led_flash_count = 0;
            state = !state;
            led_set_state(state);
        }
#endif
        
        // Pause? May not be necessary or might be bad
        sleep_ms(RX_LOOP_DELAY_MS);
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
    // The values of SDA_PIN and SCL_PIN are set
    // in the board's individual CMakeLists.txt file.
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}


/**
 * @brief Reset the host's I2C bus.
 *
 * @param frequency_khz: The bus speed in kHz.
 */
void reset_i2c(int frequency_khz) {

    i2c_deinit(I2C_PORT);
    sleep_ms(10);
    i2c_init(I2C_PORT, frequency_khz * 1000);
}


/**
 * @brief Scan the host's I2C bus for devices, and send the results.
 */
void send_scan(void) {

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
 * @brief Scan the host's I2C bus for devices, and send the results.
 *
 * @param t: A pointer to the current I2C transaction record.
 */
void send_status(I2C_Trans* t) {

    // Get the RP2040 unique ID
    char pid[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1] = {0};
    pico_get_unique_board_id_string(pid, 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1);
    // eg. DF6050788B3E1A2E

    // Get the firmware version as integers
    int major, minor, patch;
    sscanf(FW_VERSION, "%i.%i.%i",
        &major,
        &minor,
        &patch
    );

    char model[HW_MODEL_NAME_SIZE_MAX + 1] = {0};
    strncpy(model, HW_MODEL, HW_MODEL_NAME_SIZE_MAX);

    // Generate and return the status data string.
    // Data in the form: "1.1.100.110.QTPY-RP2040" or "1.1.100.110.PI-PICO"
    char status_buffer[64] = {0};

    sprintf(status_buffer, "%s.%s.%i.%i.%i.%i.%i.%i.%s.%s\r\n",
            (t->is_ready   ? "1" : "0"),        // 2 chars
            (t->is_started ? "1" : "0"),        // 2 chars
            t->frequency,                       // 4 chars
            t->address,                         // 2-4 chars
            major,                              // 2-x chars
            minor,                              // 2-x chars
            patch,                              // 2-x chars
            BUILD_NUM,                          // 2-x chars
            pid,                                // 17 chars
            model);                             // 2-17 chars
                                                // == 26-43 chars

    // Send the data
    tx(status_buffer, strlen(status_buffer));
}


/**
 * @brief Send back a list of supported commands.
 */
void send_commands(void) {

    char cmd_list_buffer[] = "?.!.d.1.4.i.s.p.x\r\n";
    tx(cmd_list_buffer, strlen(cmd_list_buffer));
}


/**
 * @brief Send a single-byte ACK.
 */
void send_ack(void) {
#ifdef BUILD_FOR_TERMINAL_TESTING
    printf("OK\r\n");
#else
    putchar(0x01);
#endif

#ifdef DO_DEBUG
    // Write the sent char on the segment's second two digits
    HT16K33_set_number(0x00, 2, false);
    HT16K33_set_number(0x01, 3, false);
    HT16K33_draw();
#endif
}


/**
 * @brief Send a single-byte ERR.
 */
void send_err(void) {
#ifdef BUILD_FOR_TERMINAL_TESTING
    printf("ERR\r\n");
#else
    putchar(0xFF);
#endif

#ifdef DO_DEBUG
    // Write the sent char on the segment's second two digits
    HT16K33_set_number(0x0F, 2, false);
    HT16K33_set_number(0x0F, 3, false);
    HT16K33_draw();
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
    int c = PICO_ERROR_TIMEOUT;
    while (data_count < 128) {
        c = getchar_timeout_us(1);
        if (c == PICO_ERROR_TIMEOUT) break;
        buffer[data_count++] = (uint8_t)c;

#ifdef DO_DEBUG
        // Write the received char on the segment's first two digits
        HT16K33_set_number((uint8_t)((c & 0xF0) >> 4), 0, false);
        HT16K33_set_number((uint8_t)(c & 0x0F), 1, false);
        HT16K33_draw();
#else
        sleep_ms(10);
#endif
    }

    return data_count;
}


void tx(uint8_t* buffer, uint32_t byte_count) {

    for (uint32_t i = 0 ; i < byte_count ; ++i) {
        putchar((buffer[i]));

#ifdef DO_DEBUG
        // Write the sent char on the segment's second two digits
        HT16K33_set_number(((buffer[i] & 0xF0) >> 4), 2, false);
        HT16K33_set_number((buffer[i] & 0x0F), 3, false);
        HT16K33_draw();
#else
        sleep_ms(10);
#endif
    }

    stdio_flush();
}