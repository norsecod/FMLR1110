#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdint.h>
#include <stdbool.h> // For bool type
#include "cayenne_lpp.h" // Cayenne LPP for sensor data formatting
#include "smtc_hal_gpio.h" // HAL GPIO for hardware control
#include "smtc_hal_rtc.h"  // HAL RTC for Real-Time Clock (if needed)
#include "smtc_hal_i2c.h"  // HAL I2C for sensor communication
#include "stm32l0xx_hal_adc.h"
#include "smtc_hal_dbg_trace.h"
#include "smtc_hal_mcu.h"
#include "stm32l0xx_hal_gpio.h"

#include "timer.h" 



#ifdef __cplusplus
extern "C" {
#endif



// Function prototypes
void ADC_Init(void);
void sensor_read(void);
void printCayenneLPPBuffer(const cayenne_lpp_t *lpp);
void sendLoRaWANPacket(const uint8_t *payload, uint8_t payloadSize);
void sendData(float temperature, float analogValue, bool digitalValue1, bool digitalValue2);
void wateralarm(bool digitalValue);
void dooralarm(bool digitalValue);
float GETtemperature(const uint32_t id);
float GETvoltage(ADC_HandleTypeDef *hadc);
void GPIO_Init(void);
void PC10Callback(void* context );
void PC11Callback(void* context );
void PA15Callback(void* context );




#ifdef __cplusplus
}
#endif

#endif // FUNCTIONS_H
