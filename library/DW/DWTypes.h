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

#ifndef SELKIELoggerDW_Types
#define SELKIELoggerDW_Types

/*!
 * @file DWTypes.h Data types and definitions for use with Datawell equipment
 * @ingroup SELKIELoggerDW
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*!
 * @addtogroup SELKIELoggerDW
 * @{
 */

/*!
 * @addtogroup DWChanStatus Channel Status
 *
 * Channel Status indicators
 *
 * @{
 */

//! Channel OK
#define DW_CHAN_OK       '-'

//! Channel Repaired
#define DW_CHAN_REPAIRED '='

//! Channel Unrecoverable
#define DW_CHAN_BAD      '!'

//! @}

//! DW Data format types
typedef enum dw_types {
	DW_TYPE_UNKNOWN = -1, //!< Default - type not known
	DW_TYPE_HVA = 0,      //!< HVA format data
	DW_TYPE_HXV,          //! HXV format data
	DW_TYPE_BVA,          //! BVA format data
} dw_types;

/*!
 * @brief Internal representation of a Datawell HVA message
 *
 * Transmitted as text, terminated by a carriage return (not stored). The
 * sequence number and data arrays are transmitted as hexadecimal characters,
 * with two characters representing each byte stored.
 */
typedef struct dw_hva {
	uint8_t seq;     //!< Sequence number, transmitted as two hex characters
	char rtStatus;   //!< Real time channel status information. @sa DWChanStatus
	uint8_t rt[9];   //!< Real time data
	char packStatus; //!< Packet data status information. @sa DWChanStatus
	uint8_t pack[3]; //!< Packet data
} dw_hva;

/*!
 * @brief Internal representation of a Datawell BVA message
 *
 * The data is identical to that stored in a dw_hva structure, but without any
 * status or sequencing information.
 */
typedef struct dw_bva {
	uint8_t rt[9];   //!< Real time data
	uint8_t pack[3]; //!< Packet data
} dw_bva;

/*!
 * @brief Internal representation of a Datawell HXV message
 *
 * Transmitted as text, terminated by a carriage return (not stored).
 *
 * All data transmitted as hexadecimal characters, with two characters
 * representing each byte stored.
 */
typedef struct dw_hxv {
	uint8_t status;  //!< Error count. 0 or 1 OK, 2+ error
	uint8_t lines;   //!< Transmitted line number
	uint8_t data[8]; //!< 8 bytes of data
} dw_hxv;

/*!
 * @brief Internal representation of HXV spectral messages
 *
 * Extracted from individual HXV lines, providing 12 bits of system data (and
 * associated sequence number) and 4 sets/lines of spectral data.
 */
typedef struct {
	uint8_t sysseq;          //!< System data sequence number
	uint16_t sysword;        //!< 12 bits of system data
	uint8_t frequencyBin[4]; //!< Index for each line of spectral data
	float frequency[4];      //!< Frequency represented by each line
	float spread[4];         //!< Wave spread for each line
	float direction[4];      //!< Direction for each line
	float rpsd[4];           //!< Relative power spectral density for each line
	float m2[4];             //!< M2 Fourier coefficient for each line
	float n2[4];             //!< N2 Fourier coefficient for each line
	float K[4];              //!< Check factor for each line
} dw_spectrum;

/*!
 * @brief Internal representation of HXV system messages
 *
 * Extracted from the sequence of system words received as part of the
 * cyclic/spectral data messages.
 */
typedef struct {
	int number;      //!< Sequence number
	bool GPSfix;     //!< Valid GPS fix available
	float Hrms;      //!< RMS Wave height
	float fzero;     //!< Zero crossing frequency
	float PSD;       //!< Peak Power Spectral Density
	float refTemp;   //!< Reference Temperature
	float waterTemp; //!< Water Temperature
	int opTime;      //!< Weeks of battery life remaining
	int battStatus;  //!< Battery status
	float a_z_off;   //!< Vertical accelerometer offset
	float a_x_off;   //!< X-axis accelerometer offset
	float a_y_off;   //!< Y-axis accelerometer offset
	float lat;       //!< GPS Latitude
	float lon;       //!< GPS Longitude
	float orient;    //!< Buoy orientation
	float incl;      //!< Buoy inclination
} dw_system;

//! Convert a string of hexadecimal characters to corresponding value
bool hexpair_to_uint(const char *in, uint8_t *out);

//! Read a line of HXV data from string and convert
bool dw_string_hxv(const char *in, size_t *end, dw_hxv *out);
//! @}
#endif
