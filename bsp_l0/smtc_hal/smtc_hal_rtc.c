/*!
 * \file      smtc_hal_rtc.c
 *
 * \brief     RTC Hardware Abstraction Layer implementation
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

#include <time.h>
#include "smtc_hal_rtc.h"

#include "stm32l0xx_hal.h"
#include "stm32l0xx_ll_rtc.h"
#include "smtc_hal_mcu.h"

#include "timer.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*!
 * Calculates ceiling( X / N )
 */
#define DIVC( X, N ) ( ( ( X ) + ( N ) -1 ) / ( N ) )

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

// clang-format off

// MCU Wake Up Time
#define MIN_ALARM_DELAY_IN_TICKS       3U              // in ticks

// sub-second number of bits
#define N_PREDIV_S                     10U

// Synchronous prediv
#define PREDIV_S                       ( ( 1U << N_PREDIV_S ) - 1U )

// Asynchronous prediv
#define PREDIV_A                       ( ( 1U << ( 15U - N_PREDIV_S ) ) - 1U )

// Sub-second mask definition
#define ALARM_SUBSECOND_MASK           ( N_PREDIV_S << RTC_ALRMASSR_MASKSS_Pos )

// RTC Time base in us
#define USEC_NUMBER                    1000000U
#define MSEC_NUMBER                    ( USEC_NUMBER / 1000 )

#define COMMON_FACTOR                  3U
#define CONV_NUMER                     ( MSEC_NUMBER >> COMMON_FACTOR )
#define CONV_DENOM                     ( 1U << ( N_PREDIV_S - COMMON_FACTOR ) )

/*!
 * Days, Hours, Minutes and seconds
 */
#define DAYS_IN_LEAP_YEAR              ( ( uint32_t ) 366U )
#define DAYS_IN_YEAR                   ( ( uint32_t ) 365U )
#define SECONDS_IN_1DAY                ( ( uint32_t ) 86400U )
#define SECONDS_IN_1HOUR               ( ( uint32_t ) 3600U )
#define SECONDS_IN_1MINUTE             ( ( uint32_t ) 60U )
#define MINUTES_IN_1HOUR               ( ( uint32_t ) 60U )
#define HOURS_IN_1DAY                  ( ( uint32_t ) 24U )

/*!
 * Correction factors
 */
#define DAYS_IN_MONTH_CORRECTION_NORM  ( ( uint32_t ) 0x99AAA0 )
#define DAYS_IN_MONTH_CORRECTION_LEAP  ( ( uint32_t ) 0x445550 )

// clang-format on

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*!
 * RTC timer context
 */
typedef struct {
    uint32_t        time_ref_in_ticks;  // Reference time
    RTC_TimeTypeDef calendar_time;      // Reference time in calendar format
    RTC_DateTypeDef calendar_date;      // Reference date in calendar format
} rtc_context_t;

typedef struct bsp_rtc_s {
    RTC_HandleTypeDef handle;
    /*!
     * Keep the value of the RTC timer when the RTC alarm is set
     * Set with the \ref bsp_rtc_set_context function
     * Value is kept as a Reference to calculate alarm
     */
    rtc_context_t context;
} bsp_rtc_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static bsp_rtc_t bsp_rtc;

static volatile bool wut_timer_irq_happened = false;

static uint32_t wakeup_timer_rounds = 0;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*!
 * Get the elapsed time in seconds and milliseconds since RTC initialization
 *
 * \param [OUT] milliseconds Number of milliseconds elapsed since RTC
 *                           initialization
 * \retval seconds           Number of seconds elapsed since RTC initialization
 */
static uint32_t rtc_get_calendar_time( uint16_t* milliseconds );

/*!
 * Get current full resolution RTC timestamp in ticks
 *
 * \retval timestamp_in_ticks Current timestamp in ticks
 */
static uint64_t rtc_get_timestamp_in_ticks( RTC_DateTypeDef* date, RTC_TimeTypeDef* time );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void hal_rtc_init( void ) {
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    bsp_rtc.handle.Instance            = RTC;
    bsp_rtc.handle.Init.HourFormat     = RTC_HOURFORMAT_24;
    bsp_rtc.handle.Init.AsynchPrediv   = PREDIV_A;
    bsp_rtc.handle.Init.SynchPrediv    = PREDIV_S;
    bsp_rtc.handle.Init.OutPut         = RTC_OUTPUT_DISABLE;
    bsp_rtc.handle.Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
    bsp_rtc.handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    bsp_rtc.handle.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

    if( HAL_RTC_Init( &bsp_rtc.handle ) != HAL_OK ) {
        mcu_panic( );
    }

    // Initialize RTC counter to 0
    date.Year    = 0;
    date.Month   = RTC_MONTH_JANUARY;
    date.Date    = 1;
    date.WeekDay = RTC_WEEKDAY_MONDAY;
    HAL_RTC_SetDate( &bsp_rtc.handle, &date, RTC_FORMAT_BIN );

    /*at 0:0:0*/
    time.Hours          = 0;
    time.Minutes        = 0;
    time.Seconds        = 0;
    time.SubSeconds     = 0;
    time.TimeFormat     = 0;
    time.StoreOperation = RTC_DAYLIGHTSAVING_NONE;
    time.DayLightSaving = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime( &bsp_rtc.handle, &time, RTC_FORMAT_BIN );

    // Enable Direct Read of the calendar registers (not through Shadow
    // registers)
    HAL_RTCEx_EnableBypassShadow( &bsp_rtc.handle );

    hal_rtc_set_timer_context( );
}

uint32_t hal_rtc_get_timer_context( void ) {
    return bsp_rtc.context.time_ref_in_ticks;
}

uint32_t hal_rtc_set_timer_context( void ) {
    bsp_rtc.context.time_ref_in_ticks = ( uint32_t ) rtc_get_timestamp_in_ticks( &bsp_rtc.context.calendar_date, &bsp_rtc.context.calendar_time );
    return bsp_rtc.context.time_ref_in_ticks;
}

uint32_t hal_rtc_get_timer_elapsed_time( void ) {
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    uint32_t calendarValue = ( uint32_t )rtc_get_timestamp_in_ticks( &date, &time );

    return( ( uint32_t )( calendarValue - bsp_rtc.context.time_ref_in_ticks ) );
}

uint32_t hal_rtc_get_minimum_timeout( void ) {
    return( MIN_ALARM_DELAY_IN_TICKS );
}

uint32_t hal_rtc_get_time_s( void ) {
    uint16_t milliseconds = 0;
    return rtc_get_calendar_time( &milliseconds );
}

uint32_t hal_rtc_get_time_100us( void ) {
    uint32_t seconds      = 0;
    uint16_t  milliseconds_div_10 = 0;

    seconds = rtc_get_calendar_time( &milliseconds_div_10 );

    return seconds * 10000 + milliseconds_div_10;
}

uint32_t hal_rtc_get_time_ms( void ) {
    return ( hal_rtc_get_time_100us( ) / 10 );
}

uint32_t hal_rtc_get_time_ticks( void ) {
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    uint32_t calendarValue = ( uint32_t )rtc_get_timestamp_in_ticks( &date, &time );

    return( calendarValue );
}

void hal_rtc_wakeup_timer_set_ms( const int32_t milliseconds ) {
    uint32_t delay_ms_2_tick = rtc_ms_2_wakeup_timer_tick( milliseconds );

    if( delay_ms_2_tick > 65535 ) {
        wakeup_timer_rounds = delay_ms_2_tick / 65535;
        delay_ms_2_tick = delay_ms_2_tick % 65535;
    } else {
        wakeup_timer_rounds = 0;
    }

    HAL_RTCEx_DeactivateWakeUpTimer( &bsp_rtc.handle );
    // reset irq status
    wut_timer_irq_happened = false;
    HAL_RTCEx_SetWakeUpTimer_IT( &bsp_rtc.handle, delay_ms_2_tick, RTC_WAKEUPCLOCK_RTCCLK_DIV16 );
}

void hal_rtc_wakeup_timer_stop( void ) {
    HAL_RTCEx_DeactivateWakeUpTimer( &bsp_rtc.handle );
    wakeup_timer_rounds = 0;
}

bool hal_rtc_has_wut_irq_happened( void ) {
    return wut_timer_irq_happened;
}

uint32_t rtc_tick_2_ms( const uint32_t tick ) {
    uint32_t seconds    = tick >> N_PREDIV_S;
    uint32_t local_tick = tick & PREDIV_S;

    return ( uint32_t )( ( seconds * 1000 ) + ( ( local_tick * 10000 ) >> N_PREDIV_S ) );
}

uint32_t rtc_ms_2_ticks( const uint32_t milliseconds ) {
    return ( uint32_t )( ( ( ( uint64_t ) milliseconds ) * CONV_DENOM ) / CONV_NUMER );
}

uint32_t rtc_ms_2_wakeup_timer_tick( const uint32_t milliseconds ) {
    uint32_t nb_tick = 0;
    // Compute is done for LSE @ 32.768kHz
    // Assuming that RTC_WAKEUPCLOCK_RTCCLK_DIV16 is used => tick is 488.281µs
    nb_tick = milliseconds * 2 + ( ( 6 * milliseconds ) >> 7 );
    return nb_tick;
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static uint32_t rtc_get_calendar_time( uint16_t* milliseconds_div_10 ) {
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    uint32_t        ticks;

    uint64_t timestamp_in_ticks = rtc_get_timestamp_in_ticks( &date, &time );

    uint32_t seconds = ( uint32_t )( timestamp_in_ticks >> N_PREDIV_S );

    ticks = ( uint32_t ) timestamp_in_ticks & PREDIV_S;

    *milliseconds_div_10 = rtc_tick_2_ms( ticks );

    return seconds;
}

static uint64_t rtc_get_timestamp_in_ticks( RTC_DateTypeDef* date, RTC_TimeTypeDef* time ) {
    uint64_t timestamp_in_ticks = 0;
    uint32_t correction;
    uint32_t seconds;

    // Make sure it is correct due to asynchronous nature of RTC
    volatile uint32_t ssr;

    do {
        ssr = RTC->SSR;
        HAL_RTC_GetDate( &bsp_rtc.handle, date, RTC_FORMAT_BIN );
        HAL_RTC_GetTime( &bsp_rtc.handle, time, RTC_FORMAT_BIN );
    } while( ssr != RTC->SSR );

    // Calculate amount of elapsed days since 01/01/2000
    seconds = DIVC( ( DAYS_IN_YEAR * 3 + DAYS_IN_LEAP_YEAR ) * date->Year, 4 );

    correction = ( ( date->Year % 4 ) == 0 ) ? DAYS_IN_MONTH_CORRECTION_LEAP : DAYS_IN_MONTH_CORRECTION_NORM;

    seconds +=
        ( DIVC( ( date->Month - 1 ) * ( 30 + 31 ), 2 ) - ( ( ( correction >> ( ( date->Month - 1 ) * 2 ) ) & 0x03 ) ) );

    seconds += ( date->Date - 1 );

    // Convert from days to seconds
    seconds *= SECONDS_IN_1DAY;

    seconds += ( ( uint32_t ) time->Seconds + ( ( uint32_t ) time->Minutes * SECONDS_IN_1MINUTE ) +
                 ( ( uint32_t ) time->Hours * SECONDS_IN_1HOUR ) );

    timestamp_in_ticks = ( ( ( uint64_t ) seconds ) << N_PREDIV_S ) + ( PREDIV_S - time->SubSeconds );

    return timestamp_in_ticks;
}

void rtc_set_timestamp( RTC_DateTypeDef date, RTC_TimeTypeDef time ) {
    HAL_RTC_SetDate( &bsp_rtc.handle, &date, RTC_FORMAT_BIN );
    HAL_RTC_SetTime( &bsp_rtc.handle, &time, RTC_FORMAT_BIN );
}

void rtc_get_timestamp( RTC_DateTypeDef* date, RTC_TimeTypeDef* time ) {
    // Make sure it is correct due to asynchronous nature of RTC
    volatile uint32_t ssr;

    do {
        ssr = RTC->SSR;
        HAL_RTC_GetDate( &bsp_rtc.handle, date, RTC_FORMAT_BIN );
        HAL_RTC_GetTime( &bsp_rtc.handle, time, RTC_FORMAT_BIN );
    } while( ssr != RTC->SSR );
}

void RTC_IRQHandler( void ) {
    HAL_RTCEx_WakeUpTimerIRQHandler( &bsp_rtc.handle );

    if( wakeup_timer_rounds == 0 ) {
        wut_timer_irq_happened = true;
        TimerIrqHandler();
    } else {
        wakeup_timer_rounds--;
        HAL_RTCEx_DeactivateWakeUpTimer( &bsp_rtc.handle );
        HAL_RTCEx_SetWakeUpTimer_IT( &bsp_rtc.handle, 65535, RTC_WAKEUPCLOCK_RTCCLK_DIV16 );
    }
}

void HAL_RTC_MspInit( RTC_HandleTypeDef* rtc_handle ) {
    __HAL_RCC_RTC_ENABLE( );
    HAL_NVIC_SetPriority( RTC_IRQn, 0, 0 );
    HAL_NVIC_EnableIRQ( RTC_IRQn );
}

void HAL_RTC_MspDeInit( RTC_HandleTypeDef* rtc_handle ) {
    __HAL_RCC_RTC_DISABLE( );
    HAL_NVIC_DisableIRQ( RTC_IRQn );
}

/* --- EOF ------------------------------------------------------------------ */
