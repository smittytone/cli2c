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
    pico_led_init();
    pico_led_off();
    
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
    pico_led_flash(10);
    pico_led_off();
    return 1;
}


