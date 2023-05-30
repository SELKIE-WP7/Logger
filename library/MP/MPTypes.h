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

#ifndef SELKIELoggerMP_Types
#define SELKIELoggerMP_Types

/*!
 * @file MPTypes.h Serial Data types and definitions for compatible MessagePack data sources
 * @ingroup SELKIELoggerMP
 */

#include "SELKIELoggerBase.h"

/*!
 * @addtogroup SELKIELoggerMP
 * @{
 */

/*! @brief MP Serial synchronisation byte 1
 *
 * 0x94 is the MessagePack header for a 4 element array
 */
#define MP_SYNC_BYTE1 0x94

/*! @brief MP Serial synchronisation byte 2
 *
 * 0x55 (0b01010101) represents 83 as a MessagePack fixed integer.
 */
#define MP_SYNC_BYTE2 0x55

/*!
 * @brief Internal representation of SELKIE logger messages
 *
 * All messages are MessagePacked arrays with 4 elements.
 * First element is fixed integer 0x55
 * Second element is a source ID (0-127)
 * Third element is a channel ID (0-127)
 * Fourth element is source specific data, which can be:
 *  - A string
 *  - A floating point value (single precision)
 *  - A millisecond precision timestamp (uint32_t)
 *  - An array of strings
 *  - An array of bytes
 *
 * These elements are the same as the members of the msg_t type defined in
 * base/messages.h, with the addition of the initial sync byte.
 *
 * The use of the initial sync byte and fixed length array provide a two byte
 * signature (0x94 0x55) that can be used to identify message boundaries.
 *
 * There is no checksumming of transmitted messages under this scheme.
 */
typedef msg_t mp_message_t;
//! @}
#endif
