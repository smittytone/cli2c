/**
 *
 * I2C driver
 * Version 1.0.0
 * Copyright © 2022 Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


#pragma mark - Serial Port Functions

/**
 * @brief Open a Mac serial port.
 *
 * @param device_file The target port file, eg. /dev/cu.usb-modem-10100
 *
 * @retval The OS file descriptor.
 */
int openSerialPort(const char *device_file) {
    struct termios serial_settings;

    int fd = open(device_file, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        print_error("Could not open %s", device_file);
        return -1;
    }

    tcgetattr(fd, &serial_settings);
    cfmakeraw(&serial_settings);
    serial_settings.c_cc[VMIN] = 1;
    // serial_settings.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &serial_settings) != 0) {
        print_error("Could not get settings from port");
        return -1;
    }

    // Prevent additional opens except by root-owned processes
    if (ioctl(fd, TIOCEXCL) == -1) {
        print_error("Could not set TIOCEXCL on %s", device_file);
        return -1;
    }

    // Set port speed
    speed_t speed = (speed_t)1000000;
    if (ioctl(fd, IOSSIOSPEED, &speed) == -1) {
        print_error("Could not set port speed to 1Mbps");
        return -1;
    }

    // Return the File Descriptor
    return fd;
}


size_t readFromSerialPort(int fd, uint8_t* buffer, size_t byte_count) {
    size_t total_read = 0;
    ssize_t number_read = -1;

    if (byte_count == 0) {
        // Unknown number of bytes -- look for \r\n
        printf("**** READING ****\n");
        uint8_t* buff_ptr = buffer;
        size_t count = 0;
        while(1) {
            number_read = read(fd, buffer + total_read, 1);
            if (number_read == -1) break;
            buff_ptr += number_read;
            count += number_read;
            printf("*** %li -> %02x %02x", number_read, *(buff_ptr - 1), *(buff_ptr - 2));
            if (count > 2 && *(buff_ptr - 2) == 0x0D && *(buff_ptr - 1) == 0x0A) break;
        }
    } else {
        while (total_read < byte_count) {
            number_read = read(fd, buffer + total_read, byte_count);
            if (number_read > 0) total_read += number_read;
        }
    }

#ifdef VERBOSE
    printf(" READ %d %d: ", (int)byte_count, (int)number_read);
    for (int i = 0 ; i < byte_count ; ++i) {
        printf("%02X ", 0xFF & buffer[i]);
    }
    printf("\n");
#endif

    return total_read;
}


void writeToSerialPort(int fd, const uint8_t* buffer, size_t byte_count) {
    write(fd, buffer, byte_count);

#ifdef VERBOSE
    printf("WRITE %u: ", (int)byte_count);
    for (int i = 0 ; i < byte_count ; ++i) {
        printf("%02X ", 0xFF & buffer[i]);
    }
    printf("\n");
#endif
}


#pragma mark - I2C Driver Functions

void i2c_connect(I2CDriver *sd, const char* portname) {
    // Mark that we're not connected
    sd->connected = false;

    // Open and get the serial port or bail
    sd->port = openSerialPort(portname);
    if (sd->port == -1) return;

    /* Clear the FIFO?
    writeToSerialPort(sd->port, (uint8_t*)"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@", 64);

    // Test the port
    const uint8_t tests[] = "A\r\n\0xff";
    for (int i = 0 ; i < 4 ; ++i) {
        uint8_t tx[2] = {'e', tests[i]};
        writeToSerialPort(sd->port, tx, 2);

        uint8_t rx[1];
        size_t n = readFromSerialPort(sd->port, rx, 1);
        if ((n != 1) || (rx[0] != tests[i])) return;
    }
     */

    // Basic comms check
    send_command(sd, 'z');
    uint8_t rx[4] = {0};
    size_t n = readFromSerialPort(sd->port, rx, 4);
    if ((n != 4) || ((rx[0] != 'O') && (rx[1] != 'K'))) return;

    // Got this far? We're good to go
    sd->connected = true;
    i2c_getstatus(sd, true);
    send_command(sd, 'i');
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

    uint8_t a = 0;
    if (readFromSerialPort(sd->port, &a, 1) != 1) return 0;
    return ((a & 1) == 1);
}


/**
 * @brief Get status info from the USB host.
 *
 * @param sd:       Pointer to an I2CDriver structure.
 * @param do_print: Output the results.
 */
void i2c_getstatus(I2CDriver *sd, bool do_print) {

    uint8_t read_buffer[26] = {0};
    send_command(sd, '?');
    size_t bytes_read = readFromSerialPort(sd->port, read_buffer, 25);

#ifdef VERBOSE
    printf("%li bytes were read: %s\n", bytes_read, read_buffer);
#endif

    // Data string is, for example,
    // "1.1.100.110.QTPY-RP2040"
    uint8_t is_ready = 0;
    uint8_t has_started = 0;
    int frequency = 100;
    int address = 0xFF;
    char host_device[32] = {0};

    sscanf((char*)read_buffer, "%c.%c.%i.%i.%s",
        &is_ready,
        &has_started,
        &frequency,
        &address,
        host_device
    );

    // Store in I2C data
    strcpy(sd->model, host_device);
    sd->speed = frequency;

    if (do_print) {
        printf("   I2C host device: %s\n",     host_device);
        printf("Target I2C address: 0x%02X\n", address);
        printf("    I2C is enabled: %s\n",     is_ready == 1 ? "true" : "false");
        printf("     I2C is active: %s\n",     has_started == 1 ? "true" : "false");
        printf("     I2C frequency: %i\n",     frequency);
    }
}


/**
 * @brief Scan the I2C bus and list devices.
 *
 * @param sd: Pointer to an I2CDriver structure.
 */
void i2c_scan(I2CDriver *sd) {

    uint8_t buffer[365] = {0};
    uint8_t* buff_ptr = buffer;

    uint8_t devices[120] = {0};
    uint8_t* dev_ptr = devices;

    send_command(sd, 'd');
    readFromSerialPort(sd->port, buffer, 360);

    // Extract device address hex strings and generate
    // integer values. For example:
    // source = "12.71.A0."
    // dest   = [18, 113, 160]
    while (*buff_ptr != 0) {
        uint8_t value[2] = {0};
        uint8_t* val_ptr = value;

        while(1) {
            uint8_t token = *buff_ptr++;
            if (token == '.') break;
            *val_ptr++ = token;
        }

        *dev_ptr++ = (uint8_t)strtol((char *)value, NULL, 0);
    }

    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
    for (int i = 0 ; i < 0x78; i++) {
        if (i % 16 == 0) printf("\n%02x ", i);
        if ((i & 0x78) == 0 || (i & 0x78) == 0x78) {
            printf("X ");
        } else {
            bool found = false;
            for (int j = 0 ; j < 0x78 ; ++j) {
                if (devices[j] == i) {
                    printf("@ ");
                    found = true;
                    break;
                }

                if (!found) printf(". ");
                if (i % 16 == 15) printf("\n");
            }
        }
    }

    printf("\n");
}


/**
 * @brief Reset the I2C bus.
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
void i2c_stop(I2CDriver *sd) {

    send_command(sd, 'p');
}


/**
 * @brief Write data to the I2C host for transmission.
 *
 * @param sd:        Pointer to an I2CDriver structure.
 * @param bytes:     The bytes to write.
 * @param num_bytes: The number of bytes to write.
 *
 * @retval The number of bytes received.
 */
size_t i2c_write(I2CDriver *sd, const uint8_t bytes[], size_t num_bytes) {

    // Count the bytes sent
    int count = 0;

    // Write the data out in blocks of 64 bytes
    for (size_t i = 0 ; i < num_bytes ; i += 64) {
        // Calculate data length for prefix byte
        size_t length = ((num_bytes - i) < 64) ? (num_bytes - i) : 64;
        uint8_t cmd[65] = {(uint8_t)(PREFIX_BYTE_WRITE + length)};

        // Write a block of bytes to the send buffer
        memcpy(cmd + 1, bytes + i, length);

        // Write out the block -- use ACK as byte count
        writeToSerialPort(sd->port, cmd, 1 + length);
        count += length;
    }

    return count;
}


/**
 * @brief Read data from the I2C host.
 *
 * @param sd:        Pointer to an I2CDriver structure.
 * @param bytes:     A buffer for the bytes to read.
 * @param num_bytes: The number of bytes to write.
 */
void i2c_read(I2CDriver *sd, uint8_t bytes[], size_t num_bytes) {

    for (size_t i = 0 ; i < num_bytes ; i += 64) {
        // Calculate data length for prefix byte
        size_t length = ((num_bytes - i) < 64) ? (num_bytes - i) : 64;
        uint8_t cmd[1] = {(uint8_t)(PREFIX_BYTE_READ + length - 1)};

        writeToSerialPort(sd->port, cmd, 1);
        readFromSerialPort(sd->port, bytes + i, length);
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
int i2c_commands(I2CDriver *sd, int argc, char *argv[]) {

    // Process args one by one
    for (int i = 0 ; i < argc ; ++i) {
        char* command = argv[i];

#ifdef VERBOSE
        printf("Command [%s]\n", command);
#endif

        // Commands should be single characters
        if (strlen(command) != 1) {
            print_bad_command_help(command);
            return 1;
        }

        switch (command[0]) {
            case 'd':   // LIST DEVICES ON BUS
                i2c_scan(sd);
                break;

            case 'i':   // PRINT HOST STATUS INFO
                i2c_getstatus(sd, true);
                break;

            case 'p':   // STOP I2C TRANSACTION
                i2c_stop(sd);
                break;

            case 'r':   // READ FROM BUS
                {
                    command = argv[++i];
                    long address = strtol(command, NULL, 0);

                    command = argv[++i];
                    size_t num_bytes = strtol(command, NULL, 0);
                    uint8_t bytes[8192];

                    i2c_start(sd, address, 1);
                    i2c_read(sd, bytes, num_bytes);
                    i2c_stop(sd);

                    // Output the bytes
                    for (size_t i = 0 ; i < num_bytes ; ++i) {
                        printf("%s0x%02x", i ? "," : "", 0xFF & bytes[i]);
                        printf("\n");
                    }
                }
                break;

            case 'w':   // WRITE TO BUS
                {
                    char* token = argv[++i];
                    long address = strtol(token, NULL, 0);

                    token = argv[++i];
                    size_t num_bytes = 0;
                    uint8_t bytes[8192];
                    char* endptr = token;

                    while (num_bytes < sizeof(bytes)) {
                        bytes[num_bytes++] = (uint8_t)strtol(endptr, &endptr, 0);
                        if (*endptr == '\0') break;
                        if (*endptr != ',') {
                            fprintf(stderr, "Invalid bytes '%s'\n", token);
                            return 1;
                        }

                        endptr++;
                    }

                    i2c_start(sd, (uint8_t)address, 0);
                    i2c_write(sd, bytes, num_bytes);
                }
                break;

            case 'x':   // RESET BUS
                i2c_reset(sd);
                break;

            default:    // NO COMMAND/UNKNOWN COMMAND
                print_bad_command_help(command);
                return 1;
        }
    }

    return 0;
}


#pragma mark - Misc. Functions

/**
 * @brief Write a single-byte command to the serial port.
 *
 * @param sd: Pointer to an I2CDriver structure.
 * @param c:  Command character.
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
    fprintf(stderr, "[ERROR] Bad command: %s\n\n", command);
    show_commands();
}


void show_commands() {
    fprintf(stdout, "Commands:\n");
    fprintf(stdout, "  w [address] [bytes] Write bytes out to I2C.\n");
    fprintf(stdout, "  r [address] {count} Read count bytes in from I2C.\n");
    fprintf(stdout, "  p                   Issue an I2C STOP.\n");
    fprintf(stdout, "  x                   Reset the I2C bus.\n");
    fprintf(stdout, "  d                   Scan for devices on the I2C bus.\n");
    fprintf(stdout, "  i                   Get I2C bus host device information.\n\n");
}
