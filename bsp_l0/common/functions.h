#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdint.h>
#include <stdbool.h> // For bool type
#include "cayenne_lpp.h" // Cayenne LPP for sensor data formatting
#include "smtc_hal_gpio.h" // HAL GPIO for hardware control
#include "smtc_hal_rtc.h"  // HAL RTC for Real-Time Clock (if needed)
//#include "smtc_hal_i2c.h"  // HAL I2C for sensor communication
#include "stm32l0xx_hal_i2c.h"

#include "smtc_hal_dbg_trace.h"
#include "smtc_hal_mcu.h"


#include "timer.h" 



#ifdef __cplusplus
extern "C" {
#endif

/*====defines for ADXL323=========*/
/*=========================================================================
		I2C ADDRESS/BITS
		-----------------------------------------------------------------------*/
#define ADXL343_ADDRESS              (0x53) /**< ALT address pin low */
/*=========================================================================*/

/*=========================================================================
		REGISTERS
		-----------------------------------------------------------------------*/
#define ADXL3XX_REG_DEVID            (0x00)        /**< Device ID */
#define ADXL3XX_REG_THRESH_TAP       (0x1D)        /**< Tap threshold */
#define ADXL3XX_REG_OFSX             (0x1E)        /**< X-axis offset */
#define ADXL3XX_REG_OFSY             (0x1F)        /**< Y-axis offset */
#define ADXL3XX_REG_OFSZ             (0x20)        /**< Z-axis offset */
#define ADXL3XX_REG_DUR              (0x21)        /**< Tap duration */
#define ADXL3XX_REG_LATENT           (0x22)        /**< Tap latency */
#define ADXL3XX_REG_WINDOW           (0x23)        /**< Tap window */
#define ADXL3XX_REG_THRESH_ACT       (0x24)        /**< Activity threshold */
#define ADXL3XX_REG_THRESH_INACT     (0x25)        /**< Inactivity threshold */
#define ADXL3XX_REG_TIME_INACT       (0x26)        /**< Inactivity time */
#define ADXL3XX_REG_ACT_INACT_CTL    (0x27)        /**< Axis enable control for activity and inactivity detection */
#define ADXL3XX_REG_THRESH_FF        (0x28)        /**< Free-fall threshold */
#define ADXL3XX_REG_TIME_FF          (0x29)        /**< Free-fall time */
#define ADXL3XX_REG_TAP_AXES         (0x2A)        /**< Axis control for single/double tap */
#define ADXL3XX_REG_ACT_TAP_STATUS   (0x2B)        /**< Source for single/double tap */
#define ADXL3XX_REG_BW_RATE          (0x2C)        /**< Data rate and power mode control */
#define ADXL3XX_REG_POWER_CTL        (0x2D)        /**< Power-saving features control */
#define ADXL3XX_REG_INT_ENABLE       (0x2E)        /**< Interrupt enable control */
#define ADXL3XX_REG_INT_MAP          (0x2F)        /**< Interrupt mapping control */
#define ADXL3XX_REG_INT_SOURCE       (0x30)        /**< Source of interrupts */
#define ADXL3XX_REG_DATA_FORMAT      (0x31)        /**< Data format control */
#define ADXL3XX_REG_DATAX0           (0x32)        /**< X-axis data 0 */
#define ADXL3XX_REG_DATAX1           (0x33)        /**< X-axis data 1 */
#define ADXL3XX_REG_DATAY0           (0x34)        /**< Y-axis data 0 */
#define ADXL3XX_REG_DATAY1           (0x35)        /**< Y-axis data 1 */
#define ADXL3XX_REG_DATAZ0           (0x36)        /**< Z-axis data 0 */
#define ADXL3XX_REG_DATAZ1           (0x37)        /**< Z-axis data 1 */
#define ADXL3XX_REG_FIFO_CTL         (0x38)        /**< FIFO control */
#define ADXL3XX_REG_FIFO_STATUS      (0x39)        /**< FIFO status */

#define ADXL3XX_SET_MEASURE_BYTE     (0x08) /*Put adxl345 into measurement mode*/
/*=========================================================================*/

/*=========================================================================
		REGISTERS
		-----------------------------------------------------------------------*/
#define ADXL343_MG2G_MULTIPLIER      (0.004)       /**< 4mg per lsb */
/*=========================================================================*/

#define ADXL343_GRAVITY (9.80665F) /**< Earth's gravity in m/s^2 */

/*======defines and enums for ADXL343=========*/

typedef enum
{
	accl_2g=0,
	accl_4g=1,
	accl_8g=2,
	accl_16g=3
}adxl345Parameters;

typedef struct
{

	float xx;
	float yy;
	float zz;

}accleration_values_t;



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

/*========prototypes for ADXL343===============*/

// Function prototypes for ADXL343

void adxl345_init(adxl345Parameters param);
void adxl345_update();
void adxl345_get_values(accleration_values_t * values);


#ifdef __cplusplus
}
#endif

#endif // FUNCTIONS_H
