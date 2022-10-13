/*
 * I2C Host Firmware - LED control middleware
 *
 * @version     0.1.4
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "led.h"


/*
 * This file maps calls from serial.c to the various supported boards'
 * LED implementations — eg. Neopixel or monochrome LED.
 *
 * It relies on defines set in the boards' respective CMakeList.txt
 * files.
 *
 */


/**
 * @brief Turn the LED on.
 */
void led_on(void) {
#ifdef NEO_BUILD
    ws2812_set_state(true);
#elif defined LED_BUILD
    pico_led_on();
#endif
}


/**
 * @brief Turn the LED off.
 */
void led_off(void) {
#ifdef NEO_BUILD
    ws2812_set_state(false);
#elif defined LED_BUILD
    pico_led_off();
#endif
}


/**
 * @brief Specify the LED's state (on or off).
 *
 * @param is_on: Turn the LED on (`true`) or off (`false`).
 */
void led_set_state(bool is_on) {
#ifdef NEO_BUILD
    ws2812_set_state(is_on);
#elif defined LED_BUILD
    pico_led_set_state(is_on);
#endif
}


/**
 * @brief Flash the LED for the specified number of times.
 *
 * @param count: The number of blinks.
 */
void led_flash(uint32_t count) {
#ifdef NEO_BUILD
    ws2812_flash(count);
#elif defined LED_BUILD
    pico_led_flash(count);
#endif
}


/**
 * @brief Set the LED's colour.
 *        Only relevant for Neopixels.
 *
 * @param colour: The LED colour as an RGB six-digit RGB hex value.
 */
void led_set_colour(uint32_t colour) {
#ifdef NEO_BUILD
   ws2812_set_colour(colour);
#endif
/*
 * No Pico function is relevant here -- just return
 */
}