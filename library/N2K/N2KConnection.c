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

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "N2KConnection.h"

#include "SELKIELoggerBase.h"

/*!
 * Delegates to openSerialConnection() to open the serial device.
 *
 * @param[in] device Path to device to open
 * @param[in] baud Baud rate to connect with (as integer)
 * @returns Status of openSerialConnection() call
 */
int n2k_openConnection(const char *device, const int baud) {
	if (device == NULL) { return -1; }
	return openSerialConnection(device, baud);
}

/*!
 * @param[in] handle File handle from n2k_openConnection()
 */
void n2k_closeConnection(int handle) {
	errno = 0;
	close(handle);
}

/*!
 * For single threaded development and testing, uses static variables rather
 * than requiring state to be tracked by caller.
 *
 * See n2k_act_readMessage_buf() for full description.
 *
 * @param[in] handle File descriptor from n2k_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @return True if out now contains a valid message, false otherwise.
 */
bool n2k_act_readMessage(int handle, n2k_act_message *out) {
	static uint8_t buf[N2K_BUFF];
	static size_t index = 0; // Current read position
	static size_t hw = 0;    // Current array end
	return n2k_act_readMessage_buf(handle, out, buf, &index, &hw);
}

/*!
 * Pulls data from `handle` and stores it in `buf`, tracking the current search
 * position in `index` and the current fill level/buffer high water mark in `hw`
 *
 * If a valid message is found then it is written to the structure provided as
 * a parameter and the function returns true.
 *
 * If a message cannot be read, the function returns false
 *
 * @param[in] handle File descriptor from n2k_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @param[in,out] buf Serial data buffer
 * @param[in,out] index Current search position within `buf`
 * @param[in,out] hw End of current valid data in `buf`
 * @return True if out now contains a valid message, false otherwise.
 *
 */
bool n2k_act_readMessage_buf(int handle, n2k_act_message *out, uint8_t buf[N2K_BUFF], size_t *index, size_t *hw) {
	int ti = 0;
	if ((*hw) < N2K_BUFF - 1) {
		errno = 0;
		ti = read(handle, &(buf[(*hw)]), N2K_BUFF - (*hw));
		if (ti >= 0) {
			(*hw) += ti;
		} else {
			if (errno != EAGAIN) {
				fprintf(stderr, "Unexpected error while reading from serial port (handle ID: 0x%02x)\n",
				        handle);
				fprintf(stderr, "read returned \"%s\" in readMessage\n", strerror(errno));
				out->priority = 0xAA;
				return false;
			}
		}
	}
	if (((*hw) == N2K_BUFF) && (*index) > 0 && (*index) > ((*hw) - 25)) {
		// Full buffer, very close to the fill limit
		// Assume we're full of garbage before index
		memmove(buf, &(buf[(*index)]), N2K_BUFF - (*index));
		(*hw) -= (*index);
		(*index) = 0;
		out->priority = 0xFF;
		if (ti == 0) { out->priority = 0xFD; }
		return false;
	}

	n2k_act_message *t = NULL;
	bool r = n2k_act_from_bytes(buf, *hw, &t, index, false);
	if (t) {
		(*out) = (*t);
		free(t); // Shallow copied into out, so don't free ->data
		t = NULL;
	} else {
		out->priority = 0xEE;
	}

	if (!r) { out->priority = 0xEE; }

	if ((*hw) > 0 && ((*hw) >= (*index))) {
		// Move data from index back to zero position
		memmove(buf, &(buf[(*index)]), N2K_BUFF - (*index));
		(*hw) -= (*index);
		(*index) = 0;
	}
	return r;
}
