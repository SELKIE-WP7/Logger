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

#ifndef SELKIELoggerNMEA_Types
#define SELKIELoggerNMEA_Types

/*!
 * @file NMEATypes.h Serial Data types and definitions for communication with NMEA devices
 * @ingroup SELKIELoggerNMEA
 *
 * It should be noted that this library has not been written based on the
 * official NMEA specifications, but based on the information available from
 * the gpsd project at https://gpsd.gitlab.io/gpsd/NMEA.html
 *
 * Testing has been carried out with a small number of devices, but
 * compatibility with specific devices is not guaranteed.
 */

#include "SELKIELoggerBase.h"

/*!
 * @addtogroup SELKIELoggerNMEA
 * @{
 */

//! @brief NMEA0183 Start Byte 1
#define NMEA_START_BYTE1 0x24

//! @brief NMEA Start Byte 2
#define NMEA_START_BYTE2 0x21

//! NMEA Checksum Delimiter
#define NMEA_CSUM_MARK   0x2a

//! NMEA End Byte 1: Carriage Return
#define NMEA_END_BYTE1   0x0d

//! NMEA End Byte 1: Line Feed
#define NMEA_END_BYTE2   0x0a

/*!
 * Messages start with either START byte, up to 79 bytes of ASCII data, and are
 * terminated by a CR-LF pair.
 *
 * Standard messages start with a talker ID (2 chars for official, 4 chars
 * beginning with P for proprietary) and a message ID (3 chars), followed by
 * comma separated fields with message specific meanings.
 *
 * A checksum may/should be present after an asterisk (NMEA_CSUM_MARK), which
 * is represented as two hex digits.
 */

//! Generic NMEA message structure

/*!
 * Initially all fields should be 0/NULL.
 *
 * If numfields is > 0 and fields is allocated, then the contents of this array
 * should take precedence over the raw array.
 */
typedef struct {
	bool encapsulated; //!< Encapsulated message (all data in raw)
	char talker[4];    //!< Talker ID (2-4 characters)
	char message[3];   //!< Message ID
	uint8_t rawlen;    //!< Length of data stored in raw
	uint8_t raw[80];   //!< Raw data. Up to 79 chars if encapsulated, otherwise will be
	                   //!< shorter
	strarray fields;   //!< If parsed, array of fields
	uint8_t checksum;  //!< Message Checksum
} nmea_msg_t;

//! @}
#endif
