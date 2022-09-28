/**
 *
 * I2C driver
 * Version 0.1.0
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


I2CDriver* bus;


int main(int argc, char *argv[]) {
    
    // Listen for SIGINT
    signal(SIGINT, ctrl_c_handler);
    
    // Process arguments
    if (argc < 2) {
        // Insufficient arguments -- issue usage info and bail
        printf("Usage: cli2c {DEVICE_PATH} [command] ... [command]\n");
    } else {
        // Check for help request
        for (int i = 0 ; i < argc ; ++i) {
            if (strcmp(argv[i], "-h") == 0) {
                show_help();
                return 0;
            }
        }
        
        // Instantiate an I2C data structure
        I2CDriver i2c;
        bus = &i2c;
        i2c.port = -1;
        
        // Connect... with the device path
        i2c_connect(&i2c, argv[1]);
        if (i2c.connected) {
            // Connected -- process the remaining commands in sequence
            int delta = 2;
            int result = i2c_commands(&i2c, argc - delta, argv + delta);
            clear_serial();
            return result;
        } else {
            print_error("Could not connect to device %s\n", argv[1]);
        }
    }
    
    return 1;
}


/**
 * @brief Issue an error message.
 *
 * @param format_string Message string with optional formatting
 * @param ...           Optional injectable values
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
 * @param is_err        Is the message an error?
 * @param format_string Message string with optional formatting
 * @param args          va_list of args from previous call
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
    fprintf(stdout, "cli2c {device} [commands]\n\n");
    show_commands();
}


void ctrl_c_handler(int dummy) {
    
    clear_serial();
    exit(0);
}


void clear_serial() {
    
    if (bus->port != -1) {
        sleep(2);
        tcflush(bus->port, TCIOFLUSH);
        int result = close(bus->port);
        if (result == 0) bus->port = -1;
    }
}
