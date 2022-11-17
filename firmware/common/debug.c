/*
 * RP2040 Bus Host Firmware - Debug functions
 *
 * @version     1.2.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "debug.h"


/**
 * @brief Post a debug log message to UART.
 *
 * @param format_string: Message string with optional formatting
 * @param ...:           Optional injectable values
 */
void debug_log(char* format_string, ...) {
    
    va_list args;
    char buffer[DEBUG_MESSAGE_MAX_B] = {0};

    // Compile the string
    va_start(args, format_string);
    vsnprintf(buffer, sizeof(buffer) - 2, format_string, args);
    va_end(args);

    // Issue the compiled string and EOL markers to UART
    uart_puts(DEBUG_UART, buffer);
    uart_puts(DEBUG_UART, "\r\n");
}


void debug_log_bytes(uint8_t* data, size_t count) {
    
    char buffer[130] = {0};
    int j = 0;

    for (size_t i = 0 ; i < count ; ++i) {
        j += sprintf(&buffer[i * 2], "%02X", data[i]);
    }

    //sprintf(buffer, "%04x", j);

    // Issue the compiled string and EOL markers to UART
    uart_puts(DEBUG_UART, "Byte(s): 0x");
    uart_puts(DEBUG_UART, buffer);
    uart_puts(DEBUG_UART, "\r\n");
}