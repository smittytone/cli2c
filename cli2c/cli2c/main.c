/*
 * Generic macOS/Linux I2C driver
 *
 * Version 1.1.3
 * Copyright © 2023, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


/*
 * STATIC PROTOTYPES
 */
static inline void  show_help(void);
static inline void  show_version(void);
static inline void  show_commands(void);


/*
 * GLOBALS
 */
// Hold an I2C data structure
I2CDriver i2c;


/**
 * @brief Main entry point.
 */
int main(int argc, char *argv[]) {

    // Listen for SIGINT
    signal(SIGINT, ctrl_c_handler);

    // Process arguments
    if (argc < 2) {
        // Insufficient arguments -- issue usage info and bail
        fprintf(stderr, "Usage: cli2c {DEVICE_PATH} [command] ... [command]\n");
        return EXIT_OK;
    } else {
        // Check for a help and/or version request
        for (int i = 0 ; i < argc ; ++i) {
            if (strcasecmp(argv[i], "h") == 0 ||
                strcasecmp(argv[i], "--help") == 0 ||
                strcasecmp(argv[i], "-h") == 0) {
                show_help();
                return EXIT_OK;
            }

            if (strcasecmp(argv[i], "v") == 0 ||
                strcasecmp(argv[i], "--version") == 0 ||
                strcasecmp(argv[i], "-v") == 0) {
                show_version();
                return EXIT_OK;
            }
        }
        
        // Check we have commands to process
        int delta = 2;
        if (argc > delta) {
            // Connect... with the device path
            i2c.port = -1;
            i2c_connect(&i2c, argv[1]);
            
            if (i2c.connected) {
                // Process the remaining commands in sequence
                int result = process_commands(&i2c, argc, argv, delta);
                flush_and_close_port(i2c.port);
                return result;
            }
        } else {
            fprintf(stderr, "No commands supplied... exiting\n");
            return EXIT_OK;
        }
    }

    if (i2c.port != -1) flush_and_close_port(i2c.port);
    return EXIT_ERR;
}


/**
 * @brief Show help.
 */
static inline void show_help() {
    
    fprintf(stderr, "cli2c {device} [commands]\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  {device} is a mandatory device path, eg. /dev/cu.usbmodem-101.\n");
    fprintf(stderr, "  [commands] are optional commands, as shown below.\n\n");
    show_commands();
}


/**
 * @brief Show app version.
 */
static inline void show_version() {
    
    fprintf(stderr, "cli2c %s\n", APP_VERSION);
    fprintf(stderr, "Copyright © 2023, Tony Smith.\n");
}


/**
 * @brief Output help info.
 */
static inline void show_commands(void) {
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  z                                Initialise the I2C bus.\n");
    fprintf(stderr, "  c {bus ID} {SDA pin} {SCL pin}   Configure the I2C bus.\n");
    fprintf(stderr, "  f {frequency}                    Set the I2C bus frequency in multiples of 100kHz.\n");
    fprintf(stderr, "                                   Only 1 and 4 are supported.\n");
    fprintf(stderr, "  w {address} {bytes}              Write bytes out to I2C.\n");
    fprintf(stderr, "  r {address} {count}              Read count bytes in from I2C.\n");
    fprintf(stderr, "                                   Issues a STOP after all the bytes have been read.\n");
    fprintf(stderr, "  p                                Manually issue an I2C STOP.\n");
    fprintf(stderr, "  x                                Reset the I2C bus.\n");
    fprintf(stderr, "  s                                Scan for devices on the I2C bus.\n");
    fprintf(stderr, "  i                                Get I2C bus host device information.\n");
    fprintf(stderr, "  g {number} [hi|lo] [in|out]      Control a GPIO pin.\n");
    fprintf(stderr, "  l {on|off}                       Turn the I2C bus host LED on or off.\n");
    fprintf(stderr, "  h                                Show help and quit.\n");
}
