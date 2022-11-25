/*
 * Generic macOS I2C driver
 *
 * Version 1.1.2
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


#pragma mark - Static Function Prototypes

// FROM 1.1.1 -- implement internal functions as statics
static int          openSerialPort(const char *portname);
static size_t       readFromSerialPort(int fd, uint8_t* b, size_t s);
static bool         writeToSerialPort(int fd, const uint8_t* b, size_t s);
static inline void  print_bad_command_help(char* token);
static bool         board_set_led(I2CDriver *sd, bool is_on);
static inline void  send_command(I2CDriver *sd, char c);
static bool         i2c_ack(I2CDriver *sd);
static bool         i2c_set_bus(I2CDriver *sd, uint8_t bus_id, uint8_t sda_pin, uint8_t scl_pin);
static bool         i2c_set_speed(I2CDriver *sd, long speed);
static bool         i2c_reset(I2CDriver *sd);
static void         i2c_get_info(I2CDriver *sd, bool do_print);
static bool         gpio_set_pin(I2CDriver *sd, uint8_t pin);
static uint8_t      gpio_get_pin(I2CDriver *sd, uint8_t pin);


#pragma mark - Globals

// Retain the original port settings
static struct termios original_settings;


#pragma mark - Serial Port Control Functions

/**
 * @brief Open a Mac serial port.
 *`
 * @param device_file: The target port file, eg. `/dev/cu.usb-modem-10100`
 *
 * @retval The OS file descriptor, or -1 on error.
 */
static int openSerialPort(const char *device_file) {

    struct termios serial_settings;

#ifdef DEBUG
    fprintf(stderr, "Opening %s\n", device_file);
#endif
    
    // Open the device
    int fd = open(device_file, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        print_error("Could not open the device at %s - %s (%d)", device_file, strerror(errno), errno);
        return fd;
    }
    
    // Prevent additional opens except by root-owned processes
    if (ioctl(fd, TIOCEXCL) == -1) {
        print_error("Could not set TIOCEXCL on %s - %s (%d)", device_file, strerror(errno), errno);
        goto error;
    }

    // Get the port settings
    tcgetattr(fd, &original_settings);
    serial_settings = original_settings;
    
    // Calls to read() will return as soon as there is
    // at least one byte available or after 100ms
    // NOTE VTIME unit is 0.1s -- inter-byte timeout
    //      VMIN is read block duration in number of received chars
    cfmakeraw(&serial_settings);
    serial_settings.c_cc[VMIN] = 0;
    serial_settings.c_cc[VTIME] = 2;
    
#ifdef DEBUG
    fprintf(stderr, "Flushing the port\n");
#endif
    
#ifndef BUILD_FOR_LINUX
    if (tcsetattr(fd, TCSASOFT | TCSAFLUSH, &serial_settings) != 0) {
        print_error("Could not apply the port settings - %s (%d)", strerror(errno), errno);
        goto error;
    }
#else
    if (tcsetattr(fd, TCSAFLUSH, &serial_settings) != 0) {
        print_error("Could not apply the port settings - %s (%d)", strerror(errno), errno);
        goto error;
    }
#endif

#ifdef DEBUG
    fprintf(stderr, "Setting the port speed\n");
#endif
    
    // Set the port speed
    speed_t speed = (speed_t)115200;
#ifndef BUILD_FOR_LINUX
    if (ioctl(fd, IOSSIOSPEED, &speed) == -1) {
        print_error("Could not set port speed to 115200bps - %s (%d)", strerror(errno), errno);
        goto error;
    }
#else
    cfsetispeed(&serial_settings, speed);
    cfsetospeed(&serial_settings, speed);
#endif
    
#ifdef DEBUG
    fprintf(stderr, "Setting the port latency\n");
#endif
    
    // Set the latency -- MAY REMOVE IF NOT NEEDED
#ifndef BUILD_FOR_LINUX
    unsigned long lat_us = 2UL;
    if (ioctl(fd, IOSSDATALAT, &lat_us) == -1) {
        print_error("Could not set port latency - %s (%d)", strerror(errno), errno);
        goto error;
    }
#endif
    
    // Return the File Descriptor
    return fd;

error:
    close(fd);
    return -1;
}


/**
 * @brief Read bytes from the serial port FIFO.
 *
 * @param fd:         The port’s OS file descriptor.
 * @param buffer:     The buffer into which the read data will be written.
 * @param byte_count: The number of bytes to read, or 0 to scan for `\r\n`.
 *
 * @retval The number of bytes read.
 */
static size_t readFromSerialPort(int fd, uint8_t* buffer, size_t byte_count) {

    size_t count = 0;
    ssize_t number_read = -1;
    struct timespec now, then;
    clock_gettime(CLOCK_MONOTONIC_RAW, &then);

    if (byte_count == 0) {
        // Unknown number of bytes -- look for \r\n
        while(1) {
            number_read = read(fd, buffer + count, 1);
            if (number_read == -1) break;
            count += number_read;
            if (count > 2 && buffer[count - 2] == 0x0D && buffer[count - 1] == 0x0A) {
                // Backstep to clear the \r\n from the string
                buffer[count - 2] = '\0';
                buffer[count - 1] = '\0';
                count -= 2;
                break;
            }
            
            clock_gettime(CLOCK_MONOTONIC_RAW, &now);
            if (now.tv_sec - then.tv_sec > READ_BUS_HOST_TIMEOUT_S) {
                print_error("Read timeout: %i bytes read of %i", count, byte_count);
                return -1;
            }
        }
    } else {
        // Read a fixed, expected number of bytes
        while (count < byte_count) {
            // Read in the data a byte at a time
            number_read = read(fd, buffer + count, 1);
            if (number_read != -1) {
                count += number_read;
            }

            clock_gettime(CLOCK_MONOTONIC_RAW, &now);
            if (now.tv_sec - then.tv_sec > READ_BUS_HOST_TIMEOUT_S) {
                print_error("Read timeout: %i bytes read of %i", count, byte_count);
                return -1;
            }
        }
    }

#ifdef DEBUG
    // Output the read data for debugging
    fprintf(stderr, "  READ %d of %d: ", (int)count, (int)byte_count);
    for (int i = 0 ; i < count ; ++i) {
        fprintf(stderr, "%02X ", 0xFF & buffer[i]);
    }
    fprintf(stderr, "\n");
#endif

    return count;
}


/**
 * @brief Write bytes to the serial port FIFO.
 *
 * @param fd:         The port’s OS file descriptor.
 * @param buffer:     The buffer into which the read data will be written.
 * @param byte_count: The number of bytes to write`.
 *
 * @retval The number of bytes read.
 */
static bool writeToSerialPort(int fd, const uint8_t* buffer, size_t byte_count) {

    // Write the bytes
    ssize_t written = write(fd, buffer, byte_count);

#ifdef DEBUG
    // Output the read data for debugging
    if (written < 0) {
        fprintf(stderr, "write() returned an error: %li\n", written);
        fprintf(stderr, "Data written: %s\n", buffer);
    } else {
        fprintf(stderr, "WRITE %u: ", (int)byte_count);
        for (int i = 0 ; i < byte_count ; ++i) {
            fprintf(stderr, "%02X ", 0xFF & buffer[i]);
        }
        
        fprintf(stderr, "\n");
        
        if (written != byte_count) fprintf(stderr, "write() returned %li\n", written);
    }
#endif
    
    return (written == byte_count);
}


/**
 * @brief Flush the port FIFOs and close the port.
 */
void flush_and_close_port(int fd) {

    if (fd != -1) {
        // Drain the FIFOs -- alternative to `tcflush(fd, TCIOFLUSH)`;
        if (tcdrain(fd) == -1) {
            print_error("Could not flush the port. %s (%d).\n", strerror(errno), errno);
        }
        
        // Set the port back to how we found it
        if (tcsetattr(fd, TCSANOW, &original_settings) == -1) {
            print_error("Could not reset port - %s (%d)", strerror(errno), errno);
        }
        
        // Close the port
        close(fd);
        
#ifdef DEBUG
        fprintf(stderr, "Port closed\n");
#endif
    }
}


#pragma mark - I2C Driver Functions


/**
 * @brief Connect to the target I2C host.
 *        Always check the I2CDriver record that the `connected` field is `true`.
 *
 * @param fd:       The port’s OS file descriptor.
 * @param portname: The device path as a string.
 */
void i2c_connect(I2CDriver *sd, const char* portname) {

    // Mark that we're not connected
    sd->connected = false;

    // Open and get the serial port or bail
    sd->port = openSerialPort(portname);
    if (sd->port == -1) {
        print_error("Could not connect to port %s", portname);
        return;
    }
    
#ifdef DEBUG
    fprintf(stderr, "Port %s FD: %i\n", portname, sd->port);
#endif
    
    // Perform a basic communications check
    send_command(sd, 'z');
    uint8_t rx[4] = {0};
    size_t result = readFromSerialPort(sd->port, rx, 4);
    if (result == -1 || ((rx[0] != 'O') && (rx[1] != 'K'))) {
        print_error("Could not connect to device %s", portname);
        return;
    }

    // Got this far? We're good to go
    sd->connected = true;
}


/**
 * @brief Check for an ACK byte.
 *        Note that we use this for a write count on
 *        write operations.
 *
 * @param sd: Pointer to an I2CDriver structure.
 *
 * @retval The op was ACK'd (1) or not (0).
 */
static bool i2c_ack(I2CDriver *sd) {

    uint8_t read_buffer[1] = {0};
    if (readFromSerialPort(sd->port, read_buffer, 1) != 1) return false;
    bool ackd = ((read_buffer[0] & ACK) == ACK);
    
#ifdef DEBUG
    fprintf(stderr, "ACK\n");
#endif
    
    return ackd;
}


/**
 * @brief Get status info from the USB host.
 *
 * @param sd:       Pointer to an I2CDriver structure.
 * @param do_print: Should we output the results?
 */
static void i2c_get_info(I2CDriver *sd, bool do_print) {

    uint8_t read_buffer[HOST_INFO_BUFFER_MAX_B] = {0};
    send_command(sd, '?');
    size_t result = readFromSerialPort(sd->port, read_buffer, 0);
    if (result == -1) {
        print_error("Could not read I2C information from device");
        return;
    }

#ifdef DEBUG
    fprintf(stderr, "Received raw info string: %s\n", read_buffer);
#endif

    // Data string is, for example,
    // "1.1.100.110.1.1.0.200.A1B23C4D5E6F0A1B.QTPY-RP2040"
    int is_ready = 0;
    int has_started = 0;
    int frequency = 100;
    int address = 0xFF;
    int major = 0;
    int minor = 0;
    int patch = 0;
    int build = 0;
    int bus = 0;
    int sda_pin = -1;
    int scl_pin = -1;
    char string_data[67] = {0};
    char model[25] = {0};
    char pid[17] = {0};

    // Extract the data
    sscanf((char*)read_buffer, "%i.%i.%i.%i.%i.%i.%i.%i.%i.%i.%i.%s",
        &is_ready,
        &has_started,
        &bus,
        &sda_pin,
        &scl_pin,
        &frequency,
        &address,
        &major,
        &minor,
        &patch,
        &build,
        string_data
    );

    // Store certain values in the I2C driver record
    // NOTE This involves separately extracting the substrings
    //      from the read `string_data` as sscanf() doesn't
    //      separate them properly
    strncpy(pid, string_data, 16);
    strcpy(model, &string_data[17]);
    sd->speed = frequency;

    if (do_print) {
        fprintf(stderr, "   I2C host device: %s\n", model);
        fprintf(stderr, "  I2C host version: %i.%i.%i (%i)\n", major, minor, patch, build);
        fprintf(stderr, "       I2C host ID: %s\n", pid);
        fprintf(stderr, "     Using I2C bus: %s\n", bus == 0 ? "i2c0" : "i2c1");
        fprintf(stderr, " I2C bus frequency: %ikHz\n", frequency);
        fprintf(stderr, " Pins used for I2C: GP%i (SDA), GP%i (SCL)\n", sda_pin, scl_pin);
        fprintf(stderr, "    I2C is enabled: %s\n", is_ready == 1 ? "YES" : "NO");
        fprintf(stderr, "     I2C is active: %s\n", has_started == 1 ? "YES" : "NO");

        // Check for a 'no device' I2C address
        if (address == 0xFF) {
            fprintf(stderr, "Target I2C address: NONE\n");
        } else {
            fprintf(stderr, "Target I2C address: 0x%02X\n", address);
        }

    }
}


/**
 * @brief Scan the I2C bus and list devices.
 *
 * @param sd: Pointer to an I2CDriver structure.
 */
void i2c_scan(I2CDriver *sd) {

    char scan_buffer[SCAN_BUFFER_MAX_B] = {0};
    uint8_t device_list[CONNECTED_DEVICES_MAX_B] = {0};
    uint32_t device_count = 0;

    // Request scan from bus host
    send_command(sd, 'd');
    size_t result = readFromSerialPort(sd->port, (uint8_t*)scan_buffer, 0);
    if (result == -1) {
        print_error("Could not read scan data from device");
        return;
    }

    // If we receive Z(ero), there are no connected devices
    if (scan_buffer[0] != 'Z') {
        // Extract device address hex strings and generate
        // integer values. For example:
        // source = "12.71.A0."
        // dest   = [18, 113, 160]

#ifdef DEBUG
        fprintf(stderr, "Buffer: %lu bytes, %lu items\n", strlen(scan_buffer), strlen(scan_buffer) / 3);
#endif

        for (uint32_t i = 0 ; i < strlen(scan_buffer) ; i += 3) {

            uint8_t value[2] = {0};
            uint32_t count = 0;
            
            // Get two hex chars and store in 'value'; break on a .
            while(1) {
                uint8_t token = scan_buffer[i + count];
                if (token == 0x2E) break;
                value[count] = token;
                count++;
            }

            device_list[device_count] = (uint8_t)strtol((char *)value, NULL, 16);
            device_count++;
        }
    }

    // Output the device list as a table (even with no devices)

    fprintf(stderr, "   0 1 2 3 4 5 6 7 8 9 A B C D E F");

    for (int i = 0 ; i < 0x80 ; i++) {
        if (i % 16 == 0) fprintf(stderr, "\n%02x ", i);
        if (i < 8 || i > 0x77) {
            fprintf(stderr, "  ");
        } else {
            bool found = false;

            if (device_count > 0) {
                for (int j = 0 ; j < 120 ; j++) {
                    if (device_list[j] == i) {
                        fprintf(stderr, "@ ");
                        found = true;
                        break;
                    }
                }
            }

            if (!found) fprintf(stderr, ". ");
        }
    }

    fprintf(stderr, "\n");
}


/**
 * @brief Tell the I2C host to Initialise the I2C bus.
 *
 * @param sd: Pointer to an I2CDriver structure.
 *
 * @retval Whether the command was ACK'd (`true`) or not (`false`).
 */
bool i2c_init(I2CDriver *sd) {

    send_command(sd, 'i');
    return i2c_ack(sd);
};


/**
 * @brief Tell the I2C host to set the bus speed.
 *
 * @param sd:    Pointer to an I2CDriver structure.
 * @param speed: Bus frequency in 100kHz. Only 1 or 4 supported.
 *
 * @retval Whether the command was ACK'd (`true`) or not (`false`).
 */
static bool i2c_set_speed(I2CDriver *sd, long speed) {

    switch(speed) {
        case 1:
            send_command(sd, '1');
            break;
        default:
            send_command(sd, '4');
    }

    return i2c_ack(sd);
}


/**
 * @brief Choose the I2C host's target bus: 0 (i2c0) or 1 (i2c1).
 *
 * @param sd:      Pointer to an I2CDriver structure.
 * @param bus_id:  The Pico SDK I2C bus ID: 0 or 1.
 * @param sda_pin: The SDA pin GPIO number.
 * @param scl_pin: The SCL pin GPIO number.
 *
 * @retval Whether the command was ACK'd (`true`) or not (`false`).
 */
static bool i2c_set_bus(I2CDriver *sd, uint8_t bus_id, uint8_t sda_pin, uint8_t scl_pin) {
    
    if (bus_id < 0 || bus_id > 1) return false;
    uint8_t set_bus_data[4] = {'c', (bus_id & 0x01), sda_pin, scl_pin};
    writeToSerialPort(sd->port, set_bus_data, sizeof(set_bus_data));
    return i2c_ack(sd);
}


/**
 * @brief Tell the I2C host to reset the I2C bus.
 *
 * @param sd: Pointer to an I2CDriver structure.
 *
 * @retval Whether the command was ACK'd (`true`) or not (`false`).
 */
static bool i2c_reset(I2CDriver *sd) {

    send_command(sd, 'x');
    return i2c_ack(sd);
}


/**
 * @brief Tell the I2C host to start an I2C transaction.
 *
 * @param sd:      Pointer to an I2CDriver structure.
 * @param address: The target device's I2C address.
 * @param op:      Read (0) or write (1) I2C operation.
 *
 * @retval Whether the command was ACK'd (`true`) or not (`false`).
 */
bool i2c_start(I2CDriver *sd, uint8_t address, uint8_t op) {

    // This is a two-byte command: command + (address | op)
    uint8_t start_data[2] = {'s', ((address << 1) | op)};
    writeToSerialPort(sd->port, start_data, sizeof(start_data));
    return i2c_ack(sd);
}


/**
 * @brief Tell the I2C host to issue a STOP to the I2C bus.
 *
 * @param sd: Pointer to an I2CDriver structure.
 */
bool i2c_stop(I2CDriver *sd) {

    send_command(sd, 'p');
    return i2c_ack(sd);
}


/**
 * @brief Write data to the I2C host for transmission.
 *
 * @param sd:         Pointer to an I2CDriver structure.
 * @param bytes:      The bytes to write.
 * @param byte_count: The number of bytes to write.
 *
 * @retval The number of bytes received.
 */
size_t i2c_write(I2CDriver *sd, const uint8_t bytes[], size_t byte_count) {

    // Count the bytes sent
    int count = 0;
    bool ack = false;

    // Write the data out in blocks of 64 bytes
    for (size_t i = 0 ; i < byte_count ; i += 64) {
        // Calculate the data length for the prefix byte
        size_t length = ((byte_count - i) < 64) ? (byte_count - i) : 64;
        uint8_t write_cmd[65] = {(uint8_t)(PREFIX_BYTE_WRITE + length - 1)};

        // Write a block of bytes to the send buffer
        memcpy(write_cmd + 1, bytes + i, length);

        // Write out the block -- use ACK as byte count
        writeToSerialPort(sd->port, write_cmd, 1 + length);
        ack = i2c_ack(sd);
        if (!ack) break;
        count += length;
    }

    return count;
}


/**
 * @brief Read data from the I2C host.
 *
 * @param sd:         Pointer to an I2CDriver structure.
 * @param bytes:      A buffer for the bytes to read.
 * @param byte_count: The number of bytes to write.
 */
void i2c_read(I2CDriver *sd, uint8_t bytes[], size_t byte_count) {

    for (size_t i = 0 ; i < byte_count ; i += 64) {
        // Calculate data length for prefix byte
        size_t length = ((byte_count - i) < 64) ? (byte_count - i) : 64;
        uint8_t read_cmd[1] = {(uint8_t)(PREFIX_BYTE_READ + length - 1)};

        writeToSerialPort(sd->port, read_cmd, 1);
        size_t result = readFromSerialPort(sd->port, bytes + i, length);
        if (result == -1) {
            print_error("Could not read back from device");
        } else {
            for (size_t i = 0 ; i < result ; ++i) {
                fprintf(stdout, "%02X", bytes[i]);
            }

            fprintf(stdout, "\n");
        }
    }
}


#pragma mark - GPIO Functions

/**
 * @brief Set a GPIO pin.
 *
 * @param sd:  Pointer to an I2CDriver structure.
 * @param pin: The state, direction and number of the target GPIO.
 *
 * @retval Was the command ACK'd (`true`) or not (`false`).
 */
static bool gpio_set_pin(I2CDriver *sd, uint8_t pin) {
    
    uint8_t set_pin_data[2] = {'g', pin};
    writeToSerialPort(sd->port, set_pin_data, sizeof(set_pin_data));
    return i2c_ack(sd);
}


/**
 * @brief Read a GPIO pin
 *
 * @param sd:  Pointer to an I2CDriver structure.
 * @param pin: The state, direction and number of the target GPIO.
 *
 * @retval Was the command ACK'd (`true`) or not (`false`).
 */
static uint8_t gpio_get_pin(I2CDriver *sd, uint8_t pin) {
    
    uint8_t set_pin_data[2] = {'g', pin};
    writeToSerialPort(sd->port, set_pin_data, sizeof(set_pin_data));
    uint8_t pin_read = 0;
    i2c_read(sd, &pin_read, 1);
    return pin_read;
}


#pragma mark - Board Control Functions

static bool board_set_led(I2CDriver *sd, bool is_on) {
    
    uint8_t set_led_data[2] = {'*', (is_on ? 1 : 0)};
    writeToSerialPort(sd->port, set_led_data, sizeof(set_led_data));
    return i2c_ack(sd);
}


/**
 * @brief Write a single-byte command to the serial port.
 *
 * @param sd: Pointer to an I2CDriver structure.
 * @param c:  A command character.
 */
static inline void send_command(I2CDriver* sd, char c) {

    writeToSerialPort(sd->port, (uint8_t*)&c, 1);
}


#pragma mark - User Feedback Functions


/**
 * @brief Output help info on receipt of a bad command.
 *
 * @param command: The bad command.
 */
static inline void print_bad_command_help(char* command) {

    print_error("Bad command: %s\n", command);
}


#pragma mark - Command Parsing and Processing

/**
 * @brief Parse driver commands.
 *
 * @param sd:   Pointer to an I2CDriver structure.
 * @param argc: The max number of args to process.
 * @param argv: The args.
 *
 * @retval The driver exit code, 0 on success, 1 on failure.
 */
int process_commands(I2CDriver *sd, int argc, char *argv[], uint32_t delta) {

    // Set a 10ms period for intra-command delay period
    struct timespec pause;
    pause.tv_sec = 0.010;
    pause.tv_nsec = 0.010 * 1000000;

    // Process args one by one
    for (int i = delta ; i < argc ; i++) {
        char* command = argv[i];

#ifdef DEBUG
        fprintf(stderr, "Command: %s\n", command);
#endif

        // Commands should be single characters
        if (strlen(command) != 1) {
            // FROM 1.1.0 -- Allow for commands with a - prefix
            if (command[0] == '-') {
                command++;
            } else {
                print_bad_command_help(command);
                return EXIT_ERR;
            }
        }

        switch (command[0]) {
            case 'C':
            case 'c':   // CHOOSE I2C BUS AND (FROM 1.1.0) PINS
                {
                    if (i < argc - 1) {
                        char* token = argv[++i];
                        long bus_id = strtol(token, NULL, 0);
                        
                        if (i < argc - 1) {
                            token = argv[++i];
                            long sda_pin = strtol(token, NULL, 0);
                            
                            if (i < argc - 1) {
                                token = argv[++i];
                                long scl_pin = strtol(token, NULL, 0);
                                
                                // Make sure we have broadly valid pin numbers
                                if (sda_pin < 0 || sda_pin > 32 ||
                                    scl_pin < 0 || scl_pin > 32 ||
                                    sda_pin == scl_pin) {
                                    print_error("Unsupported pin value(s) specified");
                                    return EXIT_ERR;
                                }
                                
                                if (bus_id != 1 && bus_id != 0) {
                                    print_warning("Incorrect I2C bus ID selected. Should be 0 or 1");
                                    bus_id = 0;
                                }
                                
#if DEBUG
                                printf("BUS %li, SDA %li, SCL %li\n", bus_id, sda_pin, scl_pin);
#endif
                                
                                bool result = i2c_set_bus(sd, (uint8_t)bus_id, (uint8_t)sda_pin, (uint8_t)scl_pin);
                                if (!result) print_warning("I2C bus config un-ACK’d");
                                break;
                            }
                        }
                    }

                    print_error("Incomplete I2C setup data given");
                    return EXIT_ERR;
                }
                
            case 'F':
            case 'f':   // SET THE BUS FREQUENCY
                {
                    if (i < argc - 1) {
                        char* token = argv[++i];
                        long speed = strtol(token, NULL, 0);

                        if (speed == 1 || speed == 4) {
                            bool result = i2c_set_speed(sd, speed);
                            if (!result) print_warning("Frequency set un-ACK’d");
                        } else {
                            print_warning("Incorrect I2C frequency selected. Should be 1(00kHz) or 4(00kHz)");
                        }

                        break;
                    }

                    print_error("No frequency value given");
                    return EXIT_ERR;
                }
            
            case 'G':   // FROM 1.1.0
            case 'g':   // SET OR GET A GPIO PIN
                {
                    if (i < argc - 1) {
                        char* token = argv[++i];
                        long pin_number = strtol(token, NULL, 0);
                        
                        if (pin_number < 0 || pin_number > 31) {
                            print_error("Pin out of range (0-31");
                            return EXIT_ERR;
                        }
                        
                        if (i < argc - 1) {
                            token = argv[++i];
                            // Is this a read op?
                            bool do_read   = (token[0] == 'r' || token[0] == 'R');
                            
                            // Is it a state change?
                            bool pin_state = (token[0] == '1');
                            bool want_high = (strncasecmp(token, "hi", 2) == 0);
                            bool want_low  = (strncasecmp(token, "lo", 2) == 0);
                            if (want_high || want_low) pin_state = want_high || !want_low;
                            
                            // Pin direction is optional
                            bool pin_direction = true;
                            if (i < argc - 1) {
                                token = argv[++i];
                                if (token[0] == '0' || token[0] == '1') {
                                    pin_direction = (token[0] == '1');
                                } else if (token[0] == 'i' || token[0] == 'o') {
                                    bool dir_in  = (strcasecmp(token, "in") == 0);
                                    bool dir_out = (strcasecmp(token, "out") == 0);
                                    if (dir_in || dir_out) pin_direction = dir_out || !dir_in;
                                } else {
                                    i -= 1;
                                }
                            }

                            // Encode the TX data:
                            // Bit 7 6 5 4 3 2 1 0
                            //     | | | |_______|________ Pin number 0-31
                            //     | | |__________________ Read flag (1 = read op)
                            //     | |____________________ Direction bit (1 = out, 0 = in)
                            //     |______________________ State bit (1 = HIGH, 0 = LOW)
                            
                            uint8_t send_byte = (uint8_t)pin_number;
                            send_byte &= 0x1F;
                            if (pin_state) send_byte |= 0x80;
                            if (pin_direction) send_byte |= 0x40;
                            if (do_read) send_byte |= 0x20;
                            
                            if (do_read) {
                                // Read back the pin value
                                uint8_t result = gpio_get_pin(sd, send_byte);
                                
                                // Issue value to STDOUT
                                fprintf(stdout, "%02X\n", ((result & 0x80) >> 7));
                                
                                // Check we got the same pin back that we asked for
                                if ((result & 0x1F) != pin_number) print_warning("GPIO pin set un-ACK’d");
                            } else {
                                // Set the pin and wait for ACK
                                bool result = gpio_set_pin(sd, send_byte);
                                if (!result) print_warning("GPIO pin set un-ACK’d");
                            }
                            break;
                        }
                        
                        print_error("No state value given");
                        return EXIT_ERR;
                    }
                    
                    print_error("No pin value given");
                    return EXIT_ERR;
                }
        
            case 'I':
            case 'i':   // PRINT HOST STATUS INFO
                i2c_get_info(sd, true);
                break;

            // FROM 1.1.0
            case 'L':
            case 'l':   // SET THE BOARD LED
                {
                    // Get the address if we can
                    if (i < argc - 1) {
                        char* token = argv[++i];
                        bool is_on = (strcasecmp(token, "on") == 0);
                        if (is_on || strcasecmp(token, "off") == 0 ) {
                            bool result = board_set_led(sd, is_on);
                            if (!result) print_warning("LED set un-ACK'd");
                            break;
                        }

                        print_error("Invalid LED state give");
                        return EXIT_ERR;
                    }
                    
                    print_error("No LED state given");
                    return EXIT_ERR;
                }
        
            case 'P':
            case 'p':   // ISSUE AN I2C STOP
                i2c_stop(sd);
                break;

            case 'R':
            case 'r':   // READ FROM THE I2C BUS
                {
                    // Get the address if we can
                    if (i < argc - 1) {
                        char* token = argv[++i];
                        long address = strtol(token, NULL, 0);

                        // Get the number of bytes if we can
                        if (i < argc - 1) {
                            token = argv[++i];
                            size_t num_bytes = strtol(token, NULL, 0);
                            uint8_t bytes[8192];

                            i2c_start(sd, address, 1);
                            i2c_read(sd, bytes, num_bytes);
                            i2c_stop(sd);
                            break;
                        } else {
                            print_error("No I2C address given");
                        }
                    } else {
                        print_error("No I2C address given");
                    }

                    return EXIT_ERR;
                }

            case 'S':
            case 's':   // LIST DEVICES ON BUS
                i2c_scan(sd);
                break;

            case 'W':
            case 'w':   // WRITE TO THE I2C BUS
                {
                    // Get the address if we can
                    if (i < argc - 1) {
                        char* token = argv[++i];
                        long address = strtol(token, NULL, 0);

                        // Get the bytes to write if we can
                        if (i < argc - 1) {
                            token = argv[++i];
                            size_t num_bytes = 0;
                            uint8_t bytes[8192];
                            char* endptr = token;

                            while (num_bytes < sizeof(bytes)) {
                                bytes[num_bytes++] = (uint8_t)strtol(endptr, &endptr, 0);
                                if (*endptr == '\0') break;
                                if (*endptr != ',') {
                                    print_error("Invalid bytes: %s\n", token);
                                    return EXIT_ERR;
                                }

                                endptr++;
                            }

                            i2c_start(sd, (uint8_t)address, 0);
                            i2c_write(sd, bytes, num_bytes);
                            break;
                        } else {
                            print_error("No I2C address given");
                        }
                    } else {
                        print_error("No I2C address given");
                    }

                    return EXIT_ERR;
                }

            case 'X':
            case 'x':   // RESET BUS
                i2c_reset(sd);
                break;
                
            case 'Z':
            case 'z':   // INITIALISE BUS
                // Initialize the I2C host's I2C bus
                if (!(i2c_init(sd))) {
                    print_error("Could not initialise I2C");
                    flush_and_close_port(sd->port);
                    return EXIT_ERR;
                }

                break;
                
            default:    // NO COMMAND/UNKNOWN COMMAND
                print_bad_command_help(command);
                return EXIT_ERR;
        }

        // Pause for the UART's breath
        nanosleep(&pause, &pause);
    }

    return 0;
}
