/*
 * Generic macOS I2C driver - Utility Functions
 *
 * Version 1.2.0
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "utils.h"


// Declared in each app's main.c
extern I2CDriver i2c;


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
 * @brief Issue a warning message.
 *
 * @param format_string: Message string with optional formatting
 * @param ...:           Optional injectable values
 */
void print_warning(char* format_string, ...) {
    va_list args;
    va_start(args, format_string);
    print_output(false, format_string, args);
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
    sprintf(buffer, is_err ? "[ERROR] " : "[WARNING] ");

    // Write the formatted text to the message
    vsnprintf(&buffer[is_err ? 8 : 10], sizeof(buffer) - (is_err ? 9 : 11), format_string, args);

    // Print it all out
    fprintf(stderr, "%s\n", buffer);
}


/**
 * @brief Callback for Ctrl-C.
 */
void ctrl_c_handler(int dummy) {

    if (i2c.port != -1) flush_and_close_port(i2c.port);
    fprintf(stderr, "\n");
    exit(EXIT_OK);
}


/**
 * @brief Convert a string to lowercase.
 *
 * @param s: Pointer to the string to convert.
 */
void lower(char* s) {
    size_t l = strlen(s);
    for (int j = 0 ; j < l ; ++j) {
        if (s[j] >= 'A' && s[j] <= 'Z') s[j] += 32;
    }
}
