/*
 * I2C Host
 *
 * @version     0.1.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#ifndef _HEADER_LED_
#define _HEADER_LED_


#ifdef QTPY_BUILD
#include "../qtpy/ws2812.h"
#endif

#ifdef PICO_BUILD
#include "../pico/pico_led.h"
#endif


/*
 * PROTOTYPES
 */
void led_off(void);
void led_on(void);
void led_set_state(bool is_on);
void led_flash(uint32_t count) ;
void led_set_colour(uint32_t colour);


#endif  // _HEADER_LED_