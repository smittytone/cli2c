#include "led.h"


void led_on() {
#ifdef QTPY_BUILD
    ws2812_pixel(RGB_COLOUR);
#elif defined PICO_BUILD
    gpio_put(PIN_PICO_LED, true);
#endif
}


void led_off() {
#ifdef QTPY_BUILD
    ws2812_pixel(0x00);
#elif defined PICO_BUILD
    gpio_put(PIN_PICO_LED, false);
#endif
}


void led_set_state(bool is_on) {
#ifdef QTPY_BUILD
    ws2812_pixel(is_on ? RGB_COLOUR : 0x00);
#elif defined PICO_BUILD
    gpio_put(PIN_PICO_LED, is_on);
#endif
}


void led_flash(uint32_t count) {
#ifdef QTPY_BUILD
    ws2812_flash(count);
#elif defined PICO_BUILD
    flash_led(count);
#endif
}


void led_set_colour(uint32_t colour) {
#ifdef QTPY_BUILD
   ws2812_set_colour(colour);
#endif
// No Pico function relevant here
}