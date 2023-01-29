#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type

#include "smtc_modem_api.h"
#include "smtc_modem_utilities.h"

#include "smtc_modem_hal.h"
#include "smtc_hal_dbg_trace.h"

#include "smtc_hal_mcu.h"
#include "smtc_hal_gpio.h"
#include "smtc_hal_rtc.h"

#if defined( SX128X )
#include "ralf_sx128x.h"
#elif defined( SX126X )
#include "ralf_sx126x.h"
#elif defined( LR11XX )
#include "ralf_lr11xx.h"
#include "lr11xx_wifi.h"
#include "lr11xx_gnss.h"
#endif

#include "string.h"

#include "settings.h"
#include "led.h"
#include "button.h"
#include "sht3x.h"
#include "lis2dw.h"
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define DEFAULT_UL_PORT         7
#define DEFAULT_DL_PORT         100

#define PERIODIC_TIMER_PERIOD 20000

#define LINK_CHECK_RATIO_THRESHOLD 50
#define LINK_CHECK_ATTEMPTS_THRESHOLD 10

/**
 * Stack id value (multistacks modem is not yet available)
 */
#define STACK_ID 0

#if defined( SX128X )
const ralf_t modem_radio = RALF_SX128X_INSTANTIATE( NULL );
#define RADIO   "SX128X"
#elif defined( SX126X )
const ralf_t modem_radio = RALF_SX126X_INSTANTIATE( NULL );
#define RADIO   "SX126X"
#elif defined( LR11XX )
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE( NULL );
#define RADIO   "LR11XX"
#else
#error "Please select radio board.."
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */
static volatile bool user_button_is_press = false;  // Flag for button status
static bool          periodic_message_flag;
static uint8_t       rx_payload[255]      = { 0 };  // Buffer for rx payload
static uint8_t       rx_payload_size      = 0;      // Size of the payload in the rx_payload buffer
extern settings_t    settings;
uint32_t txdone_counter = 0;
uint32_t link_check_attempts = 0;
static LedHandle_t led_fmlr, led_r, led_g, led_b;
static ButtonHandle_t button_handle;
static uint32_t user_button_state, tx_pending;
static int32_t  temp, rh;
static uint8_t wifi_count, gnss_count;
static int16_t acc_axes[3];
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */
static bool is_joined( void );
static void get_event( void );
static void button_led_test( void );
static void gps_snap( void );
static void wifi_scan( void );
static void user_button_callback( void* context );
static void sensor_read( void );
static void tx_measurement_data_keep_alive();
static void init_func();

static TimerEvent_t periodic_timer;
static void periodic_timer_cb( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "periodic_timer_cb\n" );
    periodic_message_flag = true;
    TimerStart( &periodic_timer );
}

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

/**
 * @brief Example to send a user payload on an external event
 *
 */
int main( void ) {

    // Disable IRQ to avoid unwanted behaviour during init
    hal_mcu_disable_irq( );

    // Configure all the uC periph (clock, gpio, timer, ...)
    hal_mcu_init( );
    SMTC_HAL_TRACE_PRINTF("_______________________________________________________________________________\n");
    SMTC_HAL_TRACE_PRINTF("Miromico EVK\n\n");
    SMTC_HAL_TRACE_PRINTF("Radio:%s\n", RADIO);
    SMTC_HAL_TRACE_PRINTF("Application version: %s\n", GIT_VERSION);
    SMTC_HAL_TRACE_PRINTF("GIT commit: %s\n", GIT_COMMIT);

    // Init the modem and use get_event as event callback, please note that the callback will be
    // called immediately after the first call to smtc_modem_run_engine because of the reset detection
//    SMTC_HAL_TRACE_PRINTF("calling: smtc_modem_init\n");
    smtc_modem_init( &modem_radio, &get_event );
    smtc_modem_version_t version;
    smtc_modem_get_modem_version( &version );

    SMTC_HAL_TRACE_INFO( "Modem driver version: %d.%d.%d\n", version.major, version.minor, version.patch );
    // Re-enable IRQ
//    SMTC_HAL_TRACE_PRINTF("hal_mcu_enable_irq\n");
    hal_mcu_enable_irq( );
    button_led_test();
    init_func();



    //periodic timer example
    TimerInit( &periodic_timer, &periodic_timer_cb );
    TimerSetValue( &periodic_timer, PERIODIC_TIMER_PERIOD );
    TimerSetContext( &periodic_timer, NULL );
    TimerStart(&periodic_timer);


#if defined( LR11XX )
    lr11xx_status_t ret;
    lr11xx_system_version_t lr11xx_version;
    ret = lr11xx_system_get_version( modem_radio.ral.context, &lr11xx_version );
    SMTC_HAL_TRACE_PRINTF("LR11xx Version: HW: %x, type: %x, fw: %x\n", lr11xx_version.hw, lr11xx_version.type, lr11xx_version.fw);

    wifi_scan();
    gps_snap();
#endif

    while( 1 ) {
        uint32_t start = hal_rtc_get_time_ms( );

        // Execute modem runtime, this function must be recalled in sleep_time_ms (max value, can be recalled sooner)
        uint32_t sleep_time_ms = smtc_modem_run_engine( );

        if( periodic_message_flag ) {
            periodic_message_flag = false;
            sensor_read();

            if( txdone_counter >= LINK_CHECK_RATIO_THRESHOLD ) {
                txdone_counter = 0;
                SMTC_HAL_TRACE_INFO( "Send link check\n" );
                smtc_modem_lorawan_request_link_check( STACK_ID );
            }

            if( is_joined() ){
                tx_measurement_data_keep_alive();
            }
        }


        // sleep_time_ms -= ( hal_rtc_get_time_ms( ) - start );
        sleep_time_ms = smtc_modem_run_engine( );
        hal_mcu_set_sleep_for_ms( sleep_time_ms );
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/**
 * @brief User callback for modem event
 *
 *  This callback is called every time an event ( see smtc_modem_event_t ) appears in the modem.
 *  Several events may have to be read from the modem when this callback is called.
 */
static void get_event( void ) {
    //SMTC_HAL_TRACE_MSG_COLOR( "get_event () callback\n", HAL_DBG_TRACE_COLOR_BLUE );

    smtc_modem_event_t current_event;
    uint8_t            event_pending_count;
    uint8_t            stack_id = STACK_ID;

    // Continue to read modem event until all event has been processed
    do {
        // Read modem event
        smtc_modem_get_event( &current_event, &event_pending_count );

        switch( current_event.event_type ) {
        case SMTC_MODEM_EVENT_RESET:
            SMTC_HAL_TRACE_INFO( "Event received: RESET\n" );

            // Set user credentials
            smtc_modem_set_deveui( stack_id, settings.deveui );
            smtc_modem_set_joineui( stack_id, settings.joineui );
            smtc_modem_set_nwkkey( stack_id, settings.nwkey );
            // Set user region
            smtc_modem_set_region( stack_id, settings.region );
            // Schedule a Join LoRaWAN network
            smtc_modem_join_network( stack_id );
            break;

        case SMTC_MODEM_EVENT_ALARM:
            SMTC_HAL_TRACE_INFO( "Event received: ALARM\n" );

            break;

        case SMTC_MODEM_EVENT_JOINED:
            SMTC_HAL_TRACE_INFO( "Event received: JOINED\n" );
            SMTC_HAL_TRACE_INFO( "Modem is now joined \n" );

            SMTC_HAL_TRACE_INFO( "Send hello_world message \n" );
            char data[] = "hello_world";
            smtc_modem_request_uplink( STACK_ID, 102, false, ( uint8_t* )&data, sizeof( data ) );

            break;

        case SMTC_MODEM_EVENT_TXDONE:
            SMTC_HAL_TRACE_INFO( "Event received: TXDONE\n" );
            SMTC_HAL_TRACE_INFO( "Transmission done \n" );
            tx_pending = 0;
            break;

        case SMTC_MODEM_EVENT_DOWNDATA:
            SMTC_HAL_TRACE_INFO( "Event received: DOWNDATA\n" );
            rx_payload_size = ( uint8_t ) current_event.event_data.downdata.length;
            memcpy( rx_payload, current_event.event_data.downdata.data, rx_payload_size );
            SMTC_HAL_TRACE_PRINTF( "Data received on port %u\n", current_event.event_data.downdata.fport );
            SMTC_HAL_TRACE_ARRAY( "Received payload", rx_payload, rx_payload_size );
            break;

        case SMTC_MODEM_EVENT_UPLOADDONE:
            SMTC_HAL_TRACE_INFO( "Event received: UPLOADDONE\n" );

            break;

        case SMTC_MODEM_EVENT_SETCONF:
            SMTC_HAL_TRACE_INFO( "Event received: SETCONF\n" );

            break;

        case SMTC_MODEM_EVENT_MUTE:
            SMTC_HAL_TRACE_INFO( "Event received: MUTE\n" );

            break;

        case SMTC_MODEM_EVENT_STREAMDONE:
            SMTC_HAL_TRACE_INFO( "Event received: STREAMDONE\n" );

            break;

        case SMTC_MODEM_EVENT_JOINFAIL:
            SMTC_HAL_TRACE_INFO( "Event received: JOINFAIL\n" );
            SMTC_HAL_TRACE_WARNING( "Join request failed \n" );
            break;

        case SMTC_MODEM_EVENT_TIME:
            SMTC_HAL_TRACE_INFO( "Event received: TIME\n" );
            break;

        case SMTC_MODEM_EVENT_TIMEOUT_ADR_CHANGED:
            SMTC_HAL_TRACE_INFO( "Event received: TIMEOUT_ADR_CHANGED\n" );
            break;

        case SMTC_MODEM_EVENT_NEW_LINK_ADR:
            SMTC_HAL_TRACE_INFO( "Event received: NEW_LINK_ADR\n" );
            break;

        case SMTC_MODEM_EVENT_LINK_CHECK:
            SMTC_HAL_TRACE_INFO( "Event received: LINK_CHECK\n" );

            //Link Check Example
            if( current_event.event_data.link_check.status == SMTC_MODEM_EVENT_LINK_CHECK_RECEIVED ) {
                SMTC_HAL_TRACE_INFO( "Link check successfull\n" );
                link_check_attempts = 0;
            } else {
                SMTC_HAL_TRACE_INFO( "Link check failed\n" );
                link_check_attempts++;

                if( link_check_attempts >= LINK_CHECK_ATTEMPTS_THRESHOLD ) {
                    SMTC_HAL_TRACE_INFO( "Link check failed %d times, rejoin network\n", link_check_attempts );
                    smtc_modem_leave_network( STACK_ID );
                    led_set( &led_b, LED_ON );
                    // TimerStop( &measurement_timer );
                    // TimerStop( &send_cyclic_data_timer );
                    smtc_modem_join_network( STACK_ID );
                }
            }

            break;

        case SMTC_MODEM_EVENT_ALMANAC_UPDATE:
            SMTC_HAL_TRACE_INFO( "Event received: ALMANAC_UPDATE\n" );
            break;

        case SMTC_MODEM_EVENT_USER_RADIO_ACCESS:
            SMTC_HAL_TRACE_INFO( "Event received: USER_RADIO_ACCESS\n" );
            break;

        case SMTC_MODEM_EVENT_CLASS_B_PING_SLOT_INFO:
            SMTC_HAL_TRACE_INFO( "Event received: CLASS_B_PING_SLOT_INFO\n" );
            break;

        case SMTC_MODEM_EVENT_CLASS_B_STATUS:
            SMTC_HAL_TRACE_INFO( "Event received: CLASS_B_STATUS\n" );
            break;

        default:
            SMTC_HAL_TRACE_ERROR( "Unknown event %u\n", current_event.event_type );
            break;
        }
    } while( event_pending_count > 0 );
}

/**
 * @brief Join status of the product
 *
 * @return Return 1 if the device is joined else 0
 */
static bool is_joined( void ) {
    uint32_t status = 0;
    smtc_modem_get_status( STACK_ID, &status );

    if( ( status & SMTC_MODEM_STATUS_JOINED ) == SMTC_MODEM_STATUS_JOINED ) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief User callback for button EXTI
 *
 * @param context Define by the user at the init
 */
static void user_button_callback( void* context ) {
    SMTC_HAL_TRACE_INFO( "Button pushed\n" );
    led_set( &led_b, LED_OFF );
    led_blink( &led_b, 100, 1000 );
    ( void ) context;  // Not used in the example - avoid warning
}

#if defined( LR11XX )
/**
 * @brief Wi-Fi scanning function
 */
static void wifi_scan( void ) {
    SMTC_HAL_TRACE_PRINTF( "\n-----------------WIFI Scan-----------------\n" );
    uint32_t i;
    uint32_t wifi_counter = 0;

    lr11xx_wifi_extended_full_result_t wifi_scan_result;
    lr11xx_status_t ret;
    uint8_t nb_results;
    ret = lr11xx_wifi_scan( modem_radio.ral.context, LR11XX_WIFI_TYPE_SCAN_B_G_N, 0x3FFF, LR11XX_WIFI_SCAN_MODE_UNTIL_SSID, 12, 3, 110, true );
    hal_mcu_delay_ms( 110 );

    if( ret != LR11XX_STATUS_OK ) {
        SMTC_HAL_TRACE_PRINTF( "WIFI scan error\n" );
    }

    ret = lr11xx_wifi_get_nb_results( modem_radio.ral.context, &nb_results );

    if( nb_results > 0 ) {
        SMTC_HAL_TRACE_PRINTF( "Listing WIFI networks: %d\n", nb_results );
    } else {
        SMTC_HAL_TRACE_PRINTF( "No WIFI networks found: %d\n", nb_results );
    }

    for( i = wifi_counter; i < nb_results; i++ ) {
        ret = lr11xx_wifi_read_extended_full_results( modem_radio.ral.context, i, 1, &wifi_scan_result );
        SMTC_HAL_TRACE_PRINTF( "wifi %d: SSID: %s, ", i, wifi_scan_result.ssid_bytes );
        SMTC_HAL_TRACE_PRINTF( "mac: %02x:%02x:%02x:%02x:%02x:%02x, rssi: %d\n", wifi_scan_result.mac_address_3[0], wifi_scan_result.mac_address_3[1], wifi_scan_result.mac_address_3[2], wifi_scan_result.mac_address_3[3], wifi_scan_result.mac_address_3[4], wifi_scan_result.mac_address_3[5], wifi_scan_result.rssi );
    }

    if( ret != LR11XX_STATUS_OK ) {
        SMTC_HAL_TRACE_PRINTF( "LR1110 ERROR \n" );
    }

    wifi_counter = nb_results;
    wifi_count = nb_results;
}

static void gps_snap( void ) {
    SMTC_HAL_TRACE_PRINTF( "\n-----------------GPS snap----------------\n" );
    hal_gpio_set_value( PB_0, 1 );
    lr11xx_status_t ret;
    uint8_t nb_sat;
    SMTC_HAL_TRACE_PRINTF( "lr11xx_gnss_scan_autonomous...\n" );
    ret = lr11xx_gnss_scan_autonomous( modem_radio.ral.context, 1338595200, LR11XX_GNSS_OPTION_BEST_EFFORT, 1, 6 );
    hal_mcu_delay_ms( 5000 );
    ret = lr11xx_gnss_get_nb_detected_satellites( modem_radio.ral.context, &nb_sat );
    SMTC_HAL_TRACE_PRINTF( "number of satellites found: %d\n\n", nb_sat );

    if( ret != LR11XX_STATUS_OK ) {
        SMTC_HAL_TRACE_PRINTF( "LR1110 ERROR \n" );
    }

    gnss_count = nb_sat;
    hal_gpio_set_value( PB_0, 0 );
}
#endif

static void button_led_test( void ) {
    hal_gpio_init_out( PC_1, 1 );
    //Init Button
    hal_gpio_init_in( PA_0, BSP_GPIO_PULL_MODE_UP, BSP_GPIO_IRQ_MODE_RISING, NULL );
    user_button_state = hal_gpio_get_value( PA_0 );
    SMTC_HAL_TRACE_PRINTF( "UserButtonState: %x\n", user_button_state );

    //If Button is pressed, then LED becomes blue and the program is waiting until the button is released
    if( user_button_state == 0 ) {
        SMTC_HAL_TRACE_PRINTF( "UserButton pressed\n" );
        hal_gpio_set_value( PC_1, 0 );

        while( user_button_state == 0 ) {
            hal_mcu_delay_ms( 100 );
            user_button_state = hal_gpio_get_value( PA_0 );
        }

        hal_gpio_set_value( PC_1, 1 );
    }
}

static void tx_measurement_data_keep_alive() {
    if( tx_pending == 1 ) {
        SMTC_HAL_TRACE_PRINTF( "tx_pending, failed to send tx_measurement_data_keep_alive\n" );
        return;
    }

    uint8_t idx = 0;
    uint8_t tx[50];
    tx[idx++] = 0;
    tx[idx++] = 1;
    tx[idx++] = ( temp + 5 / 10 ) >> 8; //convert to 1/100C with rounding
    tx[idx++] = ( temp + 5 / 10 );
    tx[idx++] = ( rh + 5 / 10 ) >> 8;
    tx[idx++] = ( rh + 5 / 10 );
    tx[idx++] = acc_axes[0] >> 8;
    tx[idx++] = acc_axes[0];
    tx[idx++] = acc_axes[1] >> 8;
    tx[idx++] = acc_axes[1];
    tx[idx++] = acc_axes[2] >> 8;
    tx[idx++] = acc_axes[2];
#if defined( LR11XX )
    tx[idx++] = wifi_count;
    tx[idx++] = gnss_count;
    //reset GNSS, Wi-Fi counter
    wifi_count = 0;
    gnss_count = 0;
#endif
    tx[0] = idx;
    smtc_modem_return_code_t ret;

    if( is_joined() ) {
        ret = smtc_modem_request_uplink( STACK_ID, DEFAULT_UL_PORT, false, tx, idx );
    } else {
        SMTC_HAL_TRACE_PRINTF( "Modem not joined \n" );
        return;
    }

    if( ret == SMTC_MODEM_RC_OK ) {
        tx_pending = 1;
    }

}

static void init_func() {
    settings_init();

    settings_print();

    //Red LED on FMLR module
    led_init( &led_fmlr, PB_12, LED_ACTIVE_LOW );
    //RGB LED on devboard
    led_init( &led_r, PH_0, LED_ACTIVE_LOW );
    led_init( &led_g, PH_1, LED_ACTIVE_LOW );
    led_init( &led_b, PC_1, LED_ACTIVE_LOW );

    //Init user button
    button_init( &button_handle, PA_0, false, user_button_callback, NULL );

    //Init sht3x temperature and humidity sensor
    SMTC_HAL_TRACE_PRINTF( "SHT sensor test\n" );
    bool ret = 0;
    ret = sht3x_init( 1, ( 0x45 << 1 ) );

    if( ret != 0 ) {
        SMTC_HAL_TRACE_PRINTF( "sht3x init done\n" );
    } else {
        SMTC_HAL_TRACE_PRINTF( "sht3x init failed\n" );
    }

    ret = lis2dw_init( 1, 0x32, RANGE_2G, M2_14BIT );

    if( ret != 0 ) {
        SMTC_HAL_TRACE_PRINTF( "lis2dw init done\n" );
    } else {
        SMTC_HAL_TRACE_PRINTF( "lis2dw init failed\n" );
    }

    //Display nice RGB pattern
    SMTC_HAL_TRACE_INFO( "LED Test\n" );

    led_set( &led_fmlr, LED_ON );
    hal_mcu_delay_ms( 500 );
    led_set( &led_fmlr, LED_OFF );

    led_set( &led_r, LED_ON );
    hal_mcu_delay_ms( 500 );
    led_set( &led_r, LED_OFF );

    led_set( &led_g, LED_ON );
    hal_mcu_delay_ms( 500 );
    led_set( &led_g, LED_OFF );

    led_set( &led_b, LED_ON );
    hal_mcu_delay_ms( 500 );
    led_set( &led_b, LED_OFF );
}

static void sensor_read( void ) {
    SMTC_HAL_TRACE_PRINTF( "----- sensor_read -----\n" );

    if( sht3x_read_temp( &temp, &rh ) ) {
        SMTC_HAL_TRACE_PRINTF( "Temperature: %d.%d C, Humidity: %d.%d% \n", temp / 1000, temp % 1000, rh / 1000, rh % 1000 );
    } else {
        SMTC_HAL_TRACE_PRINTF( "sht31 read failed\n" );
    }

    if( lis2dw_read( acc_axes ) ) {
        SMTC_HAL_TRACE_PRINTF( "X: %d, Y: %d, Z: %d \n", acc_axes[0], acc_axes[1], acc_axes[2] );
    } else {
        SMTC_HAL_TRACE_PRINTF( "lis2dw read failed\n" );
    }
#if defined( LR11XX )
            wifi_scan();
            gps_snap();
#endif

}
