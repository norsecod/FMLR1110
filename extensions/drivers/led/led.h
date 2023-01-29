#ifndef __EXTENSIONS_DRIVER_LED_LED_H__
#define __EXTENSIONS_DRIVER_LED_LED_H__

#include "stdbool.h"
#include "stdint.h"

#include "smtc_hal_gpio.h"

#include "timer.h"

#define LED_ON      true
#define LED_OFF     !LED_ON

#define LED_ACTIVE_HIGH     true
#define LED_ACTIVE_LOW      !LED_ACTIVE_HIGH

typedef struct LedHandle_s {
    hal_gpio_pin_names_t pin;
    bool active_high;
    bool state;
    int32_t blink_duration;
    uint32_t blink_period;
    TimerEvent_t blink_timer;
} LedHandle_t;

void led_init( LedHandle_t* handle, const hal_gpio_pin_names_t pin, bool active_high );

void led_set( LedHandle_t* handle, bool value );

void led_toggle( LedHandle_t* handle );

void led_blink( LedHandle_t* handle, uint32_t period, int32_t duration );

#endif // __EXTENSIONS_DRIVER_LED_LED_H__
