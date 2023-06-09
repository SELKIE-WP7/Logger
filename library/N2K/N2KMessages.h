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

#ifndef SELKIELoggerN2K_Messages
#define SELKIELoggerN2K_Messages

/*!
 * @file N2KMessages.h Message specific formats and decoders
 * @ingroup SELKIELoggerN2K
 */

#include "SELKIELoggerBase.h"
#include <stdbool.h>
#include <stdint.h>

#include "N2KTypes.h"

/*!
 * @addtogroup SELKIELoggerN2K
 * @{
 */

//! Convert raw angular value to degrees
#define N2K_TO_DEGREES 0.0057295779513082332

/*!
 * @addtogroup SELKIELoggerN2Kget Data conversion functions for N2K messages
 * @{
 */

//! Extract signed byte from N2K Message
int8_t n2k_get_int8(const n2k_act_message *n, size_t offset);

//! Extract unsigned byte from N2K Message
uint8_t n2k_get_uint8(const n2k_act_message *n, size_t offset);

//! Extract signed 16-bit integer from N2K Message
int16_t n2k_get_int16(const n2k_act_message *n, size_t offset);

//! Extract unsigned 16-bit integer from N2K Message
uint16_t n2k_get_uint16(const n2k_act_message *n, size_t offset);

//! Extract signed 32-bit integer from N2K Message
int32_t n2k_get_int32(const n2k_act_message *n, size_t offset);

//! Extract unsigned 32-bit integer from N2K Message
uint32_t n2k_get_uint32(const n2k_act_message *n, size_t offset);

//! Extract signed 64-bit integer from N2K Message
int64_t n2k_get_int64(const n2k_act_message *n, size_t offset);

//! Extract unsigned 64-bit integer from N2K Message
uint64_t n2k_get_uint64(const n2k_act_message *n, size_t offset);

//! Extract double from underlying integer data
double n2k_get_double(const n2k_act_message *n, size_t offset, uint8_t size);

//! Extract double from underlying unsigned integer data
double n2k_get_udouble(const n2k_act_message *n, size_t offset, uint8_t size);
//! @}

/*!
 * @addtogroup SELKIELoggerN2KPGN PGN Specific helper functions
 * @{
 */

//! Extract values from PGN  60928: ISO Adddress Claim
bool n2k_60928_values(const n2k_act_message *n, uint32_t *id, uint16_t *mfr, uint8_t *inst, uint8_t *fn, uint8_t *class,
                      uint8_t *sys, uint8_t *ind, bool *cfg);

//! Extract values from PGN 127250: Vessel Heading
bool n2k_127250_values(const n2k_act_message *n, uint8_t *seq, double *hdg, double *dev, double *var, uint8_t *ref);

//! Extract values from PGN 127251: Rate of Turn
bool n2k_127251_values(const n2k_act_message *n, uint8_t *seq, double *rate);

//! Extract values from PGN 127257: Device orientation
bool n2k_127257_values(const n2k_act_message *n, uint8_t *seq, double *yaw, double *pitch, double *roll);

//! Extract values from PGN 128267: Water depth
bool n2k_128267_values(const n2k_act_message *n, uint8_t *seq, double *depth, double *offset, double *range);

//! Extract values from PGN 129025: Device position
bool n2k_129025_values(const n2k_act_message *n, double *lat, double *lon);

//! Extract values from PGN 129026: Course and speed
bool n2k_129026_values(const n2k_act_message *n, uint8_t *seq, uint8_t *mag, double *course, double *speed);

//! Extract values from PGN 129029: GNSS Position
bool n2k_129029_values(const n2k_act_message *n, uint8_t *seq, uint16_t *epochDays, double *seconds, double *lat,
                       double *lon, double *alt, uint8_t *type, uint8_t *method, uint8_t *integ, uint8_t *nsv,
                       double *hdop, double *pdop, double *geos, uint8_t *rs, uint8_t *rst, uint16_t *rsid,
                       double *dgnssa);

//! Extract values from PGN 129033: Date/Time
bool n2k_129033_values(const n2k_act_message *n, uint16_t *epochDays, double *seconds, int16_t *utcMins);

//! Extract values from PGN 130306: Wind speed and direction
bool n2k_130306_values(const n2k_act_message *n, uint8_t *seq, uint8_t *ref, double *speed, double *angle);

//! Extract values from PGN 130311: Environmental data
bool n2k_130311_values(const n2k_act_message *n, uint8_t *seq, uint8_t *tid, uint8_t *hid, double *temp, double *humid,
                       double *press);

//! Print common message elements
void n2k_header_print(const n2k_act_message *n, const char d);

//! Print basic PGN details to standard output
void n2k_basic_print(const n2k_act_message *n);

//! Print PGN 60928 (Address claim) to standard output
void n2k_60928_print(const n2k_act_message *n);

//! Print PGN 127250 (Vessel Heading) to standard output
void n2k_127250_print(const n2k_act_message *n);

//! Print PGN 127251 (Rate of Turn) to standard output
void n2k_127251_print(const n2k_act_message *n);

//! Print PGN 127257 (Device orientation) to standard output
void n2k_127257_print(const n2k_act_message *n);

//! Print PGN 128267 (Water depth) to standard output
void n2k_128267_print(const n2k_act_message *n);

//! Print PGN 129025 (Device position) to standard output
void n2k_129025_print(const n2k_act_message *n);

//! Print PGN 129026 (Course and Speed) to standard output
void n2k_129026_print(const n2k_act_message *n);

//! Print PGN 129029 (GNSS Position) to standard output
void n2k_129029_print(const n2k_act_message *n);

//! Print PGN 129033 (Date and Time) to standard output
void n2k_129033_print(const n2k_act_message *n);

//! Print PGN 130306 (Wind speed and direction) to standard output
void n2k_130306_print(const n2k_act_message *n);

//! Print PGN 130311 (Environmental data) to standard output
void n2k_130311_print(const n2k_act_message *n);
//! @}

//! @}
#endif
