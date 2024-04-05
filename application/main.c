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
#define PERIODIC_TIMER_PERIOD_1 60000

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
    static bool          periodic_message_flag;
    static bool          periodic_message_flag1;
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

    
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

    static void MX_ADC_Init(void);
    static bool is_joined( void );
    static void get_event( void );
    static void gps_snap( void );
   /* static void sensor_read( void );
    void printCayenneLPPBuffer(const cayenne_lpp_t *lpp);
    void sendLoRaWANPacket(const uint8_t* payload, uint8_t payloadSize);
    void sendData(float temperature, float analogValue, bool digitalValue1, bool digitalValue2);
    float GETtemperature(const uint32_t id);
    float GETvoltage(ADC_HandleTypeDef *hadc);
*/
    static TimerEvent_t periodic_timer;
    static void periodic_timer_cb( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "periodic_timer_cb\n\r" );
    periodic_message_flag = true;
    TimerStart( &periodic_timer );
}

    static TimerEvent_t periodic_timer1;
    static void periodic_timer_cb1( void* context ) {
    SMTC_HAL_TRACE_PRINTF( "send cayenne\n\r" );
    periodic_message_flag1 = true;
    TimerStart( &periodic_timer1 );
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



    //periodic timer example
    TimerInit( &periodic_timer, &periodic_timer_cb );
    TimerSetValue( &periodic_timer, PERIODIC_TIMER_PERIOD );
    TimerSetContext( &periodic_timer, NULL );
    TimerStart(&periodic_timer);

    /*TimerInit( &periodic_timer1, &periodic_timer_cb1 );
    TimerSetValue( &periodic_timer1, PERIODIC_TIMER_PERIOD_1 );
    TimerSetContext( &periodic_timer1, NULL );
    TimerStart(&periodic_timer1);*/


    lr11xx_status_t ret;
    lr11xx_system_version_t lr11xx_version;
    ret = lr11xx_system_get_version( modem_radio.ral.context, &lr11xx_version );
    SMTC_HAL_TRACE_PRINTF("LR11xx Version: HW: %x, type: %x, fw: %x\n\r", lr11xx_version.hw, lr11xx_version.type, lr11xx_version.fw);
    
   

    while(true) 
    {

        uint32_t start = hal_rtc_get_time_ms( );

        // Execute modem runtime, this function must be recalled in sleep_time_ms (max value, can be recalled sooner)
        uint32_t sleep_time_ms = smtc_modem_run_engine( );
        


        
    if( periodic_message_flag ) 
    {
           
        sensor_read();
            SMTC_HAL_TRACE_PRINTF("Temp: %.2f°C ADC: %.2fV Voltage: %.2fV Door: %s water: %s cnutp: %d\n\r",
                      temp, ADCmeas,
                      Voltage,
                      Door == 1 ? "open" : "closed",
                      water == 1 ? "high" : "low",
                      cntup);
        
        periodic_message_flag = false;
        cntup ++;
    
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

          if( cntup ==3 )
        {   HAL_Delay(3000);
            sensor_read();
            sendData(temp,Voltage, Door, water);
            SMTC_HAL_TRACE_PRINTF("Temp: %.2f°C ADC: %.2fV Voltage: %.2fV Door: %s water: %s cnutp: %d\n\r",
                      temp, ADCmeas,
                      Voltage,
                      Door == 1 ? "open" : "closed",
                      water == 1 ? "high" : "low",
                      cntup);
                     cntup = 0;
            HAL_Delay(3000);
                      
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

        // Read the state of GPIO pin PB1 on interrupt
     switch (hal_gpio_get_value(PB_1))
	  	  {
	      	  case GPIO_PIN_SET:
	             water = 1;
	              break;

	          case GPIO_PIN_RESET:
	              water = 0;
	              break;
	      }

	      // Read the state of GPIO pin PB2 on interrupt
	  switch (hal_gpio_get_value(PB_2))
	      {
	          case GPIO_PIN_SET:
	             Door = 1;
	              break;

	          case GPIO_PIN_RESET:
	              Door = 0;
	              break;
	      }

hal_mcu_delay_ms(1000);


    }


}


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/*
float GETtemperature(const uint32_t id) {
    uint8_t device_address = 0x90; // 8-bit address for TC74A0
    uint8_t temperature_raw;
    float temperature;

    if (hal_i2c_read(id, device_address, 0x00, &temperature_raw) != SUCCESS) {
        // Error handling: Failed to receive temperature data
        return -128.0f; // Return an error value (out of measurement range) in case of error
    }

    // Convert raw temperature to float directly
    // The TC74A0 outputs temperature as an 8-bit signed integer.
    temperature = (float)((int8_t)temperature_raw);

    return temperature;
}

float GETvoltage(ADC_HandleTypeDef *hadc)
{	float batt = 0;

    hal_gpio_set_value(PA_3, 1); //activates mosfet
    MX_ADC_Init();
    hal_mcu_delay_ms(20);

    // Start ADC conversion
    if (HAL_ADC_Start(hadc) != HAL_OK) {
        // Handle error if ADC start fails
        // You can add error handling code here, such as logging or recovery actions
        return 128.0f; // Return 0 voltage in case of error
    }
    hal_mcu_delay_ms(20);
    // Wait for conversion to complete
    if (HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY) == HAL_OK)
    {
        uint32_t adc_value = HAL_ADC_GetValue(hadc);
        // Convert ADC value to voltage using appropriate scaling
        // For example, if your ADC reference voltage is 3.3V and resolution is 12 bits:
        batt = (3.3f * adc_value) / 4095.0f;
    }
    hal_mcu_delay_ms(20);
    HAL_ADC_DeInit(hadc);
    hal_mcu_delay_ms(20);
    hal_gpio_set_value(PA_3, 0); //turns of mosfet

    return batt;
}
*/

static void MX_ADC_Init(void)
{

  ADC_ChannelConfTypeDef sConfig = {0};

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = ENABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    mcu_panic();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    mcu_panic();
  }


}

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


/*
static void sensor_read( void ) {
    SMTC_HAL_TRACE_PRINTF( "----- sensor_read -----\n\r" );
        temp = GETtemperature(1);
        ADCmeas = GETvoltage(&hadc);
        Voltage = ADCmeas*VDR;


#if defined( LR11XX )
            
            //gps_snap();
#endif

}



void sendLoRaWANPacket(const uint8_t* payload, uint8_t payloadSize) {
    uint8_t port = 85; // Example application port
    bool confirmed = false; // False for unconfirmed messages, true for confirmed
    // Use the appropriate function to send the data over LoRaWAN
    smtc_modem_request_uplink(STACK_ID, port, confirmed, payload, payloadSize);
}

void sendData(float temperature, float analogValue, bool digitalValue1, bool digitalValue2) {
    cayenne_lpp_t lpp;
    cayenne_lpp_reset(&lpp);

    cayenne_lpp_add_temperature(&lpp, 1, temperature);         // Channel 1 for thermostat (temperature)
    cayenne_lpp_add_analog_input(&lpp, 2, analogValue);        // Channel 2 for analog value
    cayenne_lpp_add_digital_input(&lpp, 3, digitalValue1);     // Channel 3 for digital value 1
    cayenne_lpp_add_digital_input(&lpp, 4, digitalValue2);     // Channel 4 for digital value 2

    // lpp.buffer contains encoded data, and lpp.cursor indicates the size of the payload.
    sendLoRaWANPacket(lpp.buffer, lpp.cursor);
    printCayenneLPPBuffer(&lpp); //used only for debugging


}

void printCayenneLPPBuffer(const cayenne_lpp_t *lpp) {
    SMTC_HAL_TRACE_PRINTF("CayenneLPP Buffer Content (Hex): ");
    for(uint8_t i = 0; i < lpp->cursor; ++i) {
        SMTC_HAL_TRACE_PRINTF("%02X ", lpp->buffer[i]);
    }
    SMTC_HAL_TRACE_PRINTF("\n\r");
}

*/
