#include "sht3x.h"

#include "smtc_hal_i2c.h"

static const uint8_t CRC_POLYNOMIAL    = 0x31;
static const uint8_t CRC_INIT          = 0xff;
static uint32_t i2c = 0;
static uint8_t address = 0;

static uint8_t sht3x_generate_crc( uint8_t* data, uint16_t count );
static inline bool sht3x_check_crc( uint8_t* data, uint16_t count );

bool sht3x_init( uint32_t i2c_id, uint8_t i2c_address ) {
    i2c = i2c_id;
    address = i2c_address;
    hal_i2c_set_addr_size( I2C_ADDR_SIZE_16 );
    uint8_t buffer[3] = {0};
    hal_i2c_read_buffer( i2c, address, 0xF32D, buffer, 3 );
    return sht3x_check_crc( buffer, 2 );
}

bool sht3x_read_temp( int32_t* temp, int32_t* rh ) {
    hal_i2c_set_addr_size( I2C_ADDR_SIZE_16 );
    uint8_t buffer[6] = {0};
    hal_i2c_read_buffer( i2c, address, 0x2C06, buffer, 6 );

    if( !sht3x_check_crc( buffer, 2 ) || !sht3x_check_crc( &buffer[3], 2 ) ) {
        return false;
    }

    int32_t temp_ticks = ( buffer[1] & 0xff ) | ( ( int32_t )buffer[0] << 8 );
    int32_t rh_ticks = ( buffer[4] & 0xff ) | ( ( int32_t )buffer[3] << 8 );

    *temp = ( ( 21875 * temp_ticks ) >> 13 ) - 45000;
    *rh = ( ( 12500 * rh_ticks ) >> 13 );

    return true;
}

static uint8_t sht3x_generate_crc( uint8_t* data, uint16_t count ) {
    uint8_t crc = CRC_INIT;
    uint8_t current_byte;
    uint8_t crc_bit;

    /* calculates 8-Bit checksum with given polynomial */
    for( current_byte = 0; current_byte < count; ++current_byte ) {
        crc ^= ( data[current_byte] );

        for( crc_bit = 8; crc_bit > 0; --crc_bit ) {
            if( crc & 0x80 ) {
                crc = ( crc << 1 ) ^ CRC_POLYNOMIAL;
            } else {
                crc = ( crc << 1 );
            }
        }
    }

    return crc;
}

static inline bool sht3x_check_crc( uint8_t* data, uint16_t count ) {
    return sht3x_generate_crc( data, count ) == data[count];
}
