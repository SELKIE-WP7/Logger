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

#ifndef SELKIELoggerGPS_Serial
#define SELKIELoggerGPS_Serial

/*!
 * @file GPSSerial.h Serial Input/Output functions for communication with u-blox GPS
 * modules
 * @ingroup SELKIELoggerGPS
 */

#include "GPSTypes.h"
#include "SELKIELoggerBase.h"

/*!
 * @addtogroup SELKIELoggerGPS
 * @{
 */

//! Serial buffer size
#define UBX_SERIAL_BUFF 4096

//! Set up a connection to a UBlox module on a given port
int ubx_openConnection(const char *port, const int initialBaud);

//! Close a connection opened with ubx_openConnection()
void ubx_closeConnection(int handle);

//! Static wrapper around ubx_readMessage_buf()
bool ubx_readMessage(int handle, ubx_message *out);

//! Read data from handle, and parse message if able
bool ubx_readMessage_buf(int handle, ubx_message *out, uint8_t buf[UBX_SERIAL_BUFF], int *index, int *hw);

//! Read (and discard) messages until required message seen or timeout reached
bool ubx_waitForMessage(const int handle, const uint8_t msgClass, const uint8_t msgID, const int maxDelay,
                        ubx_message *out);

//! Send message to attached device
bool ubx_writeMessage(int handle, const ubx_message *out);
//! @}
#endif
