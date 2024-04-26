#include "functions.h"



// Global variables (declared in /application/main.c  )
extern float VDR, temp, ADCmeas, Voltage, Door, water, hastydata1, hastydata2, prev_water,prev_door, door;
extern ADC_HandleTypeDef hadc; // 
extern TimerEvent_t water_timer, door_timer;

    hal_gpio_irq_t PC10_cb = {
        .pin      = PC_10,
        .context  = NULL,     // NULL if no context is used
        .callback = PC10Callback,  // callback called when EXTI is triggered
    };
     hal_gpio_irq_t PC11_cb = {
        .pin      = PC_11,
        .context  = NULL,     // NULL if no context is used
        .callback = PC11Callback,  // callback called when EXTI is triggered
    };
     hal_gpio_irq_t PA15_cb = {
        .pin      = PA_15,
        .context  = NULL,     // NULL if no context is used
        .callback = PA15Callback,  // callback called when EXTI is triggered
    };


#define STACK_ID 0  


void sensor_read(void) {                                    //function that calls functions for sensors
    SMTC_HAL_TRACE_PRINTF("----- sensor_read -----\n\r");   //and calculates the real value of battery
    temp = GETtemperature(1);                               //adc measurment
    Voltage = GETvoltage(&hadc);

    gps_snap(); // Uncomment and implement gps_snap if applicable

}

void printCayenneLPPBuffer(const cayenne_lpp_t *lpp) 
{
    SMTC_HAL_TRACE_PRINTF("CayenneLPP Buffer Content (Hex): "); //function that prints the content of
    for (uint8_t i = 0; i < lpp->cursor; ++i) {                 // cayennelpp buffer so we know it is filled
        SMTC_HAL_TRACE_PRINTF("%02X ", lpp->buffer[i]);         //used for debuggig
    }
    SMTC_HAL_TRACE_PRINTF("\n\r");
}

void sendLoRaWANPacket(const uint8_t *payload, uint8_t payloadSize) //frame function for uplink.
{   
    uint8_t port = 85; //predetermined fport for LoRaWAN uplink
    bool confirmed = false; //unconfirmed package
    smtc_modem_request_uplink(STACK_ID, port, confirmed, payload, payloadSize); // sends data over LoRaWAN
}



void sendData(float temperature, float analogValue, bool digitalValue1, bool digitalValue2)
 {
    cayenne_lpp_t lpp;     //creates and name a structure for CayenneLPP named "lpp" contains buffer and cursor
    cayenne_lpp_reset(&lpp);    //resets the structure lpp so it is empty

    cayenne_lpp_add_temperature(&lpp, 1, temperature);  //temperature channel 1
    cayenne_lpp_add_analog_input(&lpp, 2, analogValue); //ADC (analog) value on channel 2
    cayenne_lpp_add_digital_input(&lpp, 3, digitalValue1);  //Door magnet on channel 3 (high or low)
    cayenne_lpp_add_digital_input(&lpp, 4, digitalValue2);  //watersensor on channel 4 (high or low)


    sendLoRaWANPacket(lpp.buffer, lpp.cursor); //payload in buffer, size is in cursor. is a framefunction for uplink
    printCayenneLPPBuffer(&lpp); // Used only for debugging
}

void wateralarm(bool digitalValue)                                  //used for the interrupt routine
{                                                                   //to send state of water sensor
    cayenne_lpp_t lpp;                                              //look at "sendData for more info"
    cayenne_lpp_reset(&lpp);
    cayenne_lpp_add_digital_input(&lpp, 4, digitalValue);

    sendLoRaWANPacket(lpp.buffer, lpp.cursor);
    printCayenneLPPBuffer(&lpp); // Used only for debugging
}

void dooralarm(bool digitalValue)                                   //used for the interrupt routine        
{                                                                   //to send state of door sensor
    cayenne_lpp_t lpp;                                              //look at "sendData for more info"
    cayenne_lpp_reset(&lpp);
    cayenne_lpp_add_digital_input(&lpp, 3, digitalValue);

    sendLoRaWANPacket(lpp.buffer, lpp.cursor);
    printCayenneLPPBuffer(&lpp); // Used only for debugging
}

float GETtemperature(const uint32_t id) {    //function that calls TC74A0, and reads the temperature
    uint8_t device_address = 0x90;           //adress of TC74A0                    
    uint8_t temperature_raw;                 
    float temperature;

    if (hal_i2c_read(id, device_address, 0x00, &temperature_raw) != SUCCESS) {  //reads the temperature 
        return -128.0f; // Error value                                          if error occurs returns -128 as temperature
    }

    temperature = (float)((int8_t)temperature_raw); //typecasts the int8_t data to float data and stores it in temperature
    return temperature; 
}

float GETvoltage(ADC_HandleTypeDef *hadc) {         // function that take a ADC measurment
    float batt = 0.0;                               //stores the battery voltage (adc value)
    float Volt = 0;
    hal_gpio_set_value(PA_3, 1);                    //turns on the mosfet for the voltage divider
    MX_ADC_Init();                                  //adc init

    hal_mcu_delay_ms(10);
    if (HAL_ADC_Start(hadc) != HAL_OK) {            // This code attempts to start the ADC (Analog to Digital Converter)
        return 128.0f; // Error value               // conversion using the HAL_ADC_Start function.
    }                                               //If the ADC start operation fails (returns a value other than HAL_OK),


    if (HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY) == HAL_OK) { // Polls the ADC for completion of conversion.  
                                                                    //Once completed, retrieves the ADC value and calculates the corresponding voltage.
        uint32_t adc_value = HAL_ADC_GetValue(hadc);                // Voltage is calculated using the formula: Voltage = (ADC_value * Vref) / Max_ADC_Value.
        batt = (3.3f * adc_value) / 4095.0f;                        // Here, Vref is assumed to be 3.3 volts and Max_ADC_Value is 4095.
    }

    HAL_ADC_DeInit(hadc);       
    hal_mcu_delay_ms(10);                    //adc deinit
    hal_gpio_set_value(PA_3, 0);                    //turns off mosfet for the voltage divider
     Volt = batt * VDR;   
    return Volt;
}

void MX_ADC_Init(void)
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
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    mcu_panic();
  }


}

void GPIO_Init(void) {                  //init for gpio pns called in main

    hal_gpio_init_out(PC_7,0);          //red led blinky
    hal_gpio_init_out(PC_1,1);          //green led blinky blinky will be removed from final code
    hal_gpio_init_out(PB_0,0);          //active antenna on/off
    //more pins will be added later
   

    hal_gpio_init_in( PC_11, BSP_GPIO_PULL_MODE_DOWN, BSP_GPIO_IRQ_MODE_RISING, &PC11_cb ); //EXTI interrupt doormagnet
    hal_gpio_init_in( PC_10, BSP_GPIO_PULL_MODE_DOWN, BSP_GPIO_IRQ_MODE_RISING, &PC10_cb ); //EXTI interrupt watersensor    
    hal_gpio_init_in( PA_15, BSP_GPIO_PULL_MODE_DOWN, BSP_GPIO_IRQ_MODE_RISING, &PA15_cb ); //EXTI interrupt accelerometer
}









