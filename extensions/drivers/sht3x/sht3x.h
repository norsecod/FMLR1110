#ifndef __EXTENSIONS_DRIVER_SHT3X_SHT31_H__
#define __EXTENSIONS_DRIVER_SHT3X_SHT31_H__

#include "stdbool.h"
#include "stdint.h"

#include "smtc_hal_i2c.h"

bool sht3x_init( uint32_t i2c_id, uint8_t i2c_address );

bool sht3x_read_temp( int32_t* temp, int32_t* rh );

#endif // __EXTENSIONS_DRIVER_SHT3X_SHT31_H__
