/*
 * I2C Host -- Pi Pico
 *
 * @version     0.1.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "main.h"


void pico_led_init() {
    
    gpio_init(PIN_PICO_LED);
    gpio_set_dir(PIN_PICO_LED, GPIO_OUT);
    gpio_put(PIN_PICO_LED, false);
}


void pico_led_off() {

    gpio_put(PIN_PICO_LED, false);
}


void pico_led_on() {

    gpio_put(PIN_PICO_LED, true);
}


void pico_led_set_state(bool is_on) {

    gpio_put(PIN_PICO_LED, is_on);
}


void pico_led_flash(uint32_t count) {

     while (count > 0) {
        gpio_put(PIN_PICO_LED, true);
        sleep_ms(250);
        gpio_put(PIN_PICO_LED, false);
        sleep_ms(250);
        count--;
    }
}

