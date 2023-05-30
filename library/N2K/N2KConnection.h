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

#ifndef SELKIELoggerN2K_Connection
#define SELKIELoggerN2K_Connection

#include <stdbool.h>

#include "N2KTypes.h"
#include "SELKIELoggerBase.h"

/*!
 * @file N2KConnection.h N2K connection handling
 * @ingroup SELKIELoggerN2K
 */

//! Default serial buffer allocation size
#define N2K_BUFF 1024

//! Open connection to an N2K serial device
int n2k_openConnection(const char *device, const int baud);

//! Close N2K serial connection
void n2k_closeConnection(int handle);

//! Static wrapper around n2k_readMessage_buf
bool n2k_act_readMessage(int handle, n2k_act_message *out);

//! Read data from handle, and parse message if able
bool n2k_act_readMessage_buf(int handle, n2k_act_message *out, uint8_t buf[N2K_BUFF], size_t *index, size_t *hw);

#endif
