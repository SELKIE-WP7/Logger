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

#ifndef SELKIELoggerGPS_Commands
#define SELKIELoggerGPS_Commands

/*!
 * @file GPSCommands.h Functions representing u-blox GPS module commands
 * @ingroup SELKIELoggerGPS
 */

#include <stdbool.h>
#include <stdint.h>

#include "GPSTypes.h"

/*!
 * @defgroup ubxCommands UBX Commands
 * @ingroup SELKIELoggerGPS
 *
 * Send commands to a connected GPS module
 * @{
 */
//! Send UBX port configuration to switch baud rate
bool ubx_setBaudRate(const int handle, const uint32_t baud);

//! Send UBX rate command to enable/disable message types
bool ubx_setMessageRate(const int handle, const uint8_t msgClass, const uint8_t msgID, const uint8_t rate);

//! Request specific message by class and ID
bool ubx_pollMessage(const int handle, const uint8_t msgClass, const uint8_t msgID);

//! Enable Galileo constellation use
bool ubx_enableGalileo(const int handle);

//! Set UBX navigation calculation and reporting rate
bool ubx_setNavigationRate(const int handle, const uint16_t interval, const uint16_t outputRate);

//! Enable log/information messages from GPS device
bool ubx_enableLogMessages(const int handle);

//! Disable log/information messages from GPS device
bool ubx_disableLogMessages(const int handle);

//! Set I2C address
bool ubx_setI2CAddress(const int handle, const uint8_t addr);

//! @}
#endif
