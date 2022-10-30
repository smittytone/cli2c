/*
 * Bus Host Firmware - GPIO functions
 *
 * @version     2.0.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "gpio.h"





bool set_gpio(GPIO_State* gps, uint8_t* read_value, uint8_t* data) {

    uint8_t gpio_pin = (data[1] & 0x1F);
    bool pin_state   = ((data[1] & 0x80) > 0);
    bool is_dir_out  = ((data[1] & 0x40) > 0);
    bool is_read     = ((data[1] & 0x20) > 0);

    // TODO
    // Make sure the pin's not in use for active I2C and/or active SPI

    // Register pin usage, state
    if (gps->state_map[gpio_pin] == 0x00) {
        gpio_init(gpio_pin);
        gpio_set_dir(gpio_pin, (is_dir_out ? GPIO_OUT : GPIO_IN));
        gps->state_map[gpio_pin] |= (1 << GPIO_PIN_DIRN_BIT);
        gps->state_map[gpio_pin] |= (1 << GPIO_PIN_STATE_BIT);
    }

    if (is_read) {
        // Pin is DIGITAL_IN, so get and return the state
        uint8_t pin_value = gpio_get(gpio_pin) ? 0x80 : 0x00;
        *read_value = (pin_value | gpio_pin);
        return true;
    } else if (is_dir_out) {
        // Pin is DIGITAL_OUT, so just set the state
        gpio_put(gpio_pin, pin_state);
        return true;
    } else {
        // Pin is DIGITAL_IN, but we're just setting it
        return true;
    }

    return false;
}