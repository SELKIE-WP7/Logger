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

#ifndef SELKIELoggerNMEA_Serial
#define SELKIELoggerNMEA_Serial

/*!
 * @file NMEASerial.h Serial Input/Output functions for communication with NMEA devices
 * @ingroup SELKIELoggerNMEA
 */

#include "SELKIELoggerBase.h"

#include "NMEATypes.h"

/*!
 * @addtogroup SELKIELoggerNMEA
 * @{
 */
//! Default serial buffer allocation size.
#define NMEA_SERIAL_BUFF 1024

//! Set up a connection to the specified port
int nmea_openConnection(const char *port, const int baudRate);

//! Close existing connection
void nmea_closeConnection(int handle);

//! Static wrapper around mp_readMessage_buf
bool nmea_readMessage(int handle, nmea_msg_t *out);

//! Read data from handle, and parse message if able
bool nmea_readMessage_buf(int handle, nmea_msg_t *out, uint8_t buf[NMEA_SERIAL_BUFF], int *index, int *hw);

//! Send message to attached device
bool nmea_writeMessage(int handle, const nmea_msg_t *out);
//! @}
#endif
