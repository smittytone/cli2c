/*
 * I2C Host Firmware -- Pi Pico
 *
 * @version     0.1.1
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "main.h"


/*
 * ENTRY POINT
 */
int main(void) {
    // Initalise the LED
    ws2812_init();

    // Enable STDIO and allow 2s for the board to come up
    if (stdio_usb_init()) {
        stdio_set_translate_crlf(&stdio_usb, false);;
        stdio_flush();

        // Start the loop
        // Function defined in `serial.c`
        rx_loop();

        // End
        return 0;
    }

    // Could not initialize stdio over USB,
    // so signal error (red) and end
    ws2812_set_colour(0xFF0000);
    ws2812_flash(10);
    ws2812_pixel(0xFF0000);
    return 1;
}

