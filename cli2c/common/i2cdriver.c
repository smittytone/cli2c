/**
 *
 * I2C driver w. HT16K33
 * Version 1.0.0
 * Copyright © 2019, James Bowman; 2022, Tony Smith (@smittytone)
 * Licence: BSD 3-Clause
 *
 */
#include "main.h"


// ****************************   Serial port  ********************************

/**
 * @brief Open a Mac serial port.
 *
 * @param device_file The target port file, eg. /dev/cu.usb-modem-10100
 *
 * @retval The OS file descriptor.
 */
int openSerialPort(const char *device_file) {
    struct termios serial_ettings;

    int fd = open(device_file, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        print_error("Could not open %s", device_file);
        return -1;
    }

    tcgetattr(fd, &serial_ettings);
    cfmakeraw(&serial_ettings);
    serial_ettings.c_cc[VMIN] = 1;
    // serial_ettings.c_cc[VTIME] = 10;
    
    if (tcsetattr(fd, TCSANOW, &serial_ettings) != 0) {
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
    
    while (total_read < byte_count) {
        number_read = read(fd, buffer + total_read, byte_count);
        if (number_read > 0) total_read += number_read;
    }

#ifdef VERBOSE
    printf(" READ %d %d: ", (int)byte_count, (int)number_read);
    for (int i = 0 ; i < byte_count ; ++i) {
        printf("%02x ", 0xFF & buffer[i]);
    }
    printf("\n");
#endif

    return total_read;
}


void writeToSerialPort(int fd, const uint8_t* buffer, size_t byte_count) {
    write(fd, buffer, byte_count);

#ifdef VERBOSE
    printf("WRITE %u: ", (int)byte_count);
    for (int i = 0 ; i < byte_counts ; ++i) {
        printf("%02x ", 0xFF & buffer[i]);
    }
    printf("\n");
#endif
}

// ******************************  CCITT CRC  *********************************

static const uint16_t crc_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};


static void crc_update(I2CDriver *sd, const uint8_t *data, size_t data_len) {
    unsigned int tbl_idx;
    uint16_t crc = sd->e_ccitt_crc;

    while (data_len--) {
        tbl_idx = ((crc >> 8) ^ *data) & 0xff;
        crc = (crc_table[tbl_idx] ^ (crc << 8)) & 0xffff;
        data++;
    }
    sd->e_ccitt_crc = crc;
}

// ******************************  I2CDriver  *********************************

void i2c_connect(I2CDriver *sd, const char* portname) {
    // Mark that we're not connected
    sd->connected = 0;

    // Open and get the serial port or bail
    sd->port = openSerialPort(portname);
    if (sd->port == -1) return;

    // Clear the FIFO?
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

    // Got this far? We're good to go
    sd->connected = 1;
    i2c_getstatus(sd);
    sd->e_ccitt_crc = sd->ccitt_crc;
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
static int i2c_ack(I2CDriver *sd) {
    uint8_t a[1];
    if (readFromSerialPort(sd->port, a, 1) != 1) return 0;
    return (a[0] & 1);
}


/**
 * @brief Get status info from the USB host.
 *
 * @param sd: Pointer to an I2CDriver structure.
 */
void i2c_getstatus(I2CDriver *sd)   {
    
    uint8_t read_buffer[100];
    send_command(sd, '?');
    size_t bytes_read = readFromSerialPort(sd->port, read_buffer, 80);
    read_buffer[bytes_read] = 0;

#ifdef VERBOSE
    printf("%d bytes were read: %.*s\n", bytes_read, bytes_read, read_buffer);
#endif
    
    uint8_t is_ready = 0;
    uint8_t has_started = 0;
    int frequency = 100;
    int address = 0xFF;
    char msg[32] = {0};
    
    sscanf((char*)read_buffer, "%c.%c.%i.%i.%s",
        &is_ready,
        &has_started,
        &frequency,
        &address,
        msg
    );
    
    printf("Target I2C address: %02x\n", address);
    printf("    I2C is enabled: %s\n", is_ready == 1 ? "true" : "false");
    printf("     I2C is active: %s\n", has_started == 1 ? "true" : "false");
    printf("     I2C frequency: %i\n", frequency);
}


/**
 * @brief Scan the I2C bus and list devices.
 *
 * @param sd: Pointer to an I2CDriver structure.
 */
void i2c_scan(I2CDriver *sd) {
    
    uint8_t buffer[128] = {0};
    uint8_t* buff_ptr = buffer;
    
    send_command(sd, 'd');
    readFromSerialPort(sd->port, buffer, 127);
    
    uint8_t devices[128] = {0};
    uint8_t* dev_ptr = devices;
    
    while (*buff_ptr != '0') {
        uint8_t value[4] = {0};
        uint8_t* val_ptr = value;
        
        while(1) {
            uint8_t token = *buff_ptr++;
            
            if (token == '.') {
                break;
            }
            
            *val_ptr++ = token;
        }
        
        *dev_ptr++ = (uint8_t)strtol((char *)value, NULL, 0);
    }
    
    
    printf("\n");
    for (int i = 0 ; i < 0x78; i++) {
        if (i % 16 == 0) printf("\n%02x ", i);
        if (i == 0) {
            printf("X  ");
        } else {
            bool found = false;
            for int j = 0 ; j < 128 ; ++j) {
                if (devices[j] == i) {
                    printf("@  ");
                    found = true;
                    break;
                }
                
                if (!found) printf(".  ");
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
    return (i2c_ack(sd) == 1);
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
    
    // Include the command in the data
    uint8_t start_data[2] = {'s', ((address << 1) | op)};
    writeToSerialPort(sd->port, start_data, sizeof(start_data));
    return (i2c_ack(sd) == 1);
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
        size_t length = ((num_bytes - i) < 64) ? (num_bytes - i) : 64;
        uint8_t cmd[65] = {(uint8_t)(0xC0 + length - 1)};
        
        // Write a block of bytes to the send buffer
        memcpy(cmd + 1, bytes + i, length);
        
        // Write out the block -- use ACK as byte count
        writeToSerialPort(sd->port, cmd, 1 + length);
        count += i2c_ack(sd);
    }

    crc_update(sd, bytes, num_bytes);
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
        size_t length = ((num_bytes - i) < 64) ? (num_bytes - i) : 64;
        uint8_t cmd[1] = {(uint8_t)(0x80 + length - 1)};
        writeToSerialPort(sd->port, cmd, 1);
        readFromSerialPort(sd->port, bytes + i, length);
        crc_update(sd, bytes + i, length);
    }
}


void i2c_test(I2CDriver *sd) {
    bool success = false;
    char msg[] = "Testing 1,2,3";
    char in_buffer[20] = {0};
    
    writeToSerialPort(sd->port, (uint8_t*)msg, strlen(msg));
    readFromSerialPort(sd->port, (uint8_t*)in_buffer, strlen(msg));
    success = strncmp(msg, in_buffer, 13) == 0;
    printf(success ? "I2C test success\n" : "I2C test failure\n");
}


/**
 * @brief Read data from the I2C host.
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
        char *command = argv[i];

#ifdef VERBOSE
        printf("Command [%s]\n", command);
#endif

        // Commands should be single characters
        if (strlen(command) != 1) {
            print_bad_command_help(command);
            return 1;
        }

        switch (command[0]) {
            case 'i':   // PRINT STATUS INFO
                i2c_getstatus(sd);
                break;

            case 'x':   // RESET BUS
                i2c_reset(sd);
                break;

            case 'd':   // LIST DEVICES
                {
                    uint8_t devices[128];
                    i2c_scan(sd, devices);
                    printf("\n");
                    for (int i = 8; i < 0x78; i++) {
                        if (devices[i] == '1') {
                            printf("%02x  ", i);
                        } else {
                            printf("--  ");
                        }

                        if ((i % 8) == 7) printf("\n");
                    }
                    printf("\n");
                }
                break;

            case 'w':   // WRITE TO BUS
                {
                    command = argv[++i];
                    long address = strtol(command, NULL, 0);

                    command = argv[++i];
                    size_t num_bytes = 0;
                    uint8_t bytes[8192];
                    char* endptr = command;

                    while (num_bytes < sizeof(bytes)) {
                        bytes[num_bytes++] = strtol(endptr, &endptr, 0);
                        if (*endptr == '\0') break;
                        if (*endptr != ',') {
                            fprintf(stderr, "Invalid bytes '%s'\n", command);
                            return 1;
                        }

                        endptr++;
                    }

                    i2c_start(sd, address, 0);
                    i2c_write(sd, bytes, num_bytes);
                }
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

            case 'p':   // STOP
                i2c_stop(sd);
                break;

            default:    // NO COMMAND/UNKNOWN COMMAND
                print_bad_command_help(command);
                return 1;
        }
    }

    return 0;
}


/**
 * @brief Write a single-byte command to the serial port.
 *
 * @param sd: Pointer to an I2CDriver structure.
 * @param c:  Command character.
 */
static void send_command(I2CDriver* sd, char c) {
    //
    writeToSerialPort(sd->port, (uint8_t*)&c, 1);
}


/**
 * @brief Output help info on receipt of a bad command.
 *
 * @param command: The bad command.
 */
static void print_bad_command_help(char* command) {
    fprintf(stderr, "[ERROR] Bad command: %s\n\n", command);
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  i                 Display status information\n");
    fprintf(stderr, "  x                 Reset I2C bus\n");
    fprintf(stderr, "  d                 Scan bus for Devices\n");
    fprintf(stderr, "  w address <bytes> Write bytes to I2C device at address\n");
    fprintf(stderr, "  p                 Send a STOP\n");
    fprintf(stderr, "  r address N       Read N bytes from I2C device at address, then STOP\n\n");
}
