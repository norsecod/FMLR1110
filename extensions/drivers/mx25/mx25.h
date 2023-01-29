#ifndef __EXTENSIONS_DRIVER_MX25_MX25_H__
#define __EXTENSIONS_DRIVER_MX25_MX25_H__

#include "stdbool.h"
#include "stdint.h"

#include "smtc_hal_i2c.h"

// flash infos
#define    MX25_PAGE_SIZE             256        // 256B
#define    MX25_SECTOR_SIZE           4096       // 4KB
#define    MX25_FLASH_SIZE            524288     // 512KB
#define    MX25_FLASH_ID              0x00c22813

// timing definitions
#define    MX25_tW                    30         // 30ms
#define    MX25_tDP                   1          // 10us
#define    MX25_tBP                   1          // 100us
#define    MX25_tPP                   10         // 10ms
#define    MX25_tSE                   240        // 240ms
#define    MX25_tBE                   3500       // 3.5s
#define    MX25_tCE                   15000      // 15s
#define    MX25_tBE32                 1750       // 1.75s
#define    MX25_tPUW                  10         // 10ms
#define    MX25_tWSR                  1          // 1ms
#define    MX25_tDefault              1          // default timeout is 1ms

//ID commands
#define    MX25_FLASH_CMD_RDID        0x9F       //RDID (Read Identification)
#define    MX25_FLASH_CMD_RES         0xAB       //RES (Read Electronic ID)
#define    MX25_FLASH_CMD_REMS        0x90       //REMS (Read Electronic & Device ID)

//Register commands
#define    MX25_FLASH_CMD_WRSR        0x01       //WRSR (Write Status Register)
#define    MX25_FLASH_CMD_RDSR        0x05       //RDSR (Read Status Register)
#define    MX25_FLASH_CMD_WRSCUR      0x2F       //WRSCUR (Write Security Register)
#define    MX25_FLASH_CMD_RDSCUR      0x2B       //RDSCUR (Read Security Register)
#define    MX25_FLASH_CMD_RDCR        0x15       //RDCR (Read Configuration Register)

//READ commands
#define    MX25_FLASH_CMD_READ        0x03       //READ (1 x I/O)
#define    MX25_FLASH_CMD_2READ       0xBB       //2READ (2 x I/O)
#define    MX25_FLASH_CMD_4READ       0xEB       //4READ (4 x I/O)
#define    MX25_FLASH_CMD_FASTREAD    0x0B       //FAST READ (Fast read data)
#define    MX25_FLASH_CMD_DREAD       0x3B       //DREAD (1In/2 Out fast read)
#define    MX25_FLASH_CMD_QREAD       0x6B       //QREAD (1In/4 Out fast read)
#define    MX25_FLASH_CMD_RDSFDP      0x5A       //RDSFDP (Read SFDP)

//Program commands
#define    MX25_FLASH_CMD_WREN        0x06       //WREN (Write Enable)
#define    MX25_FLASH_CMD_WRDI        0x04       //WRDI (Write Disable)
#define    MX25_FLASH_CMD_PP          0x02       //PP (page program)
#define    MX25_FLASH_CMD_4PP         0x38       //4PP (Quad page program)

//Erase commands
#define    MX25_FLASH_CMD_SE          0x20       //SE (Sector Erase)
#define    MX25_FLASH_CMD_BE32K       0x52       //BE32K (Block Erase 32kb)
#define    MX25_FLASH_CMD_BE          0xD8       //BE (Block Erase)
#define    MX25_FLASH_CMD_CE          0x60       //CE (Chip Erase) hex code: 60 or C7

//Mode setting commands
#define    MX25_FLASH_CMD_DP          0xB9       //DP (Deep Power Down)
#define    MX25_FLASH_CMD_ENSO        0xB1       //ENSO (Enter Secured OTP)
#define    MX25_FLASH_CMD_EXSO        0xC1       //EXSO  (Exit Secured OTP)
#define    MX25_FLASH_CMD_SBL         0xC0       //SBL (Set Burst Length) Old: 0xC0

//Reset commands
#define    MX25_FLASH_CMD_RSTEN       0x66       //RSTEN (Reset Enable)
#define    MX25_FLASH_CMD_RST         0x99       //RST (Reset Memory)

// Flash control register mask define
#define    MX25_FLASH_WIP_MASK        0x01

bool mx25_init( uint32_t spi_id, hal_gpio_pin_names_t chip_select );

void mx25_read_id( uint32_t* id );

void mx25_sleep( void );

void mx25_wakeup( void );

bool mx25_busy( void );

bool mx25_wait_ready( void );

void mx25_read_status( uint8_t* status );

void mx25_enable_write( void );

bool mx25_read( uint32_t address, uint8_t* buffer, uint32_t size );

bool mx25_write( uint32_t address, uint8_t* buffer, uint32_t size );

bool mx25_write_page( uint32_t address, uint8_t* buffer, uint32_t size );

bool mx25_page_writeable( uint32_t address, uint8_t* buffer, uint32_t size );

bool mx25_erase_page( uint32_t page );

bool mx25_erase_all( void );

#endif // __EXTENSIONS_DRIVER_MX25_MX25_H__
