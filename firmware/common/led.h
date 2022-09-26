#ifndef _HEADER_LED_
#define _HEADER_LED_


#ifdef QTPY_BUILD
#include "../qtpy/ws2812.h"
#endif



void led_off(void);
void led_on(void);
void led_set_state(bool is_on);
void led_flash(uint32_t count) ;
void led_set_colour(uint32_t colour);


#endif  // _HEADER_LED_