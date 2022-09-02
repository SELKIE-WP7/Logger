#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "LPMSConnection.h"

#include "SELKIELoggerBase.h"

/*!
 * Delegates to openSerialConnection() to open the serial device.
 *
 * @param[in] device Path to device to open
 * @param[in] baud Baud rate to connect with (as integer)
 * @returns Status of openSerialConnection() call
 */
int lpms_openConnection(const char *device, const int baud) {
	if (device == NULL) { return -1; }
	return openSerialConnection(device, baud);
}

/*!
 * @param[in] handle File handle from lpms_openConnection()
 */
void lpms_closeConnection(int handle) {
	errno = 0;
	close(handle);
}

/*!
 * For single threaded development and testing, uses static variables rather
 * than requiring state to be tracked by caller.
 *
 * See lpms_readMessage_buf() for full description.
 *
 * @param[in] handle File descriptor from lpms_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @return True if out now contains a valid message, false otherwise.
 */
bool lpms_readMessage(int handle, lpms_message *out) {
	static uint8_t buf[LPMS_BUFF];
	static size_t index = 0; // Current read position
	static size_t hw = 0;    // Current array end
	return lpms_readMessage_buf(handle, out, buf, &index, &hw);
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
 * @param[in] handle File descriptor from lpms_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @param[in,out] buf Serial data buffer
 * @param[in,out] index Current search position within `buf`
 * @param[in,out] hw End of current valid data in `buf`
 * @return True if out now contains a valid message, false otherwise.
 *
 */
bool lpms_readMessage_buf(int handle, lpms_message *out, uint8_t buf[LPMS_BUFF], size_t *index, size_t *hw) {
	int ti = 0;
	if ((*hw) < LPMS_BUFF - 1) {
		errno = 0;
		ti = read(handle, &(buf[(*hw)]), LPMS_BUFF - (*hw));
		if (ti >= 0) {
			(*hw) += ti;
		} else {
			if (errno != EAGAIN) {
				fprintf(stderr, "Unexpected error while reading from serial port (handle ID: 0x%02x)\n",
				        handle);
				fprintf(stderr, "read returned \"%s\" in readMessage\n", strerror(errno));
				out->id = 0xAA;
				return false;
			}
		}
	}
	if (((*hw) == LPMS_BUFF) && (*index) > 0 && (*index) > ((*hw) - 25)) {
		// Full buffer, very close to the fill limit
		// Assume we're full of garbage before index
		memmove(buf, &(buf[(*index)]), LPMS_BUFF - (*index));
		(*hw) -= (*index);
		(*index) = 0;
		out->id = 0xFF;
		if (ti == 0) { out->id = 0xFD; }
		return false;
	}

	lpms_message t = {0};
	bool r = lpms_from_bytes(buf, *hw, &t, index);
	if (r) {
		(*out) = t;
	} else {
		out->id = 0xEE;
	}

	if ((*hw) > 0 && ((*hw) >= (*index))) {
		// Move data from index back to zero position
		memmove(buf, &(buf[(*index)]), LPMS_BUFF - (*index));
		(*hw) -= (*index);
		(*index) = 0;
	}
	return r;
}
