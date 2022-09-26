/**
 *
 * I2C driver
 * Version 0.1.0
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


int main(int argc, char *argv[]) {
    
    // Process arguments
    if (argc < 2) {
        // Insufficient arguments -- issue usage info and bail
        printf("Usage: cli2c {DEVICE_PATH} [command] ... [command]\n");
    } else {
        // Instantiate an I2C data structure
        I2CDriver i2c;
        
        // Connect... with the device patj
        i2c_connect(&i2c, argv[1]);
        if (i2c.connected) {
            // Connected -- process the remaining commands in sequence
            int delta = 2;
            return i2c_commands(&i2c, argc - delta, argv + delta);
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
    print_output(false, format_string, args);
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
    
    // Add NEWLINE to the message and output to UART
    sprintf(&buffer[strlen(buffer)], "\n");
    
    // Print it all out
    printf("%s", buffer);
}
