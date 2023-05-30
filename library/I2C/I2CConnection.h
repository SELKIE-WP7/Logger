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

#ifndef SELKIELoggerI2C_Connection
#define SELKIELoggerI2C_Connection

#include <stdint.h>

/*!
 * @file I2CConnection.h General connection functions for I2C bus connected sensors
 * @ingroup SELKIELoggerI2C
 */

/*!
 * @addtogroup SELKIELoggerI2C
 * @{
 */

//! Device specific callback functions
typedef float (*i2c_dev_read_fn)(const int, const int, const void *);

//! Set up a connection to the specified bus
int i2c_openConnection(const char *bus);

//! Close existing connection
void i2c_closeConnection(int handle);

//! Swap word byte order
int16_t i2c_swapbytes(const int16_t in);
//! @}
#endif
