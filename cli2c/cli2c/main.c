/*
 * Generic macOS I2C driver
 *
 * Version 0.1.2
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


// Hold an I2C data structure
I2CDriver i2c;


int main(int argc, char *argv[]) {
    
    // Listen for SIGINT
    signal(SIGINT, ctrl_c_handler);
    
    // Process arguments
    if (argc < 2) {
        // Insufficient arguments -- issue usage info and bail
        printf("Usage: cli2c {DEVICE_PATH} [command] ... [command]\n");
    } else {
        // Check for a help request
        for (int i = 0 ; i < argc ; ++i) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                show_help();
                return EXIT_OK;
            }
        }
        
        // Connect... with the device path
        i2c.port = -1;
        i2c_connect(&i2c, argv[1]);
        
        if (i2c.connected) {
            // Initialize the I2C host's I2C bus
            if (!(i2c_init(&i2c))) {
                print_error("%s could not initialise I2C\n", argv[1]);
                flush_and_close_port(i2c.port);
                return EXIT_ERR;
            }
            
            // Check we have commands to process
            int delta = 2;
            if (delta >= argc) {
                printf("No commands supplied... exiting\n");
                flush_and_close_port(i2c.port);
                return EXIT_OK;
            }
            
            // Process the remaining commands in sequence
            int result = i2c_commands(&i2c, argc, argv, delta);
            flush_and_close_port(i2c.port);
            return result;
        } else {
            print_error("Could not connect to device %s\n", argv[1]);
        }
    }
    
    return EXIT_ERR;
}


/**
 * @brief Issue an error message.
 *
 * @param format_string: Message string with optional formatting
 * @param ...:           Optional injectable values
 */
void print_error(char* format_string, ...) {
    va_list args;
    va_start(args, format_string);
    print_output(true, format_string, args);
    va_end(args);
}


/**
 * @brief Issue any message.
 *
 * @param is_err:        Is the message an error?
 * @param format_string: Message string with optional formatting
 * @param args:          va_list of args from previous call
 */
void print_output(bool is_err, char* format_string, va_list args) {
    
    // Write the message type to the message
    char buffer[1024] = {0};
    sprintf(buffer, is_err ? "[ERROR] " : "[DEBUG] ");
    
    // Write the formatted text to the message
    vsnprintf(&buffer[8], sizeof(buffer) - 9, format_string, args);
    
    // Print it all out
    printf("%s\n", buffer);
}


/**
 * @brief Show help.
 */
void show_help() {
    printf("cli2c {device} [commands]\n\n");
    printf("Usage:\n");
    printf("  {device} is a mandatory device path, eg. /dev/cu.usbmodem-010101.\n");
    printf("  [commands] are optional commands, as shown below.\n\n");
    show_commands();
}


/**
 * @brief Callback for Ctrl-C.
 */
void ctrl_c_handler(int dummy) {
    
    if (i2c.port != -1) flush_and_close_port(i2c.port);
    printf("\n");
    exit(EXIT_OK);
}


