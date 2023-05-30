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

#ifndef SELKIELoggerBase_Serial
#define SELKIELoggerBase_Serial

/*!
 * @file serial.h Generic serial connection and utility functions
 * @ingroup SELKIELoggerBase
 */

/*!
 * @addtogroup SELKIELoggerBase
 * @{
 */
//! Open a serial connection at a given baud rate
int openSerialConnection(const char *port, const int baudRate);

//! Convert a numerical baud rate to termios flag
int baud_to_flag(const int rate);

//! Convert a termios baud rate flag to a numerical value
int flag_to_baud(const int flag);
//! @}
#endif
