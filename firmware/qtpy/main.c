/*
 * I2C Host
 *
 * @version     0.1.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "main.h"


/*
 * ENTRY POINT
 */
int main() {
    // Initalise the LED
    ws2812_init();

    // Enable STDIO and allow 2s for the board to come up
    if (stdio_usb_init()) {
        sleep_ms(2000);

        // Start the loop
        input_loop();

        // End
        return 0;
    }

    // Could not initialize stdio over USB,
    // so signal error and end
    ws2812_set_colour(0xFF0000);
    ws2812_flash(10);
    ws2812_pixel(0xFF0000);
    return 1;
}
