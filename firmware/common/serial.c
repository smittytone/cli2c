/*
 * I2C Host Firmware - Primary serial and command functions
 *
 * @version     1.1.2
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "serial.h"



/*
 * STATIC PROTOTYPES
 */
static void         init_i2c(I2C_State* itr);
static void         reset_i2c(I2C_State* itr);
// FROM 1.1.2 -- make ack and err sends inline
static inline void  send_ack(void);
static inline void  send_err(void);
static void         send_scan(I2C_State* itr);
static void         send_status(I2C_State* itr);

static void         tx(uint8_t* buffer, uint32_t byte_count);
static uint32_t     rx(uint8_t *buffer);

// FROM 1.1.0
static bool         check_pins(uint8_t bus, uint8_t sda, uint8_t scl);
static bool         pin_check(uint8_t* pins, uint8_t pin);
static void         sig_handler(int signal);

/*
 * GLOBALS
 */
// FROM 1.1.0
// Access individual boards' pin arrays
extern uint8_t PIN_PAIRS_BUS_0[];
extern uint8_t PIN_PAIRS_BUS_1[];


/**
 * @brief Listen on the USB-fed stdin for signals from the driver.
 */
void rx_loop(void) {

    signal(SIGABRT | SIGSEGV | SIGBUS | SIGTRAP | SIGSYS, sig_handler);

    // Prepare a UART RX buffer
    uint8_t rx_buffer[128] = {0};
    uint32_t read_count = 0;
    bool do_use_led = true;
    
    // Heartbeat variables
    uint64_t last = time_us_64();
    bool is_on = false;

    // Prepare a transaction record with default data
    I2C_State i2c_state;
    i2c_state.is_started = false;                         // No transaction taking place
    i2c_state.is_ready = false;                           // I2C bus not yet initialised
    i2c_state.frequency = 400;                            // The bud frequency in use
    i2c_state.address = 0xFF;                             // The target I2C address
    i2c_state.bus = DEFAULT_I2C_BUS == 0 ? i2c0 : i2c1;   // The I2C bus to use
    i2c_state.sda_pin = DEFAULT_SDA_PIN;                  // The I2C SDA pin
    i2c_state.scl_pin = DEFAULT_SCL_PIN;                  // The I2C SCL pin
    
    // FROM 1.1.0 -- record GPIO pin state
    GPIO_State gpio_state;
    memset(gpio_state.state_map, 0, 32);

#ifdef DO_UART_DEBUG
    debug_init();
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
            uint8_t* rx_ptr = rx_buffer;

            if (i2c_state.is_started && status_byte >= READ_LENGTH_BASE) {
                // We have data or a read op
                if (status_byte >= WRITE_LENGTH_BASE) {
                    // Write data received, so send it and ACK
                    i2c_state.write_byte_count = status_byte - WRITE_LENGTH_BASE + 1;

#ifdef DO_UART_DEBUG
                    debug_log("Bytes to write: %i", i2c_state.write_byte_count);
#endif

                    int bytes_sent = i2c_write_timeout_us(i2c_state.bus, i2c_state.address, &rx_buffer[1], i2c_state.write_byte_count, false, 1000);

#ifdef DO_UART_DEBUG
                    debug_log("Bytes sent: %i", bytes_sent);
#endif
                    // Send an ACK to say we wrote the data -- or an ERR if we didn't
                    if (bytes_sent == PICO_ERROR_GENERIC || bytes_sent == PICO_ERROR_TIMEOUT) {
                        send_err();
                    } else {
                        send_ack();
                    }
                } else {
                    // Read length received only
                    i2c_state.read_byte_count = status_byte - READ_LENGTH_BASE + 1;
                    uint8_t i2x_rx_buffer[65] = {0};
                    int bytes_read = i2c_read_timeout_us(i2c_state.bus, i2c_state.address, i2x_rx_buffer, i2c_state.read_byte_count, false, 1000);

                    // Return the read data
                    if (bytes_read != PICO_ERROR_GENERIC) {
                        tx(i2x_rx_buffer, i2c_state.read_byte_count);
                    }

                    // TODO --  report read error?
                }
            } else {
                // Maybe we received a command
                char cmd = (char)status_byte;

#ifdef DO_UART_DEBUG
                debug_log("Command received: %c 0x%02X", cmd, status_byte);
#endif

                switch(cmd) {
                    case '1':   // SET BUS TO 100kHz
                        if (i2c_state.frequency != 100) {
                            i2c_state.frequency = 100;

                            // If the bus is active, reset it
                            if (i2c_state.is_ready) {
                                reset_i2c(&i2c_state);
                                i2c_state.is_started = false;
                            }
                        }
                        send_ack();
                        break;

                    case '4':   // SET BUS TO 400kHZ
                        if (i2c_state.frequency != 400) {
                            i2c_state.frequency = 400;

                            // If the bus is active, reset it
                            if (i2c_state.is_ready) {
                                reset_i2c(&i2c_state);
                                i2c_state.is_started = false;
                            }
                        }
                        send_ack();
                        break;

                    case '?':   // GET STATUS
                        send_status(&i2c_state);
                        break;

                    // FROM 1.1.1 -- change command from z to !
                    case 'z':
                    case '!':   // RESPOND TO CONNECTION REQUEST
                        tx("OK\r\n", 4);
                        break;

                    // FROM 1.1.0
                    case '*':   // SET LED STATE
                        do_use_led = rx_buffer[1] == 1 ? true : false;
#ifdef SHOW_HEARTBEAT
                        send_ack();
#else
                        send_err();
#endif
                        break;

                    // FROM 1.1.0              
                    case 'c':   // CONFIGURE THE BUS AND PINS
                        if (i2c_state.is_ready) {
                            send_err();
                            break;
                        }

                        uint8_t bus_index = rx_buffer[1] & 0x01;
                        uint8_t sda_pin = rx_buffer[2];
                        uint8_t scl_pin = rx_buffer[3];

                        // Check we have valid pin values for the device
                        if (!check_pins(bus_index, sda_pin, scl_pin)) {
                            send_err();
                            break;
                        }

                        // Store the values
                        i2c_state.bus = bus_index == 0 ? i2c0 : i2c1;
                        i2c_state.sda_pin = sda_pin;
                        i2c_state.scl_pin = scl_pin;
                        send_ack();
                        break;

                    case 'd':   // SCAN THE I2C BUS FOR DEVICES
                        if (!i2c_state.is_ready) init_i2c(&i2c_state);
                        send_scan(&i2c_state);
                        break;

                    case 'i':   // INITIALISE THE I2C BUS
                        // No need it initialise if we already have
                        if (!i2c_state.is_ready) init_i2c(&i2c_state);
                        send_ack();
                        break;

                    case 'p':   // SEND AN I2C STOP
                        if (i2c_state.is_ready && i2c_state.is_started) {
                            // Send no bytes and STOP
                            //uint8_t data = 0;
                            //i2c_write_timeout_us(i2c_state.bus, i2c_state.address, &data, 0, false, 1000);

                            // Reset state
                            i2c_state.is_started = false;
                            i2c_state.is_read_op = false;
                            send_ack();
                        } else {
                            send_err();
                        }
                        break;

                    case 's':   // START AN I2C TRANSACTION
                        if (i2c_state.is_ready) {
                            // Received data is in the form ['s', (address << 1) | op];
                            i2c_state.address = (rx_buffer[1] & 0xFE) >> 1;
                            i2c_state.is_read_op = ((rx_buffer[1] & 0x01) == 1);
                            i2c_state.is_started = true;

                            // Acknowledge
                            send_ack();
                        } else {
                            // Issue error
                            send_err();
                        }
                        break;

                    case 'x':   // RESET BUS
                        i2c_state.is_started = false;
                        reset_i2c(&i2c_state);
                        send_ack();
                        break;

                    /*
                     * GPIO COMMANDS
                     */
                    
                    // FROM 1.1.0                    
                    case 'g':   // SET DIGITAL OUT PIN
                        {
                            uint8_t read_value = 0;
                            uint8_t gpio_pin = (rx_ptr[1] & 0x1F);

                            // Make sure the pin's not in use for I2C
                            if (gpio_pin == i2c_state.sda_pin || gpio_pin == i2c_state.scl_pin) {
                                send_err();
                                break;
                            }

                            if (!set_gpio(&gpio_state, &read_value, rx_ptr)) {
                                send_err();
                                break;
                            }

                            bool is_read = ((rx_ptr[1] & 0x20) > 0);
                            putchar(is_read ? read_value : ACK);
                        }
                        break;
                        
                    default:    // UNKNOWN COMMAND -- FAIL
                        send_err();
                }
            }

            // Clear buffer and listen for input
            memset(rx_buffer, 0, RX_BUFFER_LENGTH_B);
        }

#ifdef SHOW_HEARTBEAT
        // Heartbeat LED blink for debugging
        if (do_use_led) {
            uint64_t now = time_us_64();
            if (now - last > HEARTBEAT_PERIOD_US) {
                led_set_state(true);
                is_on = true;
                last = now;

#ifdef DO_UART_DEBUG
                debug_log("LED ON");
#endif

            } else if ((now - last > HEARTBEAT_FLASH_US) && is_on) {
                led_set_state(false);
                is_on = false;

#ifdef DO_UART_DEBUG
                debug_log("LED OFF");
#endif

            }
        }
#endif

        // Pause? May not be necessary or might be bad
        sleep_ms(RX_LOOP_DELAY_MS);
    }

    // Should not get here, but just in case...
    // Signal an error on the host's LED
    led_set_colour(0xFF0000);
    led_on();

    // Fall out of the firmware at this point...
}


/**
 * @brief Initialise the host's I2C bus.
 *
 * @param frequency_khz: The bus speed in kHz.
 */
static void init_i2c(I2C_State* itr) {

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
static void reset_i2c(I2C_State* itr) {

    i2c_deinit(itr->bus);
    sleep_ms(10);
    i2c_init(itr->bus, itr->frequency * 1000);
}


/**
 * @brief Scan the host's I2C bus for devices, and send the results.
 */
static void send_scan(I2C_State* itr) {

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


/**
 * @brief Scan the host's I2C bus for devices, and send the results.
 *
 * @param t: A pointer to the current I2C transaction record.
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


/**
 * @brief Send a single-byte ACK.
 */
static inline void send_ack(void) {
#ifdef BUILD_FOR_TERMINAL_TESTING
    printf("ACK\r\n");
#else
    putchar(ACK);
#ifdef DO_UART_DEBUG
    debug_log("********** ACK **********");
#endif
#endif
}


/**
 * @brief Send a single-byte ERR.
 */
static inline void send_err(void) {
#ifdef BUILD_FOR_TERMINAL_TESTING
    printf("ERR\r\n");
#else
    putchar(ERR);
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
    while (data_count < RX_BUFFER_LENGTH_B) {
        c = getchar_timeout_us(1);
        if (c == PICO_ERROR_TIMEOUT) break;
        buffer[data_count++] = (uint8_t)c;
        sleep_ms(UART_LOOP_DELAY_MS);
    }

#ifdef DO_UART_DEBUG
    if (data_count > 0) debug_log("Bytes received: %i", data_count);
#endif
    return data_count;
}


/**
 * @brief Send a single transmitted block.
 *
 * @param buffer:     A pointer to the byte store buffer.
 * @param byte_count: The number of bytes to send.
 */
static void tx(uint8_t* buffer, uint32_t byte_count) {

    for (uint32_t i = 0 ; i < byte_count ; ++i) {
        putchar((buffer[i]));
        sleep_ms(UART_LOOP_DELAY_MS);
    }

    // stdio_flush();
}


/**
 * @brief Check that supplied SDA and SCL pins are valid for the
 *        board we're using
 *
 * @param bus: The Pico SDK I2C bus ID, 0 or 1.
 * @param sda: The GPIO number of SDA pin.
 * @param scl: The GPIO number of SCL pin.
 * 
 * @retval Whether the pins are good (`true`) or not (`false`).
 */
static bool check_pins(uint8_t bus, uint8_t sda, uint8_t scl) {
    
    // Same pin? Bail
    if (sda == scl) return false;

    // Select the right pin-pair array
    uint8_t* pin_pairs = bus == 0 ? &PIN_PAIRS_BUS_0[0] : &PIN_PAIRS_BUS_1[0];
    if (!pin_check(pin_pairs, sda)) return false;
    
    pin_pairs = bus == 0 ? &PIN_PAIRS_BUS_0[1] : &PIN_PAIRS_BUS_1[1];
    if (!pin_check(pin_pairs, scl)) return false;
    return true;
}


/**
 * @brief Check that a supplied pin are valid for the
 *        board we're using
 *
 * @param bus: The Pico SDK I2C bus ID, 0 or 1.
 * @param sda: The GPIO number of SDA pin.
 * @param scl: The GPIO number of SCL pin.
 * 
 * @retval Whether the pins are good (`true`) or not (`false`).
 */
static bool pin_check(uint8_t* pins, uint8_t pin) {

    uint8_t a_pin = *pins;
    while (a_pin != 255) {
        if (a_pin == pin) return true;
        pins += 2;;
        a_pin = *pins;
    }

    return false;
}

static void sig_handler(int signal) {

#ifdef DO_UART_DEBUG
    debug_log("Signal received: %i", signal);
#endif

}
