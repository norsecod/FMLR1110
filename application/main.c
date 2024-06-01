#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type

#include "smtc_modem_api.h"
#include "smtc_modem_utilities.h"
#include "stm32l0xx_hal.h"

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
#include "lr11xx_gnss_types.h"
#include "ral_lr11xx.h"

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

#define PERIODIC_TIMER_PERIOD 60000
#define ALARM_TIMER_PERIOD  60000


#define LINK_CHECK_RATIO_THRESHOLD 50
#define LINK_CHECK_ATTEMPTS_THRESHOLD 10

/**
 * Stack id value (multistacks modem is not yet available)
 */
#define STACK_ID 0
#define MAX_SATELLITES 32

const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE( NULL );
#define RADIO   "LR11XX"
#define RESULT_BUFFER_SIZE 256
#define LR11XX_GNSS_SNR_TO_CNR_OFFSET ( 31 )




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

    bool hastydata1 = false;
    bool hastydata2 = false;
    bool hastydata3 = false;
    bool tapflag = false;
    float temp = 0.0f;
    float Voltage = 0;
    float VDR = 16/3.3;
    int door = 0;
    int prev_door = -1;
    int water = 0;
    int prev_water = -1;
    int prev_tap = 3;
    
    
    static uint8_t       rx_payload[255]      = { 0 };  // Buffer for rx payload
    static uint8_t       rx_payload_size      = 0;      // Size of the payload in the rx_payload buffer
    extern settings_t    settings;                      //holds the LoRaWAN keys
    uint32_t txdone_counter = 0;
    uint32_t link_check_attempts = 0;
    static uint32_t tx_pending;
    //uint16_t axisxyz[3];          //variable used for debuggng adxl343

lr11xx_gnss_detected_satellite_t gnss_data_buffer[RESULT_BUFFER_SIZE];
lr11xx_gnss_context_status_bytestream_t context_status_buffer;
lr11xx_gnss_context_status_t context_status;
lr11xx_gnss_timings_t gnss_timings;
lr11xx_gnss_solver_assistance_position_t assistance_position;
    
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

    static bool is_joined( void );
    static void get_event( void );
    void gps_snap( void );


    static TimerEvent_t periodic_timer;
    static void periodic_timer_cb( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "periodic_timer_cb\n\r" );
    periodic_message_flag = true;
    TimerStart( &periodic_timer );    
}
    TimerEvent_t water_timer;
    static void water_timer_cb( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "water_timer_cb\n\r" );
    hastydata1 = 0;
    PC10Callback(NULL);
      
}
    TimerEvent_t door_timer;
    static void door_timer_cb( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "door_timer_cb\n\r" );
    hastydata2 = 0;
    PC11Callback(NULL);
    
}

    TimerEvent_t tap_timer;
    static void tap_timer_cb( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "tap_timer_cb\n\r" );
    hastydata3 = 0;
    
    PA15Callback(NULL);
    
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
    hal_rtc_init();

    // Init the modem and use get_event as event callback, please note that the callback will be
    // called immediately after the first call to smtc_modem_run_engine because of the reset detection
//    SMTC_HAL_TRACE_PRINTF("calling: smtc_modem_init\n\r");
    smtc_modem_init( &modem_radio, &get_event );
    smtc_modem_version_t version;
    smtc_modem_get_modem_version( &version );
    settings_init();    //inits the DEVeui[16bit], JoinEUI[16bit], nwkey[32bit] and region[4bit]
    //settings_print();



    SMTC_HAL_TRACE_INFO( "Modem driver version: %d.%d.%d\n\r", version.major, version.minor, version.patch );
 
    GPIO_Init();            //initializes GPIO pins
    hal_i2c_init(1,PB_9,PB_6);

    hal_mcu_enable_irq( );  //enables IRQ again
    hal_gpio_irq_enable();  //enables all gpio interrupts

  

    //periodic timer 
    TimerInit( &periodic_timer, &periodic_timer_cb );
    TimerSetValue( &periodic_timer, PERIODIC_TIMER_PERIOD );
    TimerSetContext( &periodic_timer, NULL );
    TimerStart(&periodic_timer);

    //timer activated when water alarm
    TimerInit( &water_timer, &water_timer_cb );
    TimerSetValue( &water_timer, ALARM_TIMER_PERIOD );
    TimerSetContext( &water_timer, NULL );

    //timer activated when door alarm
    TimerInit( &door_timer, &door_timer_cb );
    TimerSetValue( &door_timer, ALARM_TIMER_PERIOD );
    TimerSetContext( &door_timer, NULL );

        //timer activated when tap is detected
    TimerInit( &tap_timer, &tap_timer_cb );
    TimerSetValue( &tap_timer, ALARM_TIMER_PERIOD );
    TimerSetContext( &tap_timer, NULL );
 

    lr11xx_status_t ret;
    lr11xx_system_version_t lr11xx_version;
    ret = lr11xx_system_get_version( modem_radio.ral.context, &lr11xx_version );
    SMTC_HAL_TRACE_PRINTF("LR11xx Version: HW: %x, type: %x, fw: %x\n\r", lr11xx_version.hw, lr11xx_version.type, lr11xx_version.fw);
    hal_mcu_delay_ms(1000);
    adxl_init();
    hal_mcu_delay_ms(1000);
    

    while(true) 
    {

        uint32_t start = hal_rtc_get_time_ms( );

        // Execute modem runtime, this function must be recalled in sleep_time_ms (max value, can be recalled sooner)
        uint32_t sleep_time_ms = smtc_modem_run_engine( );

        //adxl_read_acceleration(axisxyz);                                                  //used to debug adxl
        //SMTC_HAL_TRACE_PRINTF("x:%d y:%d z:%d \n\r",axisxyz[0],axisxyz[1],axisxyz[2]);

        //sleep_time_ms -= ( hal_rtc_get_time_ms( ) - start );
        sleep_time_ms = smtc_modem_run_engine( );
        hal_mcu_set_sleep_for_ms( sleep_time_ms );
    
    if( periodic_message_flag)  //reads sensors and sends the data over LoRaWAN after cntup==3 (60seconds) and is_joined
        {   

            hal_mcu_delay_ms(20);
            sensor_read();
            hal_mcu_delay_ms(20);
            sendData(temp, Voltage,door,water,tapflag);


            SMTC_HAL_TRACE_PRINTF("Temp: %.2f°C Voltage: %.2fV door: %s water: %s tap: %s \n\r",
                      temp,
                      Voltage,
                      door == 1 ? "high" : "low",
                      water == 1 ? "high" : "low",
                      tapflag == 1 ? "high" : "low");    
            PC10Callback(NULL); //checks status on PC10 watersensor
            PC11Callback(NULL); //checks status on PC11 doormagnet
            PA15Callback(NULL); //checks status on PA15 accelerometer
            periodic_message_flag = false;  //sets flag to false

            if( txdone_counter >= LINK_CHECK_RATIO_THRESHOLD )  //link check after 50 transmits
            {
                txdone_counter = 0;                            //counter set to 0 and star counting to 50 again
                SMTC_HAL_TRACE_INFO( "Send link check\n\r" );
                smtc_modem_lorawan_request_link_check( STACK_ID );  //checks the link on stack id 0
            }
        }

    
    

    }


}


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */




void gps_snap(void) {
    SMTC_HAL_TRACE_PRINTF("\n-----------------GPS snap----------------\n\r");
   

    // Activate the active antenna
    hal_gpio_set_value(PB_0, 1);
    hal_mcu_delay_ms(100);
    
    lr11xx_status_t ret;
    uint8_t nb_sat = 0;
    uint8_t result_buffer[RESULT_BUFFER_SIZE];
    uint16_t result_size = 0;

    // Get the current GNSS time
    uint32_t gnss_time = get_current_gnss_time();
    SMTC_HAL_TRACE_PRINTF("Starting GNSS scan with time: %u\n\r", gnss_time);

    // Set the GNSS constellations to use (GPS and BeiDou)
    uint32_t constellations = LR11XX_GNSS_GPS_MASK | LR11XX_GNSS_BEIDOU_MASK;
    ret = lr11xx_gnss_set_constellations_to_use(modem_radio.ral.context, constellations);
    if (ret != LR11XX_STATUS_OK) {
        SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Failed to set constellations\n\r");
        hal_gpio_set_value(PB_0, 0);
        return;
    }

    // Set the GNSS scan mode
    lr11xx_gnss_scan_mode_t scan_mode = LR11XX_GNSS_SCAN_MODE_0_SINGLE_SCAN_LEGACY;
    ret = lr11xx_gnss_set_scan_mode(modem_radio.ral.context, scan_mode);
    if (ret != LR11XX_STATUS_OK) {
        SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Failed to set scan mode\n\r");
        hal_gpio_set_value(PB_0, 0);
        return;
    }

    // Set assistance position to Høgskole i Østfold, Fredrikstad, Norway
    assistance_position.latitude = 59.2184;  // Latitude for Høgskole i Østfold
    assistance_position.longitude = 10.9291;  // Longitude for Høgskole i Østfold
    ret = lr11xx_gnss_set_assistance_position(modem_radio.ral.context, &assistance_position);
    if (ret != LR11XX_STATUS_OK) {
        SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Failed to set assistance position\n\r");
        hal_gpio_set_value(PB_0, 0);
        return;
    }

    // Start the GNSS autonomous scan with best effort mode
    ret = lr11xx_gnss_scan_autonomous(modem_radio.ral.context, gnss_time, LR11XX_GNSS_OPTION_BEST_EFFORT, 1, 6);
    if (ret != LR11XX_STATUS_OK) {
        SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Failed to start autonomous scan\n\r");
        hal_gpio_set_value(PB_0, 0);
        return;
    }

    // Wait for the scan to complete
    hal_mcu_delay_ms(5000);

    // Get the number of detected satellites
    ret = lr11xx_gnss_get_nb_detected_satellites(modem_radio.ral.context, &nb_sat);
    if (ret != LR11XX_STATUS_OK) {
        SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Failed to get number of detected satellites\n\r");
    } else {
        SMTC_HAL_TRACE_PRINTF("Number of satellites found: %d\n\r", nb_sat);
    }

    // Get the size of the GNSS results
    ret = lr11xx_gnss_get_result_size(modem_radio.ral.context, &result_size);
    if (ret == LR11XX_STATUS_OK && result_size <= RESULT_BUFFER_SIZE) {
        // Read the GNSS results
        ret = lr11xx_gnss_read_results(modem_radio.ral.context, result_buffer, result_size);
        if (ret == LR11XX_STATUS_OK) {
            // Print the raw result buffer data
            SMTC_HAL_TRACE_PRINTF("Raw GNSS result buffer: ");
            for (uint16_t i = 0; i < result_size; i++) {
                SMTC_HAL_TRACE_PRINTF("%02X ", result_buffer[i]);
            }
            SMTC_HAL_TRACE_PRINTF("\n\r");

            // Parse and store satellite information in the buffer
            for (uint8_t i = 0; i < nb_sat; i++) {
                gnss_data_buffer[i].satellite_id = result_buffer[i * 8];
                gnss_data_buffer[i].cnr = result_buffer[i * 8 + 1] + LR11XX_GNSS_SNR_TO_CNR_OFFSET;
                gnss_data_buffer[i].doppler = (int16_t)((result_buffer[i * 8 + 2] << 8) | result_buffer[i * 8 + 3]);

                SMTC_HAL_TRACE_PRINTF("Parsed Data - Satellite ID: %d, CNR: %d, Doppler: %d\n\r",
                                      gnss_data_buffer[i].satellite_id, gnss_data_buffer[i].cnr, gnss_data_buffer[i].doppler);
            }

            // Get the GNSS context status
            ret = lr11xx_gnss_get_context_status(modem_radio.ral.context, context_status_buffer);
            if (ret == LR11XX_STATUS_OK) {
                // Parse the GNSS context status buffer
                ret = lr11xx_gnss_parse_context_status_buffer(context_status_buffer, &context_status);
                if (ret == LR11XX_STATUS_OK) {
                    SMTC_HAL_TRACE_PRINTF("Parsed GNSS context status\n\r");
                } else {
                    SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Failed to parse GNSS context status\n\r");
                }
            } else {
                SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Failed to get GNSS context status\n\r");
            }

            // Get the timings of the last GNSS scan
            ret = lr11xx_gnss_get_timings(modem_radio.ral.context, &gnss_timings);
            if (ret == LR11XX_STATUS_OK) {
                SMTC_HAL_TRACE_PRINTF("GNSS scan timings - Acquisition: %u ms, Analysis: %u ms\n\r",
                                      gnss_timings.radio_ms, gnss_timings.computation_ms);
            } else {
                SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Failed to get GNSS scan timings\n\r");
            }
        } else {
            SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Failed to read GNSS results\n\r");
        }
    } else {
        SMTC_HAL_TRACE_PRINTF("LR11XX ERROR: Invalid result size\n\r");
    }

    // Deactivate the active antenna
    hal_gpio_set_value(PB_0, 0);
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
            break;

        case SMTC_MODEM_EVENT_TXDONE:
            SMTC_HAL_TRACE_INFO( "Event received: TXDONE\n\r" );
            SMTC_HAL_TRACE_INFO( "Transmission done \n\r" );
            tx_pending = 0;
          

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
void PC10Callback(void* context)
{
    // Check if the interrupt came from PC_10
    if (hal_gpio_get_value(PC_10) == GPIO_PIN_SET) // Rising edge detected
    {
if (hastydata1 == 0)
{
    water = 1; // Potentially change the state of `water`

    // Check if the state of `water` has changed since the last check
    if (water != prev_water)
    {
        // If `water` has changed, call the wateralarm function with the new state
        wateralarm(water); // Handle door opened

        // Update prev_water to the current state of `water`
        prev_water = water;
    }

    TimerStart(&water_timer);
    hastydata1 = 1; // Prevent re-triggering
}
    }
    else // Falling edge detected
    {
        if (hastydata1 == 0)
        {
            water = 0;
            

        }
    }
}

void PC11Callback(void* context)
{
    // Check if the interrupt came from PC_11
    if (hal_gpio_get_value(PC_11) == GPIO_PIN_SET) // Rising edge detected
    {
        if (hastydata2 == 0)
        {
            door = 1;

            if (door != prev_door)
            {
                // If `door` has changed, call the wateralarm function with the new state
                dooralarm(door); // Handle door opened

                // Update prev_door to the current state of `door`
                prev_door = door;
            }
        
            TimerStart(&door_timer);
            hastydata2 = 1; // Prevent re-triggering
        }
    }
    else if(hal_gpio_get_value(PC_11) == GPIO_PIN_RESET)// Falling edge detected
    {
        if (hastydata2 == 0)
        {
            door = 0;
        
        }
    }
}
void PA15Callback(void* context )
{   uint8_t data;
    
    adxl_read(0x30, &data, 1);
  
   
 if ((data>>5)& 0x01) 
  {
   SMTC_HAL_TRACE_PRINTF("double tap detected\n\r");
    tapflag = true;
        if (hastydata3 == 0)
    {
       
     hastydata3=1;
    
    if (tapflag != prev_tap)
    {
        tapalarm(tapflag);
        
        prev_tap = tapflag;
        
    }
        TimerStart(&tap_timer);
       
    } 
  }
 else if ((data>>6) & 0x01)
  {
   SMTC_HAL_TRACE_PRINTF("Single tap detected\n\r");
   tapflag = true;
    if (hastydata3 == 0)
    {
       
        hastydata3=1;
    
        if (tapflag != prev_tap)
        {
            tapalarm(tapflag);
        
            prev_tap = tapflag;
        
        }

        TimerStart(&tap_timer);
       
    } 
  }
  else {
        
    tapflag=false;
  }


}

