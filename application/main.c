#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type

#include "smtc_modem_api.h"
#include "smtc_modem_utilities.h"

#include "smtc_modem_hal.h"
#include "smtc_hal_dbg_trace.h"

#include "smtc_hal_mcu.h"
#include "smtc_hal_gpio.h"
#include "smtc_hal_rtc.h"
#include "smtc_hal_i2c.h"

#include "stm32l0xx_hal_adc.h"
#include "timer.h"

#include "ralf_lr11xx.h"
#include "lr11xx_gnss.h"

#include "cayenne_lpp.h"
#include "functions.h"


#include "string.h"

#include "settings.h"
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */
#define DEFAULT_UL_PORT         7
#define DEFAULT_DL_PORT         100

#define PERIODIC_TIMER_PERIOD 20000
#define ALARM_TIMER_PERIOD  60000

#define LINK_CHECK_RATIO_THRESHOLD 50
#define LINK_CHECK_ATTEMPTS_THRESHOLD 10

/**
 * Stack id value (multistacks modem is not yet available)
 */
#define STACK_ID 0


const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE( NULL );
#define RADIO   "LR11XX"




/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */ 
    ADC_HandleTypeDef hadc;
    static bool periodic_message_flag;
    static bool water_alarm_flag;
    static bool door_alarm_flag;
    static bool hastydata1 = false;
    static bool hastydata2 = false;
    static bool firstflag = true;

    float temp = 0.0f;
    float Voltage = 0;
    float VDR = 16/3.3;
    float ADCmeas = 0.0f;
    int Door = 0;
    int water = 0;
    int cntup = 0;
    static uint8_t gnss_count;
    static uint8_t       rx_payload[255]      = { 0 };  // Buffer for rx payload
    static uint8_t       rx_payload_size      = 0;      // Size of the payload in the rx_payload buffer
    extern settings_t    settings;
    uint32_t txdone_counter = 0;
    uint32_t link_check_attempts = 0;
    static uint32_t tx_pending;
    static int tx_mine = 0;

    
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

    static bool is_joined( void );
    static void get_event( void );
    static void gps_snap( void );


    static TimerEvent_t periodic_timer;
    static void periodic_timer_cb( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "periodic_timer_cb\n\r" );
    periodic_message_flag = true;
    TimerStart( &periodic_timer );    
}
    static TimerEvent_t water_timer;
    static void water_timer_cb( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "water_timer_cb\n\r" );
    water_alarm_flag = true;
      
}
    static TimerEvent_t door_timer;
    static void door_timer_cb( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "door_timer_cb\n\r" );
    door_alarm_flag = true;
   
    
}

  
/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */ 


int main( void ) 
{
    // Disable IRQ to avoid unwanted behaviour during init
    hal_mcu_disable_irq( );

    // Configure all the uC periph (clock, gpio, timer, ...)
 
    hal_mcu_init();
    SMTC_HAL_TRACE_PRINTF("_______________________________________________________________________________\n\r");
    SMTC_HAL_TRACE_PRINTF("Miromico-FMLR1110\n\n\r");
    SMTC_HAL_TRACE_PRINTF("Radio:%s\n\r", RADIO);

    // Init the modem and use get_event as event callback, please note that the callback will be
    // called immediately after the first call to smtc_modem_run_engine because of the reset detection
//    SMTC_HAL_TRACE_PRINTF("calling: smtc_modem_init\n\r");
    smtc_modem_init( &modem_radio, &get_event );
    smtc_modem_version_t version;
    smtc_modem_get_modem_version( &version );
    settings_init();
    //settings_print();

    SMTC_HAL_TRACE_INFO( "Modem driver version: %d.%d.%d\n\r", version.major, version.minor, version.patch );
 
    hal_gpio_init_out(PC_7,0);
    hal_gpio_init_out(PC_1,1);
    hal_gpio_init_out(PB_0,0);
    hal_gpio_init_out(PA_3,0);
    hal_gpio_init_in( PB_1, GPIO_PULLDOWN, GPIO_MODE_IT_RISING, 1);
    hal_gpio_init_in( PB_2, GPIO_PULLDOWN, GPIO_MODE_IT_RISING, 1);
    
    hal_mcu_enable_irq( );

    hal_gpio_irq_attach(&PB1_Callback);
    hal_gpio_irq_attach(&PB2_Callback);



    //periodic timer 
    TimerInit( &periodic_timer, &periodic_timer_cb );
    TimerSetValue( &periodic_timer, PERIODIC_TIMER_PERIOD );
    TimerSetContext( &periodic_timer, NULL );
    TimerStart(&periodic_timer);

    TimerInit( &water_timer, &water_timer_cb );
    TimerSetValue( &water_timer, ALARM_TIMER_PERIOD );
    TimerSetContext( &water_timer, NULL );


    TimerInit( &door_timer, &door_timer_cb );
    TimerSetValue( &door_timer, ALARM_TIMER_PERIOD );
    TimerSetContext( &door_timer, NULL );
 

    lr11xx_status_t ret;
    lr11xx_system_version_t lr11xx_version;
    ret = lr11xx_system_get_version( modem_radio.ral.context, &lr11xx_version );
    SMTC_HAL_TRACE_PRINTF("LR11xx Version: HW: %x, type: %x, fw: %x\n\r", lr11xx_version.hw, lr11xx_version.type, lr11xx_version.fw);
    SMTC_HAL_TRACE_PRINTF("------------<first sensor read>------------");
    sensor_read();

    while(true) 
    {

        uint32_t start = hal_rtc_get_time_ms( );

        // Execute modem runtime, this function must be recalled in sleep_time_ms (max value, can be recalled sooner)
        uint32_t sleep_time_ms = smtc_modem_run_engine( );
        
    
        
    if( periodic_message_flag ) 
    {
           
        sensor_read();
        SMTC_HAL_TRACE_PRINTF("Temp: %.2f°C ADC: %.2fV Voltage: %.2fV Door: %s water: %s cnutp: %d hasty1: %d hasty2: %d\n\r",
                      temp, ADCmeas,
                      Voltage,
                      Door == 1 ? "high" : "low",
                      water == 1 ? "high" : "low",
                      cntup,
                      hastydata1,
                      hastydata2);         
        
        cntup ++;
        periodic_message_flag = false;

       if( txdone_counter >= LINK_CHECK_RATIO_THRESHOLD ) 
        {
            txdone_counter = 0;
            SMTC_HAL_TRACE_INFO( "Send link check\n\r" );
            smtc_modem_lorawan_request_link_check( STACK_ID );
        }


    }

        //sleep_time_ms -= ( hal_rtc_get_time_ms( ) - start );
        sleep_time_ms = smtc_modem_run_engine( );
        hal_mcu_set_sleep_for_ms( sleep_time_ms );

    if( cntup ==3 && is_joined() == 1)
        {   
            sensor_read();
            sendData(temp,Voltage);
            cntup = 0;

            SMTC_HAL_TRACE_PRINTF("Temp: %.2f°C ADC: %.2fV Voltage: %.2fV Door: %s water: %s cnutp: %d hasty1: %d hasty2: %d\n\r",
                      temp, ADCmeas,
                      Voltage,
                      Door == 1 ? "high" : "low",
                      water == 1 ? "high" : "low",
                      cntup,
                      hastydata1,
                      hastydata2);    
            
        }

    if (water_alarm_flag == true)
    {
        hastydata1 = 0;
        water_alarm_flag = false;
    }        
    if (door_alarm_flag == true)
    {
        hastydata2 = 0;
        door_alarm_flag = false;
    }
    
    
    if (hal_gpio_get_value(PC_7) == 1)
    {
        hal_gpio_set_value(PC_1, 1);
        hal_gpio_set_value(PC_7, 0);

       
    }
    else
    {   
        hal_gpio_set_value(PC_1, 0);
        hal_gpio_set_value(PC_7, 1);

    }



hal_mcu_delay_ms(1000);


    }


}


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */




static void gps_snap( void ) {

    SMTC_HAL_TRACE_PRINTF( "\n-----------------GPS snap----------------\n\r" );
    hal_gpio_set_value( PB_0, 1 );
    lr11xx_status_t ret;
    uint8_t nb_sat;
    SMTC_HAL_TRACE_PRINTF( "lr11xx_gnss_scan_autonomous...\n\r" );
    ret = lr11xx_gnss_scan_autonomous( modem_radio.ral.context, 1338595200, LR11XX_GNSS_OPTION_BEST_EFFORT, 1, 6 );
    hal_mcu_delay_ms( 5000 );
    ret = lr11xx_gnss_get_nb_detected_satellites( modem_radio.ral.context, &nb_sat );
    SMTC_HAL_TRACE_PRINTF( "number of satellites found: %d\n\n\r", nb_sat );

    if( ret != LR11XX_STATUS_OK ) {
        SMTC_HAL_TRACE_PRINTF( "LR1110 ERROR \n\r" );
    }

    gnss_count = nb_sat;
    hal_gpio_set_value( PB_0, 0 );
}

static void get_event( void ) {
    //SMTC_HAL_TRACE_MSG_COLOR( "get_event () callback\n\r", HAL_DBG_TRACE_COLOR_BLUE );

    smtc_modem_event_t current_event;
    uint8_t            event_pending_count;
    uint8_t            stack_id = STACK_ID;

    // Continue to read modem event until all event has been processed
    do {
        // Read modem event
        smtc_modem_get_event( &current_event, &event_pending_count );

        switch( current_event.event_type ) {
        case SMTC_MODEM_EVENT_RESET:
            SMTC_HAL_TRACE_INFO( "Event received: RESET\n\r" );

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
            SMTC_HAL_TRACE_INFO( "Event received: ALARM\n\r" );

            break;

        case SMTC_MODEM_EVENT_JOINED:
            SMTC_HAL_TRACE_INFO( "Event received: JOINED\n\r" );
            SMTC_HAL_TRACE_INFO( "Modem is now joined \n\r" );

            SMTC_HAL_TRACE_INFO( "Send hello_world message \n\r" );
            char data[] = "hello_world";
            smtc_modem_request_uplink( STACK_ID, 102, false, ( uint8_t* )&data, sizeof( data ) );
            if (firstflag == true)
            {   SMTC_HAL_TRACE_PRINTF("------------<sending first data>------------\n\r");
                if (tx_mine == 1);
                {
                    sendData(temp, Voltage);
                }
                if (tx_mine == 2);
                {
                    wateralarm(water);
                }
                if (tx_mine == 3);
                {
                if (tx_mine == 2);
                {
                    wateralarm(water);
                };
                }
                firstflag = false;
            }

            break;

        case SMTC_MODEM_EVENT_TXDONE:
            SMTC_HAL_TRACE_INFO( "Event received: TXDONE\n\r" );
            SMTC_HAL_TRACE_INFO( "Transmission done \n\r" );
            tx_pending = 0;
            if (firstflag == true)
            {
                tx_mine ++;
            }

            break;

        case SMTC_MODEM_EVENT_DOWNDATA:
            SMTC_HAL_TRACE_INFO( "Event received: DOWNDATA\n\r" );
            rx_payload_size = ( uint8_t ) current_event.event_data.downdata.length;
            memcpy( rx_payload, current_event.event_data.downdata.data, rx_payload_size );
            SMTC_HAL_TRACE_PRINTF( "Data received on port %u \n\r", current_event.event_data.downdata.fport );
            SMTC_HAL_TRACE_ARRAY( "Received payload \n\r", rx_payload, rx_payload_size );
            break;

        case SMTC_MODEM_EVENT_UPLOADDONE:
            SMTC_HAL_TRACE_INFO( "Event received: UPLOADDONE\n\r" );

            break;

        case SMTC_MODEM_EVENT_SETCONF:
            SMTC_HAL_TRACE_INFO( "Event received: SETCONF\n\r" );

            break;

        case SMTC_MODEM_EVENT_MUTE:
            SMTC_HAL_TRACE_INFO( "Event received: MUTE\n\r" );

            break;

        case SMTC_MODEM_EVENT_STREAMDONE:
            SMTC_HAL_TRACE_INFO( "Event received: STREAMDONE\n\r" );

            break;

        case SMTC_MODEM_EVENT_JOINFAIL:
            SMTC_HAL_TRACE_INFO( "Event received: JOINFAIL\n\r" );
            SMTC_HAL_TRACE_WARNING( "Join request failed \n\r" );
            break;

        case SMTC_MODEM_EVENT_TIME:
            SMTC_HAL_TRACE_INFO( "Event received: TIME\n\r" );
            break;

        case SMTC_MODEM_EVENT_TIMEOUT_ADR_CHANGED:
            SMTC_HAL_TRACE_INFO( "Event received: TIMEOUT_ADR_CHANGED\n\r" );
            break;

        case SMTC_MODEM_EVENT_NEW_LINK_ADR:
            SMTC_HAL_TRACE_INFO( "Event received: NEW_LINK_ADR\n\r" );
            break;

        case SMTC_MODEM_EVENT_LINK_CHECK:
            SMTC_HAL_TRACE_INFO( "Event received: LINK_CHECK\n\r" );

            //Link Check Example
            if( current_event.event_data.link_check.status == SMTC_MODEM_EVENT_LINK_CHECK_RECEIVED ) {
                SMTC_HAL_TRACE_INFO( "Link check successfull\n\r" );
                link_check_attempts = 0;
            } else {
                SMTC_HAL_TRACE_INFO( "Link check failed\n" );
                link_check_attempts++;

                if( link_check_attempts >= LINK_CHECK_ATTEMPTS_THRESHOLD ) {
                    SMTC_HAL_TRACE_INFO( "Link check failed %d times, rejoin network\n\r", link_check_attempts );
                    smtc_modem_leave_network( STACK_ID );
                    
                    // TimerStop( &measurement_timer );
                    // TimerStop( &send_cyclic_data_timer );
                    smtc_modem_join_network( STACK_ID );
                }
            }

            break;

        case SMTC_MODEM_EVENT_ALMANAC_UPDATE:
            SMTC_HAL_TRACE_INFO( "Event received: ALMANAC_UPDATE\n\r" );
            break;

        case SMTC_MODEM_EVENT_USER_RADIO_ACCESS:
            SMTC_HAL_TRACE_INFO( "Event received: USER_RADIO_ACCESS\n\r" );
            break;

        case SMTC_MODEM_EVENT_CLASS_B_PING_SLOT_INFO:
            SMTC_HAL_TRACE_INFO( "Event received: CLASS_B_PING_SLOT_INFO\n\r" );
            break;

        case SMTC_MODEM_EVENT_CLASS_B_STATUS:
            SMTC_HAL_TRACE_INFO( "Event received: CLASS_B_STATUS\n\r" );
            break;

        default:
            SMTC_HAL_TRACE_ERROR( "Unknown event %u\n\r", current_event.event_type );
            break;
        }
    } while( event_pending_count > 0 );
}

static bool is_joined( void ) {
    uint32_t status = 0;
    smtc_modem_get_status( STACK_ID, &status );

    if( ( status & SMTC_MODEM_STATUS_JOINED ) == SMTC_MODEM_STATUS_JOINED ) {
        return true;
    } else {
        return false;
    }
}


