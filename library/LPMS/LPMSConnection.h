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

#ifndef SELKIELoggerLPMS_Connection
#define SELKIELoggerLPMS_Connection

#include <stdbool.h>

#include "SELKIELoggerBase.h"

#include "LPMSTypes.h"

/*!
 * @file LPMSConnection.h LPMS connection handling
 * @ingroup SELKIELoggerLPMS
 */

//! Default serial buffer allocation size
#define LPMS_BUFF 1024

//! Open connection to an LPMS serial device
int lpms_openConnection(const char *device, const int baud);

//! Close LPMS serial connection
void lpms_closeConnection(int handle);

//! Static wrapper around lpms_readMessage_buf
bool lpms_readMessage(int handle, lpms_message *out);

//! Read data from handle, and parse message if able
bool lpms_readMessage_buf(int handle, lpms_message *out, uint8_t buf[LPMS_BUFF], size_t *index, size_t *hw);

//! Read data from handle until first of specified message types is found
bool lpms_find_messages(int handle, size_t numtypes, const uint8_t types[], int timeout, lpms_message *out,
                        uint8_t buf[LPMS_BUFF], size_t *index, size_t *hw);

//! Write command defined by structure to handle
bool lpms_send_command(const int handle, lpms_message *m);

//! Shortcut: Send LPMS_MSG_MODE_CMD
bool lpms_send_command_mode(const int handle);

//! Shortcut: Send LPMS_MSG_MODE_STREAM
bool lpms_send_stream_mode(const int handle);

//! Shortcut: Send LPMS_MSG_GET_OUTPUTS
bool lpms_send_get_config(const int handle);
#endif
