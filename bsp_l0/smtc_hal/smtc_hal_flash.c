/*!
 * \file      smtc_hal_flash.c
 *
 * \brief     FLASH Hardware Abstraction Layer implementation
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type
#include <stdio.h>    // TODO: check if needed

#include "smtc_hal_flash.h"
#include "stm32l0xx_hal.h"
#include "smtc_hal_dbg_trace.h"

#include <string.h>
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/*!
 * Generic definition
 */
#ifndef SUCCESS
#define SUCCESS 1
#endif

#ifndef FAIL
#define FAIL 0
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/**
 * @brief  Gets the page of a given address
 * @param  Addr: Address of the FLASH Memory
 * @retval The page of a given address
 */
static uint32_t flash_get_page( uint32_t Address );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

uint8_t hal_flash_erase_page( uint32_t addr, uint8_t nb_page ) {
    uint8_t  status                = SUCCESS;
    uint8_t  hal_status            = SUCCESS;
    uint32_t FirstUserPage         = 0;
    uint32_t PageError             = 0;
    uint8_t  flash_operation_retry = 0;

    FLASH_EraseInitTypeDef EraseInitStruct;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock( );

    /* Clear OPTVERR bit set on virgin samples */
    __HAL_FLASH_CLEAR_FLAG( FLASH_SR_OPTVERR );

    /* Get the 1st page to erase */
    FirstUserPage = flash_get_page( addr );

    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FirstUserPage*FLASH_PAGE_SIZE;
    EraseInitStruct.NbPages = nb_page;

    SMTC_HAL_TRACE_INFO( "Erase page %u\r\n", FirstUserPage );

    /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
    do {
        hal_status = HAL_FLASHEx_Erase( &EraseInitStruct, &PageError );
        flash_operation_retry++;
    } while( ( hal_status != HAL_OK ) && ( flash_operation_retry < FLASH_OPERATION_MAX_RETRY ) );

    if( flash_operation_retry >= FLASH_OPERATION_MAX_RETRY ) {
        /*
          Error occurred while  erase.
          User can add here some code to deal with this error.
          PageError will contain the faulty  and then to know the code error on this ,
          user can call function 'HAL_FLASH_GetError()'
        */
        SMTC_HAL_TRACE_ERROR( "FLASH_OPERATION_MAX_RETRY\r\n" );

        /* Infinite loop */
        while( 1 ) {
        }
    } else {
        flash_operation_retry = 0;
    }

    /* Lock the Flash to disable the flash control register access (recommended
    to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock( );

    return status;
}

uint32_t hal_flash_write_buffer( uint32_t addr, const uint8_t* buffer, uint32_t size ) {
    uint8_t  status      = SUCCESS;
    uint8_t  hal_status  = SUCCESS;
    uint32_t BufferIndex = 0, real_size = 0, AddrEnd = 0;
    uint32_t data32                = 0;
    uint8_t  flash_operation_retry = 0;

    /* Complete size for FLASH_TYPEPROGRAM_DOUBLEWORD operation*/
    if( ( size % 4 ) != 0 ) {
        real_size = size + ( 4 - ( size % 4 ) );
    } else {
        real_size = size;
    }

    AddrEnd = addr + real_size;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock( );

    /* Clear OPTVERR bit set on virgin samples */
    __HAL_FLASH_CLEAR_FLAG( FLASH_SR_OPTVERR );

    /* Don't draw outside the lines */
    if( AddrEnd > ( FLASH_USER_END_ADDR + 1 ) ) {
        status = FAIL;
        return status;
    }

    /* Program the user Flash area word by word
    (area defined by FlashUserStartAddr and FLASH_USER_END_ADDR) ***********/

    while( addr < AddrEnd ) {
        data32 = 0;

        for( uint8_t i = 0; i < 4; i++ ) {
            data32 += ( ( ( uint32_t ) buffer[BufferIndex + i] ) << ( i * 4 ) );
        }

        do {
            hal_status = HAL_FLASH_Program( FLASH_TYPEPROGRAMDATA_WORD, addr, data32 );
            flash_operation_retry++;
        } while( ( hal_status != HAL_OK ) && ( flash_operation_retry < FLASH_OPERATION_MAX_RETRY ) );

        if( flash_operation_retry >= FLASH_OPERATION_MAX_RETRY ) {
            /* Error occurred while writing data in Flash memory.
            User can add here some code to deal with this error */
            /* Infinite loop */
            while( 1 ) {
            }
        } else {
            flash_operation_retry = 0;
            /* increment to next double word*/
            addr        = addr + 4;
            BufferIndex = BufferIndex + 4;
        }
    }

    /* Lock the Flash to disable the flash control register access (recommended
    to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock( );

    return real_size;
}

void hal_flash_read_buffer( uint32_t addr, uint8_t* buffer, uint32_t size ) {
    uint32_t     FlashIndex = 0;
    __IO uint8_t data8      = 0;

    while( FlashIndex < size ) {
        data8 = *( __IO uint8_t* )( addr + FlashIndex );

        buffer[FlashIndex] = data8;

        FlashIndex++;
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static uint32_t flash_get_page( uint32_t Addr ) {
    return ( Addr - FLASH_BASE ) / FLASH_PAGE_SIZE;
}

/* --- EOF ------------------------------------------------------------------ */
