#include "led.h"

static void blink_timer_cb( void* context );

void led_init( LedHandle_t* handle, const hal_gpio_pin_names_t pin, bool active_high ) {
    handle->pin = pin;
    handle->active_high = active_high;
    hal_gpio_init_out( pin, !active_high );
    led_set( handle, 0 );
}

void led_set( LedHandle_t* handle, bool value ) {
    hal_gpio_set_value( handle->pin, handle->active_high ? value : !value );
    handle->state = value;
}

void led_toggle( LedHandle_t* handle ) {
    led_set( handle, !handle->state );
}

void led_blink( LedHandle_t* handle, uint32_t period, int32_t duration ) {
    handle->blink_period = period;
    handle->blink_duration = duration;
    TimerInit( &handle->blink_timer, &blink_timer_cb );
    TimerSetValue( &handle->blink_timer, period );
    TimerSetContext( &handle->blink_timer, ( void* )handle );
    led_set( handle, LED_ON );
    blink_timer_cb( ( void* )handle );
}

static void blink_timer_cb( void* context ) {
    LedHandle_t* handle = ( LedHandle_t* )context;

    if( handle->blink_duration <= 0 ) {
        led_set( handle, LED_OFF );
        return;
    }

    handle->blink_duration -= handle->blink_period;
    led_toggle( handle );
    TimerStart( &handle->blink_timer );
}
