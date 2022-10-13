/*
 * I2C Host Firmware -- Tiny 2040 LED
 *
 * @version     0.1.5
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "main.h"


Tiny_LED_colour colour;


/**
 * @brief Initialise the LED's GPIO pins
 */
void tiny_led_init(void) {
    
    // Initialise the RGB pins' PWM drivers
    tiny_pwm_init(PIN_TINY_LED_BLUE);
    tiny_pwm_init(PIN_TINY_LED_GREEN);
    tiny_pwm_init(PIN_TINY_LED_RED);

    // Set the LED's colour
    tiny_led_set_colour(DEFAULT_LED_COLOUR);
}


/**
 * @brief Initialise the LED's GPIO pins
 * 
 * @param pin: The GPIO pin to provision.
 */
void tiny_pwm_init(uint pin) {

    gpio_set_function(pin, GPIO_FUNC_PWM);
    
    uint slice =  pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 3);
    pwm_set_chan_level(slice, PWM_CHAN_A, 1);
    pwm_set_chan_level(slice, PWM_CHAN_B, 3);
    pwm_set_enabled(slice, true);
    pwm_set_gpio_level(pin, 0);
}


/**
 * @brief Turn the LED off.
 */
void tiny_led_off(void) {

    tiny_led_set_state(false);
}


/**
 * @brief Turn the LED on.
 */
void tiny_led_on(void) {

    tiny_led_set_state(true);
}


/**
 * @brief Specify the LED's state (on or off).
 * 
 * @param is_on: Turn the LED on (`true`) or off (`false`).
 */
void tiny_led_set_state(bool is_on) {

    if (is_on) {
        pwm_set_gpio_level(PIN_TINY_LED_BLUE, colour.blue);
        pwm_set_gpio_level(PIN_TINY_LED_GREEN, colour.green);
        pwm_set_gpio_level(PIN_TINY_LED_RED, colour.red);
    } else {
        pwm_set_gpio_level(PIN_TINY_LED_BLUE, 0);
        pwm_set_gpio_level(PIN_TINY_LED_GREEN, 0);
        pwm_set_gpio_level(PIN_TINY_LED_RED, 0);
    }
}


/**
 * @brief Flash the LED for the specified number of times.
 * 
 * @param count: The number of blinks.
 */
void tiny_led_flash(uint32_t count) {

     while (count > 0) {
        tiny_led_set_state(true);
        sleep_ms(200);
        tiny_led_set_state(false);
        sleep_ms(200);
        count--;
    }
}


void tiny_led_set_colour(uint32_t rgb_colour) {

    colour.blue = ((rgb_colour & 0xFF) / 0xFF) * 16;
    colour.green = (((rgb_colour & 0xFF00) >> 8) / 0xFF) * 16;
    colour.red = (((rgb_colour & 0xFF0000) >> 16) / 0xFF) * 16;
}