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

    uint32_t ts = to_ms_since_boot(get_absolute_time());
    sprintf(buffer, "%i ", ts);
    size_t len = strlen(buffer);

    // Compile the string
    va_start(args, format_string);
    vsnprintf(buffer + len, sizeof(buffer) - 2 - len, format_string, args);
    va_end(args);

    // Issue the compiled string and EOL markers to UART
    uart_puts(DEBUG_UART, buffer);
    uart_puts(DEBUG_UART, "\r\n");
}


void debug_log_bytes(uint8_t* data, size_t count) {
    
    char buffer[DEBUG_MESSAGE_MAX_B] = {0};
    int j = 0;

    uint32_t ts = to_ms_since_boot(get_absolute_time());
    sprintf(buffer, "%i ", ts);
    size_t len = strlen(buffer);

    for (size_t i = 0 ; i < count ; ++i) {
        j += sprintf(&buffer[i * 2 + len], "%02X", data[i]);
    }

    // sprintf(buffer, "%04x", j);

    // Issue the compiled string and EOL markers to UART
    uart_puts(DEBUG_UART, buffer);
    uart_puts(DEBUG_UART, "\r\n");
}