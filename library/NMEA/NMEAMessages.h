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

#ifndef SELKIELoggerNMEA_Messages
#define SELKIELoggerNMEA_Messages

/*!
 * @file NMEAMessages.h Utility functions for processing NMEA messages
 * @ingroup SELKIELoggerNMEA
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "NMEATypes.h"

/*!
 * @addtogroup nmea NMEA Message support functions
 * @ingroup SELKIELoggerNMEA
 *
 * Convert and verify NMEA protocol messages
 * @{
 */

//! Calculate checksum for NMEA message
void nmea_calc_checksum(const nmea_msg_t *msg, uint8_t *cs);

//! Set checksum bytes for NMEA message
void nmea_set_checksum(nmea_msg_t *msg);

//! Verify checksum bytes of NMEA message
bool nmea_check_checksum(const nmea_msg_t *msg);

//! Calculate number of bytes required to represent message
size_t nmea_message_length(const nmea_msg_t *msg);

//! Convert NMEA message to array of bytes for transmission
size_t nmea_flat_array(const nmea_msg_t *msg, char **out);

//! Return NMEA message as string
char *nmea_string_hex(const nmea_msg_t *msg);

//! Print NMEA message
void nmea_print_hex(const nmea_msg_t *msg);

//! Parse raw data into fields
strarray *nmea_parse_fields(const nmea_msg_t *nmsg);

//! Get date/time from NMEA ZDA message
struct tm *nmea_parse_zda(const nmea_msg_t *msg);
//! @}
#endif
