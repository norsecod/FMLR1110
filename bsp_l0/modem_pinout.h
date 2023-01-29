/*!
 * \file      modem_pinout.h
 *
 * \brief     modem specific pinout
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
#ifndef __MODEM_PIN_NAMES_H__
#define __MODEM_PIN_NAMES_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include "smtc_hal_gpio_pin_names.h"

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC CONSTANTS --------------------------------------------------------
 */

/********************************************************************************/
/*                         Application     dependant                            */
/********************************************************************************/
// clang-format off

//Debug uart specific pinout for debug print
#define DEBUG_UART_TX           PA_9
#define DEBUG_UART_RX           PA_10

//Radio specific pinout and peripherals
#define RADIO_SPI_MOSI          PB_15
#define RADIO_SPI_MISO          PB_14
#define RADIO_SPI_SCLK          PB_10
#define RADIO_NSS               PA_4
#define RADIO_DIOX              PA_1

#define RADIO_SPI_ID            2

#define SPI1_ENABLED
#define SPI1_MOSI               PB_5
#define SPI1_MISO               PB_4
#define SPI1_SCK                PB_3

#define I2C1_ENABLED
#define I2C1_SDA                PB_9
#define I2C1_SCL                PB_6

#if defined (SX126X)
#define SX126X_RADIO_RF_SWITCH_CTRL    PA_9
#define RADIO_BUSY_PIN          PC_4
#define RADIO_NRST              PB_11
#endif

#if defined (LR11XX)
#define RADIO_BUSY_PIN          PA_7
#define RADIO_NRST              PC_2
#endif

#if defined (SX128X)
// For sx128x eval board with 2 antennas
#define RADIO_ANTENNA_SWITCH    PB_0
#endif

#if defined( LR11XX_TRANSCEIVER ) && defined( ENABLE_MODEM_GNSS_FEATURE )
// LR11XX_TRANSCEIVER - Use for GNSS LNA control
#define RADIO_LNA_CTRL          PB_0
#endif

//Hw modem specific pinout
#define HW_MODEM_COMMAND_PIN    NC
#define HW_MODEM_EVENT_PIN      NC
#define HW_MODEM_BUSY_PIN       NC
#define HW_MODEM_TX_LINE        NC
#define HW_MODEM_RX_LINE        NC

// clang-format on

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

#ifdef __cplusplus
}
#endif

#endif  //__MODEM_PIN_NAMES_H__