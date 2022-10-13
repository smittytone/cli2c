/*
 * Generic macOS I2C driver
 *
 * Version 0.1.5
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


// Retain the original port settings
static struct termios original_settings;


#pragma mark - Serial Port Functions

/**
 * @brief Open a Mac serial port.
 *`
 * @param device_file: The target port file, eg. `/dev/cu.usb-modem-10100`
 *
 * @retval The OS file descriptor, or -1 on error.
 */
int openSerialPort(const char *device_file) {
    
    struct termios serial_settings;
    
    // Open the device
    int fd = open(device_file, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        print_error("Could not open the device at %s - %s (%d)", device_file, strerror(errno), errno);
        return fd;
    }
    
    // Prevent additional opens except by root-owned processes
    if (ioctl(fd, TIOCEXCL) == -1) {
        print_error("Could not set TIOCEXCL on %s - %s (%d)", device_file, strerror(errno), errno);
        goto error;;
    }

    // Get the port settings
    tcgetattr(fd, &original_settings);
    serial_settings = original_settings;
    
    // Calls to read() will return as soon as there is
    // at least one byte available or after 100ms.
    cfmakeraw(&serial_settings);
    serial_settings.c_cc[VMIN] = 0;
    serial_settings.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &serial_settings) != 0) {
        print_error("Could not apply the port settings - %s (%d)", strerror(errno), errno);
        goto error;
    }

    // Set the port speed
    speed_t speed = (speed_t)115200;
    if (ioctl(fd, IOSSIOSPEED, &speed) == -1) {
        print_error("Could not set port speed to 115200bps - %s (%d)", strerror(errno), errno);
        goto error;
    }
    
    // Set the latency -- MAY REMOVE IF NOT NEEDED
    unsigned long lat_us = 1UL;
    if (ioctl(fd, IOSSDATALAT, &lat_us) == -1) {
        print_error("Could not set port latency - %s (%d)", strerror(errno), errno);
        goto error;
    }
    
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
size_t readFromSerialPort(int fd, uint8_t* buffer, size_t byte_count) {

    size_t count = 0;
    ssize_t number_read = -1;
    
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
        }
    } else {
        // Read a fixed, expected number of bytes
        struct timespec now, then;
        clock_gettime(CLOCK_MONOTONIC_RAW, &then);
        
        while (count < byte_count) {
            // Read in the data a byte at a time
            number_read = read(fd, buffer + count, 1);
            if (number_read != -1) {
                count += number_read;
            }
            
            clock_gettime(CLOCK_MONOTONIC_RAW, &now);
            if (now.tv_sec - then.tv_sec > 15) {
                print_error("Read timeout: %i bytes read of %i", count, byte_count);
                return -1;
            }
        }
    }

#ifdef DEBUG
    // Output the read data for debugging
    printf("  READ %d of %d: ", (int)count, (int)byte_count);
    for (int i = 0 ; i < count ; ++i) {
        printf("%02X ", 0xFF & buffer[i]);
    }
    printf("\n");
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
void writeToSerialPort(int fd, const uint8_t* buffer, size_t byte_count) {
    
    // Write the bytes
    write(fd, buffer, byte_count);

#ifdef DEBUG
    // Output the read data for debugging
    printf("WRITE %u: ", (int)byte_count);
    for (int i = 0 ; i < byte_count ; ++i) {
        printf("%02X ", 0xFF & buffer[i]);
    }
    printf("\n");
#endif
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
    if (sd->port == -1) return;

    // Perform a basic communications check
    send_command(sd, 'z');
    uint8_t rx[4] = {0};
    size_t result = readFromSerialPort(sd->port, rx, 4);
    if (result == -1 || ((rx[0] != 'O') && (rx[1] != 'K'))) {
        print_error("Could not read from device");
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
    if (readFromSerialPort(sd->port, read_buffer, 1) != 1) {
        print_error("Last action not ACK’d by device");
        return false;
    }
    
    return ((read_buffer[0] & 0x01) == 0x01);
}


/**
 * @brief Get status info from the USB host.
 *
 * @param sd:       Pointer to an I2CDriver structure.
 * @param do_print: Should we output the results?
 */
void i2c_get_info(I2CDriver *sd, bool do_print) {

    uint8_t read_buffer[HOST_INFO_BUFFER_MAX] = {0};
    send_command(sd, '?');
    size_t result = readFromSerialPort(sd->port, read_buffer, 0);
    if (result == -1) {
        print_error("Could not read from device");
        return;
    }

#ifdef DEBUG
    printf("Received raw info string: %s\n", read_buffer);
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
    char string_data[34] = {0};
    char model[17] = {0};
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
    // NOTE This involves seaprately extracting the substrings
    //      from the read `string_data` as sscanf() doesn't
    //      separate them properly
    strncpy(pid, string_data, 16);
    strcpy(model, &string_data[17]);
    sd->speed = frequency;

    if (do_print) {
        printf("   I2C host device: %s\n", model);
        printf("  I2C host version: %i.%i.%i (%i)\n", major, minor, patch, build);
        printf("       I2C host ID: %s\n", pid);
        printf("     Using I2C bus: %s\n", bus == 0 ? "i2c0" : "i2c1");
        printf(" I2C bus frequency: %ikHz\n", frequency);
        printf(" Pins used for I2C: GP%i (SDA), GP%i (SCL)\n", sda_pin, scl_pin);
        printf("    I2C is enabled: %s\n", is_ready == 1 ? "YES" : "NO");
        printf("     I2C is active: %s\n", has_started == 1 ? "YES" : "NO");
        
        // Check for a 'no device' I2C address
        if (address == 0xFF) {
            printf("Target I2C address: NONE\n");
        } else {
            printf("Target I2C address: 0x%02X\n", address > 1);
        }
        
    }
}


/**
 * @brief Scan the I2C bus and list devices.
 *
 * @param sd: Pointer to an I2CDriver structure.
 */
void i2c_scan(I2CDriver *sd) {

    char scan_buffer[1024] = {0};
    uint8_t device_list[120] = {0};
    uint32_t device_count = 0;
    
    send_command(sd, 'd');
    size_t result = readFromSerialPort(sd->port, (uint8_t*)scan_buffer, 0);
    if (result == -1) {
        print_error("Could not read from device");
        return;
    }
    
    if (scan_buffer[0] != 'Z') {
        // Extract device address hex strings and generate
        // integer values. For example:
        // source = "12.71.A0."
        // dest   = [18, 113, 160]
        
#ifdef DEBUG
        printf("Buffer: %lu bytes, %lu items\n", strlen(scan_buffer), strlen(scan_buffer) / 3);
#endif
        
        for (uint32_t i = 0 ; i < strlen(scan_buffer) ; i += 3) {
            
            uint8_t value[2] = {0};
            uint32_t count = 0;
            
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
    
    printf("   0 1 2 3 4 5 6 7 8 9 A B C D E F");
    
    for (int i = 0 ; i < 0x80 ; i++) {
        if (i % 16 == 0) printf("\n%02x ", i);
        if (i < 8 || i > 0x77) {
            printf("  ");
        } else {
            bool found = false;

            if (device_count > 0) {
                for (int j = 0 ; j < 120 ; j++) {
                    if (device_list[j] == i) {
                        printf("@ ");
                        found = true;
                        break;
                    }
                }
            }

            if (!found) printf(". ");
        }
    }

    printf("\n");
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
bool i2c_set_speed(I2CDriver *sd, long speed) {
    
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
 * @brief Tell the I2C host to reset the I2C bus.
 *
 * @param sd: Pointer to an I2CDriver structure.
 *
 * @retval Whether the command was ACK'd (`true`) or not (`false`).
 */
bool i2c_reset(I2CDriver *sd) {

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
        }
    }
}


/**
 * @brief Parse driver commands.
 *
 * @param sd:   Pointer to an I2CDriver structure.
 * @param argc: The max number of args to process.
 * @param argv: The args.
 *
 * @retval The driver exit code, 0 on success, 1 on failure.
 */
int i2c_commands(I2CDriver *sd, int argc, char *argv[], uint32_t delta) {
    
    // Set a 10ms period for intra-command delay period
    struct timespec pause;
    pause.tv_sec = 0.010;
    pause.tv_nsec = 0.010 * 1000000;
    
    // Process args one by one
    for (int i = delta ; i < argc ; i++) {
        char* command = argv[i];

#ifdef DEBUG
        printf("Command: %s\n", command);
#endif

        // Commands should be single characters
        if (strlen(command) != 1) {
            // TODO Check for a flag mark, `-`
            print_bad_command_help(command);
            return EXIT_ERR;
        }

        switch (command[0]) {
            case 'f':   // SET THE BUS FREQUENCY
                {
                    if (i < argc - 1) {
                        char* token = argv[++i];
                        long speed = strtol(token, NULL, 0);
                        
                        if (speed == 1 || speed == 4) {
                            bool result = i2c_set_speed(sd, speed);
                            if (!result) print_warning("Command f un-ACK’d");
                        } else {
                            print_warning("Incorrect I2C frequency selected. Should be 1(00kHz) or 4(00kHz)");
                        }
                        
                        break;
                    }
                    
                    print_error("No frequency value given");
                    return EXIT_ERR;
                }

            case 'i':   // PRINT HOST STATUS INFO
                i2c_get_info(sd, true);
                break;

            case 'p':   // ISSUE AN I2C STOP
                i2c_stop(sd);
                break;

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

            case 's':   // LIST DEVICES ON BUS
                i2c_scan(sd);
                break;

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

            case 'x':   // RESET BUS
                i2c_reset(sd);
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


#pragma mark - Misc. Functions

/**
 * @brief Write a single-byte command to the serial port.
 *
 * @param sd: Pointer to an I2CDriver structure.
 * @param c:  A command character.
 */
static void send_command(I2CDriver* sd, char c) {
    
    writeToSerialPort(sd->port, (uint8_t*)&c, 1);
}


/**
 * @brief Output help info on receipt of a bad command.
 *
 * @param command: The bad command.
 */
static void print_bad_command_help(char* command) {
    
    print_error("Bad command: %s\n", command);
    show_commands();
}


/**
 * @brief Output help info.
 */
void show_commands(void) {
    printf("Commands:\n");
    printf("  w {address} {bytes} Write bytes out to I2C.\n");
    printf("  r {address} {count} Read count bytes in from I2C.\n");
    printf("                      Issues a STOP after all the bytes have been read.\n");
    printf("  p                   Issue an I2C STOP.\n");
    printf("  f {frequency}       Set the I2C bus frequency in multiples of 100kHz.\n");
    printf("                      Only 1 and 4 are supported.\n");
    printf("  x                   Reset the I2C bus.\n");
    printf("  s                   Scan for devices on the I2C bus.\n");
    printf("  i                   Get I2C bus host device information.\n");
    printf("  h                   Show help and quit.\n");
}


/**
 * @brief Flush the port FIFOs and close the port.
 */
void flush_and_close_port(int fd) {
    
    // Drain the FIFOs -- alterntive to `tcflush(fd, TCIOFLUSH)`;
    if (tcdrain(fd) == -1) {
        print_error("Could not flush the port. %s (%d).\n", strerror(errno), errno);
    }
    
    // Set the port back to how we found it
    if (tcsetattr(fd, TCSANOW, &original_settings) == -1) {
        print_error("Coould not reset port - %s (%d)", strerror(errno), errno);
    }

    // Close the port
    close(fd);
}
