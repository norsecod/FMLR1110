/*!
 * \file      smtc_hal_rtc.h
 *
 * \brief     RTC Hardware Abstraction Layer definition
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
#ifndef __RTC_UTILITIES_H__
#define __RTC_UTILITIES_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type
#include "stm32l0xx_hal.h"

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC CONSTANTS --------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

/*!
 *  Initializes the MCU RTC peripheral
 */
void hal_rtc_init( void );

/*!
 * Gets the RTC timer reference
 *
 * \retval value Timer value in ticks
 */
uint32_t hal_rtc_get_timer_context( void );

/*!
 * Sets the RTC timer reference
 *
 * \retval value Timer reference value in ticks
 */
uint32_t hal_rtc_set_timer_context( void );

/*!
 * Get the RTC timer elapsed time since the last Alarm was set
 *
 * \retval RTC Elapsed time since the last alarm in ticks.
 */
uint32_t hal_rtc_get_timer_elapsed_time( void );

/*!
 *  Returns the minimum timeout value
 *
 * \retval minTimeout Minimum timeout value in in ticks
 */
uint32_t hal_rtc_get_minimum_timeout( void );

/*!
 * Returns the current RTC time in seconds
 *
 * \remark Used for scheduling autonomous retransmissions (i.e: NbTrans),
 *         transmitting MAC answers, basically any delay without accurate time
 *         constraints. It is also used to measure the time spent inside the
 *         LoRaWAN process for the integrated failsafe.
 *
 * retval rtc_time_s Current RTC time in seconds
 */
uint32_t hal_rtc_get_time_s( void );

/*!
 * Returns the current RTC time in milliseconds
 *
 * \remark Used to timestamp radio events (i.e: end of TX), will also be used
 * for ClassB
 *
 * retval rtc_time_ms Current RTC time in milliseconds wraps every 49 days
 */
uint32_t hal_rtc_get_time_ms( void );

/*!
 * Returns the current RTC time in ticks
 *
 * retval rtc_time_ticks Current RTC time in ticks
 */
uint32_t hal_rtc_get_time_ticks( void );

/*!
 * Returns the current RTC time in 0.1milliseconds
 *
 * \remark will also be used for d2d
 *
 *
 * retval rtc_time_ms Current RTC time in milliseconds wraps every 4.9 days
 */
uint32_t hal_rtc_get_time_100us( void );

/*!
 * Sets the rtc wakeup timer for milliseconds parameter. The RTC will generate
 * an IRQ to wakeup the MCU.
 *
 * \param[IN] milliseconds Number of seconds before wakeup
 */
void hal_rtc_wakeup_timer_set_ms( const int32_t milliseconds );

/*!
 * Stop the rtc wakeup timer
 */
void hal_rtc_wakeup_timer_stop( void );

/*!
 * Return true if the wake up timer irq has been triggered
 *
 * \retval true if wut irq happended
 */
bool hal_rtc_has_wut_irq_happened( void );

/*!
 * Converts time in ms to time in wake up timer ticks
 * Assuming WUCKSEL[2:0] = 000: RTCCLK/16 clock is selected
 *
 * \param[IN] milliseconds Time in milliseconds
 * \retval ticks Time in wake up timer ticks
 */
uint32_t rtc_ms_2_wakeup_timer_tick( const uint32_t milliseconds );

/*!
 * Converts time in ticks to time in ms
 *
 * \param[IN] ticks Time in timer ticks
 * \retval milliseconds Time in milliseconds
 */
uint32_t rtc_tick_2_ms( const uint32_t tick );

/*!
 * Converts time in ms to time in ticks
 *
 * \param[IN] milliseconds Time in ms
 * \retval ticks Time in timer ticks
 */
uint32_t rtc_ms_2_ticks( const uint32_t milliseconds );

/*!
 * Set the date and time of the rtc
 *
 * \param[IN] date as RTC_DateTypeDef
 * \param[IN] time as RTC_TimeTypeDef
 */
void rtc_set_timestamp( RTC_DateTypeDef date, RTC_TimeTypeDef time );

/*!
 * Get the date and time of the rtc
 *
 * \param[OUT] date as RTC_DateTypeDef
 * \param[OUT] time as RTC_TimeTypeDef
 */
void rtc_get_timestamp( RTC_DateTypeDef* date, RTC_TimeTypeDef* time );

#ifdef __cplusplus
}
#endif

#endif  // __RTC_UTILITIES_H__

/* --- EOF ------------------------------------------------------------------ */
