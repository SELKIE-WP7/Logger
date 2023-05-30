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

#include "I2CConnection.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

/*!
 * Opens the specified bus in read/write mode
 *
 * @param[in] bus Path to I2C bus device
 * @return Handle to opened bus, or -1 on failure
 */
int i2c_openConnection(const char *bus) {
	errno = 0;
	int handle = open(bus, O_RDWR);
	if (handle < 0) { return -1; }
	return handle;
}

/*!
 * Close connection previously opened with i2c_openConnection()
 *
 * @param[in] handle File handle returned by i2c_openConnection()
 */
void i2c_closeConnection(int handle) {
	close(handle);
}

/*!
 * Swap the bytes read or written using the SMBus commands
 *
 * Not sure if the need to do this is a misunderstanding or a documentation issue.
 *
 * @param[in] in Word to be swapped
 * @return Input, with byte order reversed
 */
int16_t i2c_swapbytes(const int16_t in) {
	uint16_t tmp = in;
	return (int16_t)((tmp >> 8) + ((tmp & 0xFF) << 8));
}
