/*
 * RP2040 Bus Host Firmware - GPIO functions
 *
 * @version     1.1.2
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include "gpio.h"


/**
 * @brief Set a GPIO pin.
 *
 * @param gps:        The GPIO state record.
 * @param read_value: Pointer to a byte into which to read the pin value, if read.
 * @param data:       The command data.
 *
 * @retval Whether the operation was successful (`true`) or not (`false`).
 */
bool set_gpio(GPIO_State* gps, uint8_t* read_value, uint8_t* data) {

    uint8_t gpio_pin = (data[1] & 0x1F);
    bool pin_state   = ((data[1] & 0x80) > 0);
    bool is_dir_out  = ((data[1] & 0x40) > 0);
    bool is_read     = ((data[1] & 0x20) > 0);

    // NOTE Function will not have been called if a bus is using the pin,
    //      but the check should really be here

    // Register pin usage, state and initialise if necessary
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
