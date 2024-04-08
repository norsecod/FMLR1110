#include "functions.h"



// Global variables (ensure they are declared and initialized appropriately elsewhere)
extern float VDR, temp, ADCmeas, Voltage, Door, water, hastydata1, hastydata2, prev_water,prev_door, door;
extern ADC_HandleTypeDef hadc; // Ensure this type is defined elsewhere
extern TimerEvent_t water_timer, door_timer;

    hal_gpio_irq_t PC10_cb = {
        .pin      = PC_10,
        .context  = NULL,     // context pass to the callback - not used in this example
        .callback = PC10Callback,  // callback called when EXTI is triggered
    };
     hal_gpio_irq_t PC11_cb = {
        .pin      = PC_11,
        .context  = NULL,     // context pass to the callback - not used in this example
        .callback = PC11Callback,  // callback called when EXTI is triggered
    };
     hal_gpio_irq_t PA15_cb = {
        .pin      = PA_15,
        .context  = NULL,     // context pass to the callback - not used in this example
        .callback = PA15Callback,  // callback called when EXTI is triggered
    };


#define STACK_ID 0


void sensor_read(void) {
    SMTC_HAL_TRACE_PRINTF("----- sensor_read -----\n\r");
    temp = GETtemperature(1);
    ADCmeas = GETvoltage(&hadc);
    Voltage = ADCmeas * VDR;   

#if defined(LR11XX)
    //gps_snap(); // Uncomment and implement gps_snap if applicable
#endif
}

void printCayenneLPPBuffer(const cayenne_lpp_t *lpp) {
    SMTC_HAL_TRACE_PRINTF("CayenneLPP Buffer Content (Hex): ");
    for (uint8_t i = 0; i < lpp->cursor; ++i) {
        SMTC_HAL_TRACE_PRINTF("%02X ", lpp->buffer[i]);
    }
    SMTC_HAL_TRACE_PRINTF("\n\r");
}

void sendLoRaWANPacket(const uint8_t *payload, uint8_t payloadSize) {
    uint8_t port = 85;
    bool confirmed = false;
    smtc_modem_request_uplink(STACK_ID, port, confirmed, payload, payloadSize);
}



void sendData(float temperature, float analogValue, bool digitalValue1, bool digitalValue2) {
    cayenne_lpp_t lpp;
    cayenne_lpp_reset(&lpp);

    cayenne_lpp_add_temperature(&lpp, 1, temperature);
    cayenne_lpp_add_analog_input(&lpp, 2, analogValue);
    cayenne_lpp_add_digital_input(&lpp, 3, digitalValue1);
    cayenne_lpp_add_digital_input(&lpp, 4, digitalValue2);


    sendLoRaWANPacket(lpp.buffer, lpp.cursor);
    printCayenneLPPBuffer(&lpp); // Used only for debugging
}

void wateralarm(bool digitalValue)
{ 
    cayenne_lpp_t lpp;
    cayenne_lpp_reset(&lpp);
    cayenne_lpp_add_digital_input(&lpp, 4, digitalValue);

    sendLoRaWANPacket(lpp.buffer, lpp.cursor);
    printCayenneLPPBuffer(&lpp); // Used only for debugging
}

void dooralarm(bool digitalValue)
{
    cayenne_lpp_t lpp;
    cayenne_lpp_reset(&lpp);
    cayenne_lpp_add_digital_input(&lpp, 3, digitalValue);

    sendLoRaWANPacket(lpp.buffer, lpp.cursor);
    printCayenneLPPBuffer(&lpp); // Used only for debugging
}

float GETtemperature(const uint32_t id) {
    uint8_t device_address = 0x90;
    uint8_t temperature_raw;
    float temperature;

    if (hal_i2c_read(id, device_address, 0x00, &temperature_raw) != SUCCESS) {
        return -128.0f; // Error value
    }

    temperature = (float)((int8_t)temperature_raw);
    return temperature;
}

float GETvoltage(ADC_HandleTypeDef *hadc) {
    float batt = 0.0;

    hal_gpio_set_value(PA_3, 1);
    MX_ADC_Init();
    hal_mcu_delay_ms(20);

    if (HAL_ADC_Start(hadc) != HAL_OK) {
        return 128.0f; // Error value
    }

    hal_mcu_delay_ms(20);
    if (HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY) == HAL_OK) {
        uint32_t adc_value = HAL_ADC_GetValue(hadc);
        batt = (3.3f * adc_value) / 4095.0f;
    }
    hal_mcu_delay_ms(20);
    HAL_ADC_DeInit(hadc);
    hal_gpio_set_value(PA_3, 0);

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

void GPIO_Init(void) {

    hal_gpio_init_out(PC_7,0);
    hal_gpio_init_out(PC_1,1);
    hal_gpio_init_out(PB_0,0);
    hal_gpio_init_out(PA_3,0);
    hal_gpio_init_out(PD_2,0);

    hal_gpio_init_in( PC_11, BSP_GPIO_PULL_MODE_DOWN, BSP_GPIO_IRQ_MODE_RISING, &PC11_cb );
    hal_gpio_init_in( PC_10, BSP_GPIO_PULL_MODE_DOWN, BSP_GPIO_IRQ_MODE_RISING, &PC10_cb );
    hal_gpio_init_in( PA_15, BSP_GPIO_PULL_MODE_DOWN, BSP_GPIO_IRQ_MODE_RISING, &PA15_cb );
}








