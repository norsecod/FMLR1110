#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdint.h>
#include <stdbool.h> // For bool type
#include "cayenne_lpp.h" // Cayenne LPP for sensor data formatting
#include "smtc_hal_gpio.h" // HAL GPIO for hardware control
#include "smtc_hal_rtc.h"  // HAL RTC for Real-Time Clock (if needed)
#include "smtc_hal_i2c.h"  // HAL I2C for sensor communication
#include "stm32l0xx_hal_i2c.h"
#include "ralf_lr11xx.h"
#include "lr11xx_gnss.h"
#include "ral_lr11xx.h"
#include "smtc_hal_dbg_trace.h"
#include "smtc_hal_mcu.h"


#include "timer.h" 



#ifdef __cplusplus
extern "C" {
#endif


// Function prototypes
void MX_ADC_Init(void);
void sensor_read(void);
void printCayenneLPPBuffer(const cayenne_lpp_t *lpp);
void sendLoRaWANPacket(const uint8_t *payload, uint8_t payloadSize);
void sendData(float temperature, float analogValue, bool digitalValue1, bool digitalValue2, bool digitalValue3);
void wateralarm(bool digitalValue);
void dooralarm(bool digitalValue);
void tapalarm(bool digitalValue);
float GETtemperature(const uint32_t id);
float GETvoltage(ADC_HandleTypeDef *hadc);
void GPIO_Init(void);
void PC10Callback(void* context );
void PC11Callback(void* context );
void PA15Callback(void* context );
uint32_t get_current_gnss_time();



/*========prototypes for ADXL343===============*/

// Function prototypes for ADXL343
void adxl_init(void);
void adxl_write(uint8_t reg, uint8_t value);
void adxl_read(uint8_t reg, uint8_t *data_rec, uint8_t numberofbytes);
void adxl_read_acceleration(uint16_t *axis);
void adxl_tap_config(void);

#ifdef __cplusplus
}
#endif

#endif // FUNCTIONS_H
