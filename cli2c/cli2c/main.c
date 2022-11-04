/*
 * Generic macOS I2C driver
 *
 * Version 1.2.0
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


/*
 * STATIC PROTOTYPES
 */
static void    show_help(void);
static void    show_version(void);


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

        // Connect... with the device path
        i2c.port = -1;
        i2c.started = false;
        i2c_connect(&i2c, argv[1]);

        if (i2c.connected) {
            // Check we have commands to process
            int delta = 2;
            if (delta >= argc) {
                fprintf(stderr, "No commands supplied... exiting\n");
                flush_and_close_port(i2c.port);
                return EXIT_OK;
            }

            // Process the remaining commands in sequence
            int result = process_commands(&i2c, argc, argv, delta);
            flush_and_close_port(i2c.port);
            return result;
        } else {
            print_error("Could not connect to device %s\n", argv[1]);
        }
    }

    return EXIT_ERR;
}


/**
 * @brief Show help.
 */
static void show_help() {
    
    fprintf(stderr, "cli2c {device} [commands]\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  {device} is a mandatory device path, eg. /dev/cu.usbmodem-101.\n");
    fprintf(stderr, "  [commands] are optional commands, as shown below.\n\n");
    show_commands();
}


/**
 * @brief Show app version.
 */
static void show_version() {
    
    fprintf(stderr, "cli2c %s\n", APP_VERSION);
    fprintf(stderr, "Copyright © 2022, Tony Smith.\n");
}
