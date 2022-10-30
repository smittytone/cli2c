/*
 * Bus Host Firmware - Primary serial and command functions
 *
 * @version     2.0.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "serial.h"



/*
 * STATIC PROTOTYPES
 */
static void     send_status(I2C_State* itr);
static void     send_ack(void);
static void     send_err(void);
static uint32_t rx(uint8_t *buffer);
// FROM 2.0.0
static uint8_t  get_mode(char mode_key);
static void     send_host_status(uint8_t mode);


/**
 * @brief Listen on the USB-fed stdin for signals from the driver.
 */
void rx_loop(void) {

    // Prepare a UART RX buffer
    uint8_t rx_buffer[128] = {0};
    uint32_t read_count = 0;
    bool do_use_led = true;

    // Prepare a transaction record with default data
    I2C_State i2c_state;
    i2c_state.is_started = false;                           // No transaction taking place
    i2c_state.is_ready = false;                             // I2C bus not yet initialised
    i2c_state.bus = DEFAULT_I2C_BUS == 0 ? i2c0 : i2c1;     // The I2C bus to use
    i2c_state.frequency = 400;                              // The I2C bus frequency in use
    i2c_state.sda_pin = DEFAULT_SDA_PIN;                    // The I2C SDA pin
    i2c_state.scl_pin = DEFAULT_SCL_PIN;                    // The I2C SCL pin
    i2c_state.address = 0xFF;                               // The target I2C address

    // FROM 1.1.0 -- record GPIO pin state
    GPIO_State gpio_state;
    memset(gpio_state.state_map, 0, 32);

    // FROM 2.0.0 -- Prepare an SPI transaction record with default data
    SPI_State spi_state;
    spi_state.is_ready = false;
    spi_state.is_started = false;
    spi_state.bus = DEFAULT_SPI_BUS == 0 ? spi0 : spi1;     // The SPI bus to use
    spi_state.baudrate = 500;                               // The SPI bus baud rate
    spi_state.rx_pin = DEFAULT_SPI_RX_PIN;                  // The SPI RX pin
    spi_state.tx_pin = DEFAULT_SPI_TX_PIN;                  // The SPI TX pin
    spi_state.cs_pin = DEFAULT_SPI_CS_PIN;                  // The SPI CS pin
    spi_state.sck_pin = DEFAULT_SPI_SCK_PIN;                // The SPI SCK pin

    // FROM 2.0.0
    // Default current mode to I2C, for backwards compatibility
    // NOTE Call the function so the LED colour is correctly set
    uint8_t current_mode = get_mode('i');
    void* bus_states[2] = {&i2c_state, &spi_state};

    // Heartbeat variables
    uint64_t last = time_us_64();

#ifdef DO_DEBUG
    // Set up a parallel segment display to assist
    // with debugging
    init_i2c(i2c_state.frequency);
    HT16K33_init();
    HT16K33_clear_buffer();
    HT16K33_draw();
#endif

    while(1) {
        // Scan for input
        read_count = rx(rx_buffer);
        bool client_is_old = true;

        // Did we receive anything?
        if (read_count > 0) {
            // Are we expecting write data or a read op next?
            // NOTE The first byte will always be:
            //      32-127  (ascii char as a command),
            //      128-191 (read 1-64 bytes), or
            //      192-255 (write 1-64 bytes)
            uint8_t status_byte = rx_buffer[0];
            uint8_t* rx_ptr = rx_buffer;
            char cmd = (char)status_byte;

            // FROM 2.0.0
            // Look for the key and mode specifier bytes
            if (cmd == '#') {
                // Make sure we have enough subsequent bytes
                if (read_count < 3) {
                    send_err();
                    continue;
                }

                // Set the mode from the mode specifier byte
                current_mode = get_mode(rx_buffer[1]);

                // Move the status/command and buffer pointer on two bytes
                // past the key and mode specifier bytes
                status_byte = rx_buffer[2];
                rx_ptr = &rx_buffer[2];

                // Note client type
                client_is_old = false;
            }

            if (status_byte >= READ_LENGTH_BASE) {
                // We have data or a read op
                if (status_byte >= WRITE_LENGTH_BASE) {
                    // Write data received, so send it and ACK
                    int bytes_sent = 0;
                    switch(current_mode) {
                        case MODE_I2C:
                            if (i2c_state.is_started) {
                                i2c_state.write_byte_count = status_byte - WRITE_LENGTH_BASE + 1;
                                bytes_sent = i2c_write_timeout_us(i2c_state.bus, i2c_state.address, &rx_buffer[1], i2c_state.write_byte_count, false, 1000);

                                // Send an ACK to say we wrote the data
                                if (bytes_sent != PICO_ERROR_GENERIC && bytes_sent != PICO_ERROR_TIMEOUT) {
                                    send_ack();
                                    break;
                                }
                            }
                            send_err();
                            break;
                        case MODE_SPI:
                            if (spi_state.is_started) {
                                spi_state.write_byte_count = status_byte - WRITE_LENGTH_BASE + 1;
                                spi_state.read_byte_count = spi_state.write_byte_count;
                                uint8_t rx_data[spi_state.read_byte_count];
                                bytes_sent = spi_write_read_blocking(spi_state.bus, &rx_buffer[1], rx_data, spi_state.write_byte_count);

                                // Send back any read data -- or an ERR
                                if (bytes_sent != PICO_ERROR_GENERIC && bytes_sent != PICO_ERROR_TIMEOUT) {
                                    tx(rx_data, spi_state.read_byte_count);
                                    break;
                                }
                            }
                            send_err();
                    }
                } else {
                    switch(current_mode) {
                        case MODE_I2C:
                            if (i2c_state.is_started) {
                                // Read length received only
                                i2c_state.read_byte_count = status_byte - READ_LENGTH_BASE + 1;
                                uint8_t i2x_rx_buffer[65] = {0};
                                int bytes_read = i2c_read_timeout_us(i2c_state.bus, i2c_state.address, i2x_rx_buffer, i2c_state.read_byte_count, false, 1000);

                                // Return the read data
                                if (bytes_read != PICO_ERROR_GENERIC) {
                                    tx(i2x_rx_buffer, i2c_state.read_byte_count);
                                    send_ack();
                                    break;
                                }
                            }
                            send_err();
                            break;
                        case MODE_SPI:
                            if (spi_state.is_started) {


                            }
                            send_err();
                    }
                }
            } else {
                // Maybe we received a command
                char cmd = (char)status_byte;
                switch(cmd) {
                    /*
                     * GENERIC HOST COMMANDS
                     */
                    case '?':   // GET STATUS
                        if (client_is_old) {
                            send_status(&i2c_state);
                        } else {
                            send_host_status(current_mode);
                        }
                        break;

                        // FROM 2.0.0 -- remove z
                        // FROM 1.1.1 -- change command from z to !
                        case '!':   // RESPOND TO CONNECTION REQUEST
                            tx("OK\r\n", 4);
                            break;

                        // FROM 1.1.0
                        case '*':   // SET LED STATE
                                    // APPLIES TO ALL MODE
                            do_use_led = rx_ptr[1] == 1 ? true : false;
#ifdef SHOW_HEARTBEAT
                            send_ack();
#else
                            send_err();
#endif
                            break;

                        /*
                         * MULTI-BUS COMMANDS
                         */
                        case 'c':   // CONFIGURE THE BUS AND PINS
                                    // APPLIES TO: I2C, SPI
                            {
                                bool done = false;
                                switch (current_mode) {
                                    case MODE_I2C:
                                        done = configure_i2c(&i2c_state, rx_ptr);
                                        break;
                                    case MODE_SPI:
                                        done = configure_spi(&spi_state, rx_ptr);
                                }
                                // Send ACK or ERR on success or failure
                                putchar(done ? ACK : ERR);
                                break;
                            }

                        case 'i':   // INITIALISE A BUS
                                    // APPLIES TO: I2C, SPI
                            switch(current_mode) {
                                case MODE_I2C:
                                    // No need it initialise if we already have
                                    if (!i2c_state.is_ready) init_i2c(&i2c_state);
                                    break;
                                case MODE_SPI:
                                    if (!spi_state.is_ready) init_spi(&spi_state);
                            }
                            send_ack();
                            break;

                        case 'x':   // RESET A BUS
                                    // APPLIES TO I2C, SPI
                            switch(current_mode) {
                                case MODE_I2C:
                                    i2c_state.is_started = false;
                                    reset_i2c(&i2c_state);
                                    break;
                                case MODE_SPI:
                                    spi_state.is_started = false;
                                    reset_spi(&spi_state);
                            }
                            send_ack();
                            break;

                        /*
                         * I2C-SPECIFIC COMMANDS
                         */
                        case '1':   // SET I2C BUS TO 100kHz
                            set_i2c_frequency(&i2c_state, 100);
                            send_ack();
                            break;

                        case '4':   // SET I2C BUS TO 400kHZ
                            set_i2c_frequency(&i2c_state, 400);
                            send_ack();
                            break;

                        case 'd':   // SCAN THE I2C BUS FOR DEVICES
                            if (!i2c_state.is_ready) init_i2c(&i2c_state);
                            send_scan(&i2c_state);
                            break;

                        case 'p':   // SEND AN I2C STOP
                            if (stop_i2c(&i2c_state)) {
                                send_ack();
                            } else {
                                send_err();
                            }
                            break;

                        case 's':   // START AN I2C TRANSACTION
                            if (start_i2c(&i2c_state, rx_ptr)) {
                                send_ack();
                            } else {
                                send_err();
                            }
                            break;

                        /*
                         * SPI-SPECIFIC COMMANDS
                         */

                        /*
                         * GPIO COMMANDS
                         */
                        case 'g':   // SET DIGITAL OUT PIN
                                    // APPLIES TO NO/ALL BUSES
                            {
                                uint8_t read_value = 0;
                                if (!set_gpio(&gpio_state, &read_value, rx_ptr)) {
                                    send_err();
                                    break;
                                }

                                bool is_read = ((rx_ptr[1] & 0x20) > 0);
                                putchar(is_read ? read_value : ACK);
                                break;
                            }

                        default:    // UNKNOWN COMMAND -- FAIL
                            send_err();
                }
            }

            // Clear the RX buffer and listen for input
            memset(rx_buffer, 0, read_count);
        }

#ifdef SHOW_HEARTBEAT
        // Heartbeat LED blink for debugging
        if (do_use_led) {
            uint64_t now = time_us_64();
            if (now - last > HEARTBEAT_PERIOD_US) {
                led_set_state(true);
                last = now;
            } else if (now - last > HEARTBEAT_FLASH_US) {
                led_set_state(false);
            }
        }
#endif

        // Pause? May not be necessary or might be bad
        current_mode = MODE_NONE;
        sleep_ms(RX_LOOP_DELAY_MS);
    }

    // Should not get here, but just in case...
    // Signal an error on the host's LED
    led_set_colour(0xFF0000);
    led_flash(5);

    // Fall out of the firmware at this point...
}


/**
 * @brief Scan the host's I2C bus for devices, and send the results.
 *
 * @param itr: A pointer to the current I2C transaction record.
 */
static void send_status(I2C_State* itr) {

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
    //strncpy(model, model, HW_MODEL_NAME_SIZE_MAX);
    strncat(model, HW_MODEL, HW_MODEL_NAME_SIZE_MAX);

    // Generate and return the status data string.
    // Data in the form: "1.1.100.110.QTPY-RP2040" or "1.1.100.110.PI-PICO"
    char status_buffer[129] = {0};

    sprintf(status_buffer, "%s.%s.%s.%i.%i.%i.%i.%i.%i.%i.%i.%s.%s\r\n",
            (itr->is_ready   ? "1" : "0"),          // 2 chars
            (itr->is_started ? "1" : "0"),          // 2 chars
            (itr->bus == i2c0 ? "0" : "1"),         // 2 chars
            itr->sda_pin,                           // 2-3 chars
            itr->scl_pin,                           // 2-3 chars
            itr->frequency,                         // 2 chars
            itr->address,                           // 2-4 chars
            major,                                  // 2-4 chars
            minor,                                  // 2-4 chars
            patch,                                  // 2-4 chars
            BUILD_NUM,                              // 2-4 chars
            pid,                                    // 17 chars
            model);                                 // 2-17 chars
                                                    // == 41-68 chars

    // Send the data
    tx(status_buffer, strlen(status_buffer));
}


static void send_host_status(uint8_t mode) {

    // Get the RP2040 unique ID
    // eg. DF6050788B3E1A2E
    char pid[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1] = {0};
    pico_get_unique_board_id_string(pid, 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1);

    // Get the firmware version as integers
    int major, minor, patch;
    sscanf(FW_VERSION, "%i.%i.%i",
        &major,
        &minor,
        &patch
    );

    char model[HW_MODEL_NAME_SIZE_MAX + 1] = {0};
    strncat(model, HW_MODEL, HW_MODEL_NAME_SIZE_MAX);

    // Generate and return the status data string.
    // Data in the form:
    // "xxx.yyy.zzz.aaa.b.A0B1C2D3E4F5A6B7.RP-PI-PICO"
    char status_buffer[256] = {0};

    sprintf(status_buffer, "%i.%i.%i.%i.%i.%s.%s\r\n",
            major,                                  // 2-4 chars
            minor,                                  // 2-4 chars
            patch,                                  // 2-4 chars
            BUILD_NUM,                              // 2-4 chars
            mode,                                   // 2 chars
            pid,                                    // 17 chars
            model);                                 // 2-17 chars
                                                    // == 41-68 chars
    // Send the data
    tx(status_buffer, strlen(status_buffer));
}


/**
 * @brief Send a single-byte ACK.
 */
static void send_ack(void) {
#ifdef BUILD_FOR_TERMINAL_TESTING
    printf("OK\r\n");
#else
    putchar(ACK);
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
static void send_err(void) {
#ifdef BUILD_FOR_TERMINAL_TESTING
    printf("ERR\r\n");
#else
    putchar(ERR);
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
 * @param buffer: A pointer to the byte store buffer.
 *
 * @retval The number of bytes to process.
 */
static uint32_t rx(uint8_t* buffer) {

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


/**
 * @brief Send a single transmitted block.
 *
 * @param buffer:     A pointer to the byte store buffer.
 * @param byte_count: The number of bytes to send.
 */
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


/**
 * @brief Check that a supplied pin are valid for the
 *        board we're using
 *
 * @param pins:  The array of pin IDs
 * @param pin:   The pin we're checking.
 * @param delta: The pin increment: 2 for I2C, 4 for SPI, etc.
 *
 * @retval Whether the pins are good (`true`) or not (`false`).
 */
bool pin_check(uint8_t* pins, uint8_t pin, uint8_t delta) {

    uint8_t a_pin = *pins;
    while (a_pin != 255) {
        if (a_pin == pin) return true;
        pins += delta;
        a_pin = *pins;
    }

    return false;
}


/**
 * @brief Return in the mode (I2C, SPI, etc.) integer ID from the
 *        char ID sent to the host from the client.
 *
 *        It also sets the device LED colour.
 *
 * @param mode_key: The mode character: `i`, `s` etc.
 *                  NOTE Should always follow a `#`.
 *
 * @retval The mode ID as an integer.
 */
static uint8_t get_mode(char mode_key) {

    switch(mode_key) {
        case 'i':
        case 'I':
            led_set_colour(COLOUR_MODE_I2C);
            return MODE_I2C;
        case 's':
        case 'S':
            led_set_colour(COLOUR_MODE_SPI);
            return MODE_SPI;
        case 'u':
        case 'U':
            led_set_colour(COLOUR_MODE_UART);
            return MODE_UART;
        case 'o':
        case 'O':
        case '1':
            led_set_colour(COLOUR_MODE_ONE_WIRE);
            return MODE_ONE_WIRE;
    }

    // Error condition
    return MODE_NONE;
}