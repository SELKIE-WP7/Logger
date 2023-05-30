/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SELKIELoggerI2C_SN3218
#define SELKIELoggerI2C_SN3218

/*!
 * @file I2C-SN3218.h SN3218 Support functions
 * @ingroup SELKIELoggerI2C
 */

#include <stdbool.h>
#include <stdint.h>

//! SN3218 I2C Address
#define SN3218_ADDR_DEFAULT 0x54

/*!
 * @defgroup SN3218REG SN3218 Register Addresses
 * @ingroup SELKIELoggerI2C
 *
 * @{
 */

//! Global device enable register
#define SN3218_REG_ENABLE   0x00

/*!
 * @defgroup SN3218REGPWM SN3218 PWM Registers
 *
 * @ingroup SN3218REG
 *
 * Control the PWM duty cycle of each LED channel in 256 steps between 0 (Off)
 * and 255 (Maximum current)
 *
 * @{
 */

//! LED 1 PWM Intensity (0-255)
#define SN3218_REG_PWM_01   0x01

//! LED 2 PWM Intensity (0-255)
#define SN3218_REG_PWM_02   0x02

//! LED 3 PWM Intensity (0-255)
#define SN3218_REG_PWM_03   0x03

//! LED 4 PWM Intensity (0-255)
#define SN3218_REG_PWM_04   0x04

//! LED 5 PWM Intensity (0-255)
#define SN3218_REG_PWM_05   0x05

//! LED 6 PWM Intensity (0-255)
#define SN3218_REG_PWM_06   0x06

//! LED 7 PWM Intensity (0-255)
#define SN3218_REG_PWM_07   0x07

//! LED 8 PWM Intensity (0-255)
#define SN3218_REG_PWM_08   0x08

//! LED 9 PWM Intensity (0-255)
#define SN3218_REG_PWM_09   0x09

//! LED 10 PWM Intensity (0-255)
#define SN3218_REG_PWM_10   0x0A

//! LED 11 PWM Intensity (0-255)
#define SN3218_REG_PWM_11   0x0B

//! LED 12 PWM Intensity (0-255)
#define SN3218_REG_PWM_12   0x0C

//! LED 13 PWM Intensity (0-255)
#define SN3218_REG_PWM_13   0x0D

//! LED 14 PWM Intensity (0-255)
#define SN3218_REG_PWM_14   0x0E

//! LED 15 PWM Intensity (0-255)
#define SN3218_REG_PWM_15   0x0F

//! LED 16 PWM Intensity (0-255)
#define SN3218_REG_PWM_16   0x10

//! LED 17 PWM Intensity (0-255)
#define SN3218_REG_PWM_17   0x11

//! LED 18 PWM Intensity (0-255)
#define SN3218_REG_PWM_18   0x12
//! @}

/*!
 * @defgroup SN3218REGLED SN3218 LED Control Registers
 * @ingroup SN3218REG
 *
 * The lower 6 bits written to each register will enable/disable the associated
 * channels. The 2 high bits of each value are reserved and should be written
 * as 00.
 *
 * @{
 */

//! LEDs 1 - 6 control register
#define SN3218_REG_LED_01   0x13

//! LEDs 7-12 control register
#define SN3218_REG_LED_02   0x14

//! LEDs 13-18 control register
#define SN3218_REG_LED_03   0x15
//!@}

//! SN3218 Update register - set high to apply changes
#define SN3218_REG_UPDATE   0x16

//! SN3218 Reset register - set high to reset all control registers
#define SN3218_REG_RESET    0x17

//! @}

/*!
 * @defgroup SN3218Control SN3218 Control Functions
 * @ingroup SELKIELoggerI2C
 * @{
 */

/*!
 * @brief SN3218 State representation
 *
 * The SN3218 chip cannot be read to get current state information, so define a
 * software representation of its state for convenience.
 */
typedef struct {
	uint8_t led[18];    //!< Individual LED brightnesses
	bool global_enable; //!< Global on/off switch
} i2c_sn3218_state;

//! Reset device to defaults
bool i2c_sn3218_reset(const int busHandle);

//! Configure SN3218 chip to match state object
bool i2c_sn3218_update(const int busHandle, const i2c_sn3218_state *state);

//! @}
#endif
