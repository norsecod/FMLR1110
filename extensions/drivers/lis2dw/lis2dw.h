#ifndef __EXTENSIONS_DRIVER_LIS2DW_LIS2DW_H__
#define __EXTENSIONS_DRIVER_LIS2DW_LIS2DW_H__

#include "stdbool.h"
#include "stdint.h"

#define WHO_AM_I              0x0F
#define WHO_AM_I_ID           0x44

#define OUT_T_L               0x0D
#define OUT_T_H               0x0E

#define CTRL1                 0x20
#define CTRL2                 0x21
#define CTRL3                 0x22
#define CTRL4_INT1_PAD_CTRL   0x23
#define CTRL5_INT2_PAD_CTRL   0x24
#define CTRL6                 0x25

#define OUT_T                 0x26

#define STATUS_REG            0x27

#define OUT_X_L               0x28
#define OUT_X_H               0x29

#define OUT_Y_L               0x2A
#define OUT_Y_H               0x2B

#define OUT_Z_L               0x2C
#define OUT_Z_H               0x2D

typedef enum {
    RANGE_2G,
    RANGE_4G,
    RANGE_8G,
    RANGE_16G
} range_t;

typedef enum {
    M1_12BIT,
    M2_14BIT,
    M3_14BIT,
    M4_14BIT
} lp_mode_t;

bool lis2dw_init( uint32_t i2c_id, uint8_t i2c_address, uint8_t datarate, uint8_t lp_mode );

bool lis2dw_read();

#endif // __EXTENSIONS_DRIVER_LIS2DW_LIS2DW_H__
