#ifndef __EXTENSIONS_DRIVER_BUTTON_BUTTON_H__
#define __EXTENSIONS_DRIVER_BUTTON_BUTTON_H__

#include "stdint.h"
#include "stdbool.h"

#include "smtc_hal_gpio.h"

#include "timer.h"

#define BUTTON_ACTIVE_HIGH     true
#define BUTTON_ACTIVE_LOW      !BUTTON_ACTIVE_HIGH

#define BUTTON_DEBOUNCE_TIME   50

typedef enum ButtonState_e {
    BUTTON_STATE_IDLE      = 0,
    BUTTON_STATE_DEBOUNCE  = 1
} ButtonState_t;

typedef struct ButtonHandle_s {
    hal_gpio_pin_names_t pin;
    bool active_high;
    TimerEvent_t debounce_timer;
    void ( *callback )( void* context );
    void* context;
    hal_gpio_irq_t irq;
    uint8_t state;
    uint16_t debounce_duration;
} ButtonHandle_t;

void button_init( ButtonHandle_t* handle, const hal_gpio_pin_names_t pin, bool active_high, void ( *callback )( void* context ), void* context );

#endif // __EXTENSIONS_DRIVER_BUTTON_BUTTON_H__
