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

#ifndef SELKIELoggerDW_Messages
#define SELKIELoggerDW_Messages

/*!
 * @file DWMessages.h Utility functions for use with Datawell equipment messages
 * @ingroup SELKIELoggerDW
 */

#include <stdint.h>

#include "DWTypes.h"

/*!
 * @addtogroup SELKIELoggerDW
 * @{
 */

//! Extract cyclic data word from HXV input line
uint16_t dw_hxv_cycdat(const dw_hxv *in);

//! Extract vertical displacement component from HXV input line
int16_t dw_hxv_vertical(const dw_hxv *in);

//! Extract north displacement component from HXV input line
int16_t dw_hxv_north(const dw_hxv *in);

//! Extract west displacement component from HXV input line
int16_t dw_hxv_west(const dw_hxv *in);

//! Extract parity word from HXV input line
uint16_t dw_hxv_parity(const dw_hxv *in);

//! Populate dw_spectrum from array of cyclic data words
bool dw_spectrum_from_array(const uint16_t *arr, dw_spectrum *out);

//! Populate a specific component of dw_spectrum from array of cyclic data words
bool dw_spectral_block(const uint16_t *arr, const int ix, dw_spectrum *out);

//! Extract system data from array of cyclic data words
bool dw_system_from_array(const uint16_t *arr, dw_system *out);
//! @}
#endif
