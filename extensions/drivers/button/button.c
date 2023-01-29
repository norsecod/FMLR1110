#include "button.h"

#include "timer.h"

static void button_cb( void* context );
static void debounce_timer_cb( void* context );

void button_init( ButtonHandle_t* handle, const hal_gpio_pin_names_t pin, bool active_high, void ( *callback )( void* context ), void* context ) {
    handle->pin = pin;
    handle->active_high = active_high;
    handle->callback = callback;
    handle->context = context;
    handle->irq.context  = ( void* )handle;
    handle->irq.callback = button_cb;
    handle->debounce_duration = BUTTON_DEBOUNCE_TIME;
    handle->state = BUTTON_STATE_IDLE;

    hal_gpio_init_in( pin, active_high ? BSP_GPIO_PULL_MODE_DOWN : BSP_GPIO_PULL_MODE_UP, active_high ? BSP_GPIO_IRQ_MODE_RISING : BSP_GPIO_IRQ_MODE_FALLING, &handle->irq );

    TimerInit( &handle->debounce_timer, debounce_timer_cb );
    TimerSetContext( &handle->debounce_timer, ( void* )handle );
    TimerSetValue( &handle->debounce_timer, handle->debounce_duration );
}

static void button_cb( void* context ) {
    ButtonHandle_t* handle = ( ButtonHandle_t* )context;

    if( handle->state == BUTTON_STATE_IDLE ) {
        handle->state = BUTTON_STATE_DEBOUNCE;
        TimerStart( &handle->debounce_timer );
    }
}

static void debounce_timer_cb( void* context ) {
    ButtonHandle_t* handle = ( ButtonHandle_t* )context;

    if( handle->state == BUTTON_STATE_DEBOUNCE && hal_gpio_get_value( handle->pin ) == handle->active_high ) {
        handle->callback( handle->context );
    }

    handle->state = BUTTON_STATE_IDLE;
}
