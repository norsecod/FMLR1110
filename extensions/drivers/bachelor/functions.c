/*written by Terje Gården
for a bachelors project "LoRaWAN Båtsensor" at Hiof (Høgskolen i Østfold)*/

#include "functions.h"
#include "smtc_hal_i2c.h"

#define adxl_addr 0x53<<1
// Global variables (declared in /application/main.c  )
extern float VDR, temp, Voltage, Door, water, hastydata1, hastydata2, prev_water,prev_door, door;
extern ADC_HandleTypeDef hadc;
extern TimerEvent_t water_timer, door_timer;
extern uint16_t axisxyz[3]; 


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
    temp = GETtemperature(1);   
    hal_mcu_delay_ms(10);                            //adc measurment
    Voltage = GETvoltage(&hadc);
    hal_mcu_delay_ms(10);
   // gps_snap(); // Uncomment and implement gps_snap if applicable


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
    cayenne_lpp_add_analog_input(&lpp, 2, analogValue); //voltage value on channel 2
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

    if (hal_i2c_read(id, device_address, 0x00, &temperature_raw) != SUCCESS) //reads the temperature 
    {  
        return -128.0f; // Error value                                          if error occurs returns -128 as temperature
    }

    temperature = (float)((int8_t)temperature_raw); //typecasts the int8_t data to float data and stores it in temperature
    return temperature; 
}

float GETvoltage(ADC_HandleTypeDef *hadc) {         // function that take a ADC measurment
    float batt = 0.0;                               //stores the battery voltage (adc value)
    hal_gpio_set_value(PA_3, 1);                    //turns on the mosfet for the voltage divider
    MX_ADC_Init();                                  //adc init

    HAL_Delay(30);
    if (HAL_ADC_Start(hadc) != HAL_OK) {            // This code attempts to start the ADC (Analog to Digital Converter)
        return 128.0f; // Error value               // conversion using the HAL_ADC_Start function.
    }                                               //If the ADC start operation fails (returns a value other than HAL_OK),


    if (HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY) == HAL_OK) { // Polls the ADC for completion of conversion.  
                                                                    //Once completed, retrieves the ADC value and calculates the corresponding voltage.
        uint32_t adc_value = HAL_ADC_GetValue(hadc);                // Voltage is calculated using the formula: Voltage = (ADC_value * Vref) / Max_ADC_Value.
        batt = ((3.3f * adc_value) / 4095.0f)*VDR;                        // Here, Vref is assumed to be 3.3 volts and Max_ADC_Value is 4095.
    }

    HAL_ADC_DeInit(hadc);       
    HAL_Delay(30);                    //adc deinit
    hal_gpio_set_value(PA_3, 0);                    //turns off mosfet for the voltage divider
        
    return batt;
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

    hal_gpio_init_out(PB_0,0);          //active antenna on/off
    hal_gpio_init_out(PA_3,0);          //voltage divider on/off
  

    hal_gpio_init_in( PC_11, BSP_GPIO_PULL_MODE_DOWN, BSP_GPIO_IRQ_MODE_RISING, &PC11_cb ); //EXTI interrupt doormagnet
    hal_gpio_init_in( PC_10, BSP_GPIO_PULL_MODE_DOWN, BSP_GPIO_IRQ_MODE_RISING, &PC10_cb ); //EXTI interrupt watersensor    
    hal_gpio_init_in( PA_15, BSP_GPIO_PULL_MODE_DOWN, BSP_GPIO_IRQ_MODE_RISING, &PA15_cb ); //EXTI interrupt accelerometer
}

/*=============================functions for ADCL343========================================*/


// Function to write data to ADXL343
void adxl_write(uint8_t reg, uint8_t value) {
    
    hal_i2c_write(1, adxl_addr , reg, value);
}

// Function to read data from ADXL343
void adxl_read(uint8_t reg, uint8_t *data_rec, uint8_t numberofbytes) {
    hal_i2c_read_buffer(1, adxl_addr , reg, data_rec, numberofbytes);
}

// Function to initialize ADXL343
void adxl_init() {
    uint8_t data_rec[1];
    adxl_read(0x00, data_rec, 1);
    hal_mcu_delay_ms(10);
    SMTC_HAL_TRACE_PRINTF("Data read from register 0x%02X: 0x%02X\n",0x00, data_rec[0]);
    if (data_rec[0]== 0xE5)
    {
        SMTC_HAL_TRACE_PRINTF("ADXL found\n\r");
 
        adxl_write(0x2d, 0x00);  // reset all bits
        adxl_write(0x2d, 0x08);  // measure mode

        adxl_write(0x31, 0x01);  // +- 4g range

    }
    else
    {
        SMTC_HAL_TRACE_PRINTF("ADXL not found\n\r");
    }
    
}

// Function to read acceleration data from ADXL343
void adxl_read_acceleration(uint16_t *axis) {
    uint8_t data_rec[6];

    // Read 6 bytes of data from 0x32 to 0x37
    adxl_read(0x32, data_rec, 6);

    // Convert the raw data to 16-bit integers
    axis[0] = (uint16_t)((data_rec[1] << 8) | data_rec[0]);
    axis[1] = (uint16_t)((data_rec[3] << 8) | data_rec[2]);
    axis[2] = (uint16_t)((data_rec[5] << 8) | data_rec[4]);
   

}