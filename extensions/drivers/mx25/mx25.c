#include "mx25.h"

#include "smtc_hal_mcu.h"
#include "smtc_hal_gpio.h"
#include "smtc_hal_spi.h"
#include "smtc_hal_dbg_trace.h"

static uint32_t spi = 0;
static hal_gpio_pin_names_t cs = 0;
static uint32_t _max_expected_busy_time = 0;

bool mx25_init( uint32_t spi_id, hal_gpio_pin_names_t chip_select ) {
    spi = spi_id;
    cs = chip_select;
    hal_gpio_init_out( cs, 1 );

    hal_mcu_delay_ms( MX25_tPUW );

    mx25_wakeup( );

    uint32_t id;
    mx25_read_id( &id );

    if( id != MX25_FLASH_ID ) {
        return false;
    }

    mx25_sleep();

    return true;
}

void mx25_sleep( void ) {
    if( !mx25_wait_ready( ) ) {
        return;
    }

    hal_gpio_set_value( cs, 0 );

    hal_spi_in_out( 1, MX25_FLASH_CMD_DP );

    hal_gpio_set_value( cs, 1 );

    _max_expected_busy_time = MX25_tDefault;
}

void mx25_wakeup( void ) {
    hal_gpio_set_value( cs, 0 );
    hal_mcu_delay_ms( 1 );
    hal_gpio_set_value( cs, 1 );
    hal_mcu_delay_ms( 1 );

    _max_expected_busy_time = MX25_tDefault;
}

bool mx25_busy( void ) {
    uint8_t status;
    mx25_read_status( &status );

    if( ( status & MX25_FLASH_WIP_MASK ) == MX25_FLASH_WIP_MASK ) {
        return true;
    } else {
        return false;
    }
}

bool mx25_wait_ready( void ) {
    uint32_t tickstart = HAL_GetTick( );

    while( mx25_busy( ) ) {
        if( ( HAL_GetTick( ) - tickstart ) >= _max_expected_busy_time ) {
            return false;
        }
    }

    return true;
}

void mx25_read_status( uint8_t* status ) {
    hal_gpio_set_value( cs, 0 );

    hal_spi_in_out( 1, MX25_FLASH_CMD_RDSR );
    *status = hal_spi_in_out( 1, 0xff );

    hal_gpio_set_value( cs, 1 );
}

void mx25_read_id( uint32_t* id ) {
    hal_gpio_set_value( cs, 0 );

    hal_spi_in_out( 1, MX25_FLASH_CMD_RDID );
    *id = hal_spi_in_out( 1, 0xff ) << 16;
    *id |= hal_spi_in_out( 1, 0xff ) << 8;
    *id |= hal_spi_in_out( 1, 0xff ) << 0;

    hal_gpio_set_value( cs, 1 );

    _max_expected_busy_time = MX25_tDefault;
}

void mx25_enable_write( void ) {
    if( !mx25_wait_ready( ) ) {
        return;
    }

    hal_gpio_set_value( cs, 0 );

    hal_spi_in_out( 1, MX25_FLASH_CMD_WREN );

    hal_gpio_set_value( cs, 1 );

    _max_expected_busy_time = MX25_tWSR;
}

bool mx25_read( uint32_t address, uint8_t* buffer, uint32_t size ) {
    if( !mx25_wait_ready( ) ) {
        return false;
    }

    if( address + size >= MX25_FLASH_SIZE ) {
        return false;
    }

    hal_gpio_set_value( cs, 0 );

    hal_spi_in_out( 1, MX25_FLASH_CMD_READ );
    hal_spi_in_out( 1, ( address >> 16 ) & 0xff );
    hal_spi_in_out( 1, ( address >> 8 ) & 0xff );
    hal_spi_in_out( 1, ( address >> 0 ) & 0xff );

    for( uint32_t i = 0; i < size; i++ ) {
        buffer[i] = hal_spi_in_out( 1, 0xff );
    }

    hal_gpio_set_value( cs, 1 );

    _max_expected_busy_time = MX25_tDefault;

    return true;
}

bool mx25_write( uint32_t address, uint8_t* buffer, uint32_t size ) {
    if( !mx25_wait_ready( ) ) {
        return false;
    }

    if( address + size >= MX25_FLASH_SIZE ) {
        return false;
    }

    uint32_t first_page = address / MX25_PAGE_SIZE;
    uint32_t last_page = ( address + size ) / MX25_PAGE_SIZE;

    // only one page affected, write directly
    if( first_page == last_page ) {
        return mx25_write_page( address, buffer, size );
    } else {
        // multiple pages affected, split into multiple write commands
        uint32_t counter = 0;

        for( uint32_t page = first_page; page <= last_page; page++ ) {
            bool status;

            if( page == first_page ) {
                // first affected page
                status = mx25_write_page( address, buffer, MX25_PAGE_SIZE - ( address % MX25_PAGE_SIZE ) );
                counter += MX25_PAGE_SIZE - ( address % MX25_PAGE_SIZE );
            } else if( page == last_page ) {
                // last affected page
                status = mx25_write_page( page * MX25_PAGE_SIZE, &buffer[counter], size - counter );
            } else {
                status = mx25_write_page( page * MX25_PAGE_SIZE, &buffer[counter], MX25_PAGE_SIZE );
                counter += MX25_PAGE_SIZE;
            }

            if( !status ) {
                return false;
            }
        }
    }

    return true;
}

bool mx25_write_page( uint32_t address, uint8_t* buffer, uint32_t size ) {
    if( !mx25_wait_ready( ) ) {
        return false;
    }

    if( address + size >= MX25_FLASH_SIZE ) {
        return false;
    }

    if( size > MX25_PAGE_SIZE ) {
        return false;
    }

    if( ( address + size - 1 ) / MX25_PAGE_SIZE != address / MX25_PAGE_SIZE ) {
        return false;
    }

    if( !mx25_page_writeable( address, buffer, size ) ) {
        return false;
    }

    mx25_enable_write();

    hal_gpio_set_value( cs, 0 );

    hal_spi_in_out( 1, MX25_FLASH_CMD_PP );
    hal_spi_in_out( 1, ( address >> 16 ) & 0xff );
    hal_spi_in_out( 1, ( address >> 8 ) & 0xff );
    hal_spi_in_out( 1, ( address >> 0 ) & 0xff );

    for( uint32_t i = 0; i < size; i++ ) {
        hal_spi_in_out( 1, buffer[i] );
    }

    hal_gpio_set_value( cs, 1 );

    _max_expected_busy_time = MX25_tPP;

    return true;
}

bool mx25_page_writeable( uint32_t address, uint8_t* buffer, uint32_t size ) {
    if( address + size >= MX25_FLASH_SIZE ) {
        return false;
    }

    if( size > MX25_PAGE_SIZE ) {
        return false;
    }

    if( ( address + size - 1 ) / MX25_PAGE_SIZE != address / MX25_PAGE_SIZE ) {
        return false;
    }

    uint8_t temp_buffer[MX25_PAGE_SIZE];

    if( !mx25_read( address, temp_buffer, size ) ) {
        return false;
    }

    for( uint32_t i = 0; i < size; i++ ) {
        if( ~( temp_buffer[i] ) & buffer[i] ) {
            return false;
        }
    }

    return true;
}

bool mx25_erase_page( uint32_t page ) {
    if( !mx25_wait_ready( ) ) {
        return false;
    }

    if( page * MX25_SECTOR_SIZE >= MX25_FLASH_SIZE ) {
        return false;
    }

    uint32_t address = page * MX25_SECTOR_SIZE;

    mx25_enable_write();

    hal_gpio_set_value( cs, 0 );

    hal_spi_in_out( 1, MX25_FLASH_CMD_SE );
    hal_spi_in_out( 1, ( address >> 16 ) & 0xff );
    hal_spi_in_out( 1, ( address >> 8 ) & 0xff );
    hal_spi_in_out( 1, ( address >> 0 ) & 0xff );

    hal_gpio_set_value( cs, 1 );

    _max_expected_busy_time = MX25_tSE;

    return true;
}

bool mx25_erase_all( void ) {
    if( !mx25_wait_ready( ) ) {
        return false;
    }

    mx25_enable_write();

    hal_gpio_set_value( cs, 0 );

    hal_spi_in_out( 1, MX25_FLASH_CMD_CE );

    hal_gpio_set_value( cs, 1 );

    _max_expected_busy_time = MX25_tCE;

    return true;
}
