#include "lis2dw.h"

#include "smtc_hal_i2c.h"
#include "smtc_hal_mcu.h"

static uint32_t i2c = 0;
static uint8_t address, lp_mod, dr;

static bool lis2dw_write_byte( uint8_t register_addr, uint8_t value );
static bool lis2dw_read_byte( uint8_t register_addr, uint8_t* value );
static bool lis2dw_read_multiple( uint8_t register_addr, uint8_t* value, uint8_t size );

bool lis2dw_init( uint32_t i2c_id, uint8_t i2c_address, uint8_t datarate, uint8_t lp_mode ) {
    bool i2c_status = true;
    i2c = i2c_id;
    address = i2c_address;
    lp_mod = lp_mode;
    dr = datarate;

    // verify sensor
    uint8_t who_am_i;
    i2c_status = lis2dw_read_byte( WHO_AM_I, &who_am_i );

    if( !i2c_status || who_am_i != WHO_AM_I_ID ) {
        return false;
    }

    // soft reset to defaults
    i2c_status = lis2dw_write_byte( CTRL2, 0x40 );

    if( !i2c_status ) {
        return false;
    }

    uint8_t answer;

    do {
        i2c_status = lis2dw_read_byte( CTRL2, &answer );
        hal_mcu_wait_us( 5000 );
    } while( answer & 0x40 ); //check if the reset is finished

    // with ODR = 12Hz, low power mode 0
    i2c_status = lis2dw_write_byte( CTRL1 | 0x80, 0x02 << 4 | lp_mod );

    if( !i2c_status ) {
        return false;
    }

    // single conversion mode
    i2c_status = lis2dw_write_byte( CTRL3, 0x01 );

    if( !i2c_status ) {
        return false;
    }

    i2c_status = lis2dw_write_byte( CTRL6, datarate << 4 );

    if( !i2c_status ) {
        return false;
    }

    // enable multiple byte read
    i2c_status = lis2dw_write_byte( CTRL2, 0x04 );

    if( !i2c_status ) {
        return false;
    }

    return true;
}

bool lis2dw_read( int16_t* axis ) {
    uint8_t buffer[6];
    bool i2c_status = true;

    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;

    // trigger conversion
    i2c_status = lis2dw_write_byte( CTRL3, 0x02 );

    if( !i2c_status ) {
        return false;
    }

    hal_mcu_wait_us( 5000 ); // wait for conversion

    i2c_status = lis2dw_read_multiple( OUT_X_L, buffer, 6 );

    if( !i2c_status ) {
        return false;
    }

    x = buffer[0] | buffer[1] << 8;
    y = buffer[2] | buffer[3] << 8;
    z = buffer[4] | buffer[5] << 8;

    if( lp_mod == M1_12BIT ) {
        x >>= 4;
        y >>= 4;
        z >>= 4;

        switch( dr ) {
        case RANGE_2G:
            x = ( int16_t )( ( float )x * 0.976f );
            y = ( int16_t )( ( float )y * 0.976f );
            z = ( int16_t )( ( float )z * 0.976f );
            break;

        case RANGE_4G:
            x = ( int16_t )( ( float )x * 1.952f );
            y = ( int16_t )( ( float )y * 1.952f );
            z = ( int16_t )( ( float )z * 1.952f );
            break;

        case RANGE_8G:
            x = ( int16_t )( ( float )x * 3.904f );
            y = ( int16_t )( ( float )y * 3.904f );
            z = ( int16_t )( ( float )z * 3.904f );
            break;

        case RANGE_16G:
            x = ( int16_t )( ( float )x * 7.808f );
            y = ( int16_t )( ( float )y * 7.808f );
            z = ( int16_t )( ( float )z * 7.808f );
            break;
        }
    } else {
        x >>= 2;
        y >>= 2;
        z >>= 2;

        switch( dr ) {
        case RANGE_2G:
            x = ( int16_t )( ( float )x * 0.244f );
            y = ( int16_t )( ( float )y * 0.244f );
            z = ( int16_t )( ( float )z * 0.244f );
            break;

        case RANGE_4G:
            x = ( int16_t )( ( float )x * 0.488f );
            y = ( int16_t )( ( float )y * 0.488f );
            z = ( int16_t )( ( float )z * 0.488f );
            break;

        case RANGE_8G:
            x = ( int16_t )( ( float )x * 0.976f );
            y = ( int16_t )( ( float )y * 0.976f );
            z = ( int16_t )( ( float )z * 0.976f );
            break;

        case RANGE_16G:
            x = ( int16_t )( ( float )x * 1.952f );
            y = ( int16_t )( ( float )y * 1.952f );
            z = ( int16_t )( ( float )z * 1.952f );
            break;
        }
    }

    axis[0] = x;
    axis[1] = y;
    axis[2] = z;

    return true;
}

static bool lis2dw_write_byte( uint8_t register_addr, uint8_t value ) {
    hal_i2c_set_addr_size( I2C_ADDR_SIZE_8 );
    uint8_t status = hal_i2c_write_buffer( i2c, address, register_addr, &value, 1 );

    if( status ) {
        return false;
    }

    return true;
}

static bool lis2dw_read_byte( uint8_t register_addr, uint8_t* value ) {
    hal_i2c_set_addr_size( I2C_ADDR_SIZE_8 );
    uint8_t status = hal_i2c_read_buffer( i2c, address, register_addr, value, 1 );

    if( status ) {
        return false;
    }

    return true;
}

static bool lis2dw_read_multiple( uint8_t register_addr, uint8_t* value, uint8_t size ) {
    hal_i2c_set_addr_size( I2C_ADDR_SIZE_8 );
    uint8_t status = hal_i2c_read_buffer( i2c, address, register_addr, value, size );

    if( status ) {
        return false;
    }

    return true;
}
