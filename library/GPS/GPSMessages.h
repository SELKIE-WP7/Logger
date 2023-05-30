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

#ifndef SELKIELoggerGPS_Messages
#define SELKIELoggerGPS_Messages

/*!
 * @file GPSMessages.h Utility functions for processing UBX messages
 * @ingroup SELKIELoggerGPS
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "GPSTypes.h"

/*!
 * @defgroup ubx UBX Message handling
 * @ingroup SELKIELoggerGPS
 *
 * Convert and verify UBX protocol messages
 * @{
 */

//! Calculate checksum for UBX message
void ubx_calc_checksum(const ubx_message *msg, uint8_t *csA, uint8_t *csB);

//! Set checksum bytes for UBX message
void ubx_set_checksum(ubx_message *msg);

//! Verify checksum bytes of UBX message
bool ubx_check_checksum(const ubx_message *msg);

//! Convert UBX message to flat array of bytes
size_t ubx_flat_array(const ubx_message *msg, uint8_t **out);

//! Return UBX message as string of hexadecimal pairs
char *ubx_string_hex(const ubx_message *msg);

//! Print UBX message in hexadecimal form
void ubx_print_hex(const ubx_message *msg);

//! Decode UBX NAV-PVT message
bool ubx_decode_nav_pvt(const ubx_message *msg, ubx_nav_pvt *out);

//! @}
#endif
