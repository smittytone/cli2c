/*
 * I2C Host Firmware -- Pi Pico
 *
 * @version     0.1.3
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "main.h"


extern I2C_PINS i2c_pins;


/*
 * ENTRY POINT
 */
int main(void) {
    // Initialise the pin data
    init_pins();

    // Initialise the LED
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


void init_pins(void) {

    i2c_pins.bus_count = 2;
    i2c_pins.bus_0_pair_count = 4;
    i2c_pins.bus_1_pair_count = 4;
    i2c_pins.bus_0_pins[0] = 0;
    i2c_pins.bus_0_pins[1] = 1;
    i2c_pins.bus_0_pins[2] = 4;
    i2c_pins.bus_0_pins[3] = 5;
    i2c_pins.bus_0_pins[4] = 8;
    i2c_pins.bus_0_pins[5] = 9;
    i2c_pins.bus_0_pins[6] = 12;
    i2c_pins.bus_0_pins[7] = 13;
    i2c_pins.bus_1_pins[0] = 2;
    i2c_pins.bus_1_pins[1] = 3;
    i2c_pins.bus_1_pins[2] = 6;
    i2c_pins.bus_1_pins[3] = 7;
    i2c_pins.bus_1_pins[4] = 10;
    i2c_pins.bus_1_pins[5] = 11;
    i2c_pins.bus_1_pins[6] = 14;
    i2c_pins.bus_1_pins[7] = 15;
}