/*
 * I2C Host Firmware -- Pi Pico LED
 *
 * @version     0.1.6
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "main.h"


/**
 * @brief Initialise the LED's GPIO pin
 */
void pico_led_init() {
    
    gpio_init(PIN_PICO_LED);
    gpio_set_dir(PIN_PICO_LED, GPIO_OUT);
    gpio_put(PIN_PICO_LED, false);
}


/**
 * @brief Turn the LED off.
 */
void pico_led_off() {

    gpio_put(PIN_PICO_LED, false);
}


/**
 * @brief Turn the LED on.
 */
void pico_led_on() {

    gpio_put(PIN_PICO_LED, true);
}


/**
 * @brief Specify the LED's state (on or off).
 * 
 * @param is_on: Turn the LED on (`true`) or off (`false`).
 */
void pico_led_set_state(bool is_on) {

    gpio_put(PIN_PICO_LED, is_on);
}


/**
 * @brief Flash the LED for the specified number of times.
 * 
 * @param count: The number of blinks.
 */
void pico_led_flash(uint32_t count) {

     while (count > 0) {
        gpio_put(PIN_PICO_LED, true);
        sleep_ms(200);
        gpio_put(PIN_PICO_LED, false);
        sleep_ms(200);
        count--;
    }
}

