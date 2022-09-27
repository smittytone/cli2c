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
 * @brief Listen on the stdin for signals from the driver.
 */
void input_loop() {

    uint8_t load_buffer[128] = {0};
    uint32_t bytes_read = 0;

    I2C_Trans transaction;
    transaction.is_started = false;
    transaction.is_inited = false;
    transaction.frequency = 100;
    transaction.address = 0xFF;

    led_flash(5);

    bool state = true;
    int flash_count = 0;

    while(true) {
        // Listen for input
        bytes_read = get_block(load_buffer);

        if (bytes_read > 0) {
            // Are we expecting write data or a read op next?
            uint8_t status_byte = load_buffer[0];
            if (status_byte >= READ_LENGTH_BASE && transaction.is_started) {
                // We have data or a read op
                if (status_byte > WRITE_LENGTH_BASE) {
                    // Write data
                    transaction.write_byte_count = status_byte - WRITE_LENGTH_BASE;
                    write_i2c(transaction.address, &load_buffer[1], transaction.write_byte_count, false);
                    do_ack();
                } else {
                    // Read length
                    transaction.read_byte_count = status_byte - READ_LENGTH_BASE;
                    uint8_t buffer[65] = {0};
                    read_i2c(transaction.address, buffer, 64, false);

                    // Return the read data
                    printf("%s\r\n", buffer);
                }
            } else {
                // Get the first byte from the buffer
                char cmd = (char)status_byte;

                // What command have we received
                switch(cmd) {
                    case '1':   // SET 100kHz
                        if (transaction.frequency != 100) {
                            transaction.frequency = 100;

                            if (transaction.is_inited) {
                                do_reset_i2c(transaction.frequency);
                                transaction.is_started = false;
                            }
                        }
                        do_ack();
                        break;

                    case '4':   // SET 400kHZ
                        if (transaction.frequency != 400) {
                            transaction.frequency = 400;

                            if (transaction.is_inited) {
                                do_reset_i2c(transaction.frequency);
                                transaction.is_started = false;
                            }
                        }
                        do_ack();
                        break;

                    case '?':   // GET STATUS
                        do_print_status(&transaction);
                        break;

                    case 'd':   //SCAN
                        if (transaction.is_inited) {
                            do_scan();
                        } else {
                            do_err();
                        }
                        break;

                    case 'i':   // INITIALISE I2C
                        if (!transaction.is_inited) {
                            init_i2c(transaction.frequency);
                            transaction.is_inited = true;
                            do_ack();
                        } else {
                            do_err();
                        }
                        break;

                    case 'p':   // I2C STOP
                        if (transaction.is_inited && transaction.is_started) {
                            // Send no bytes and STOP
                            write_i2c(transaction.address, load_buffer, 0, true);

                            // Reset state
                            transaction.is_started = false;
                            transaction.is_read_op = false;
                            transaction.address = 0xFF;
                            do_ack();
                        } else {
                            do_err();
                        }
                        break;

                    case 's':   // START I2C TRANSACTION
                        if (transaction.is_inited) {
                            // Sent data: ['s', (address << 1) | op];
                            transaction.address = load_buffer[1] & 0xFE;
                            transaction.is_read_op = load_buffer[1] &0x01;
                            transaction.is_started = true;

                            // Acknowledge
                            do_ack();
                        } else {
                            // Issue error
                            do_err();
                        }
                        break;

                    case 'x':   // RESET BUS
                        transaction.is_started = false;
                        do_reset_i2c(transaction.frequency);
                        do_ack();
                        break;

                    case 'z':
                        printf("Success\r\n");
                        break;

                    default:    // UNKNOWN COMMAND
                        do_err();

                }
            }

            // Clear buffer for next time
            bytes_read = 0;
        }

        flash_count++;
        if (flash_count > 50) {
            flash_count = 0;
            state = !state;
            led_set_state(state);
        }

        // Pause? May not be necessary or might be bad
        sleep_ms(10);
    }

    // Signal an error and turn off the LED
    led_set_colour(0xFF0000);
    led_flash(5);
}


void init_i2c(int frequency_khz) {
    // Intialise I2C
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


void write_i2c(uint8_t address, uint8_t* data, uint32_t count, bool do_stop) {
    i2c_write_blocking(I2C_PORT, address, data, count, do_stop);
}


void read_i2c(uint8_t address, uint8_t* data, uint32_t count, bool do_stop) {
    int r = i2c_read_blocking(I2C_PORT, address, data, count, do_stop);

}


void do_scan() {
    int ret;
    uint8_t rxdata;
    char buffer[361] = {0};
    char* buffer_ptr = buffer;

    for (uint32_t i = 0 ; i < 0x78 ; ++i) {
        ret = i2c_read_blocking(I2C_PORT, i, &rxdata, 1, false);
        if (ret >= 0) {
            sprintf(buffer_ptr, "%02X.", i);
            buffer_ptr += 2;
        }
    }

    printf("%s\r\n", buffer);
}


void do_print_status(I2C_Trans* t) {
    char status_buffer[64] = {0};
    sprintf(status_buffer, "%s.%s.%i.%i",
            (t->is_inited ? "1" : "0"),         // 2 chars
            (t->is_started ? "1" : "0"),        // 2 chars
            t->frequency,                       // 4 chars
            t->address);                        // 2-4 chars
                                                // 2-17 chars (HW_MODEL)
    printf("%s.%s\r\n", status_buffer, HW_MODEL);
}


void do_reset_i2c(int frequency_khz) {
    i2c_deinit(I2C_PORT);
    sleep_ms(10);
    init_i2c(frequency_khz);
}


void do_ack() {
#ifdef BUILD_FOR_TERMINAL
    printf("OK\r\n");
#else
    put(1);
#endif
}


void do_err() {
#ifdef BUILD_FOR_TERMINAL
    printf("ERR\r\n");
#else
    put(0xFF);
#endif
}


/**
 * @brief Read in a single transmitted block (up to 262 bytes).
 *
 * @param buff: A pointer to the byte store buffer,
 *
 * @retval The number of bytes to process.
 */
uint32_t get_block(uint8_t *buff) {
    uint32_t data_count = 0;

    while (data_count < 4096) {
        int c = getchar_timeout_us(SERIAL_READ_TIMEOUT_US);
        if (c == PICO_ERROR_TIMEOUT || data_count > 4094) break;
        buff[data_count++] = (c & 0xFF);
    }

    return data_count;
}
