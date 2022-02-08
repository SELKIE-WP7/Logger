#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "NMEAMessages.h"
#include "NMEASerial.h"
#include "NMEATypes.h"

/*!
 * Currently just wraps openSerialConnection()
 *
 * @param[in] port Target character device (i.e. Serial port)
 * @param[in] baudRate Target baud rate
 * @return Return value of openSerialConnection()
 */
int nmea_openConnection(const char *port, const int baudRate) {
	return openSerialConnection(port, baudRate);
}

/*!
 * Currently just closes the file descriptor passed as `handle`
 *
 * @param[in] handle File descriptor opened with nmea_openConnection()
 */
void nmea_closeConnection(int handle) {
	close(handle);
}

/*!
 * For single threaded development and testing, uses static variables rather
 * than requiring state to be tracked by caller.
 *
 * See nmea_readMessage_buf() for full description.
 *
 * @param[in] handle File descriptor from nmea_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @return True if out now contains a valid message, false otherwise.
 */
bool nmea_readMessage(int handle, nmea_msg_t *out) {
	static uint8_t buf[NMEA_SERIAL_BUFF];
	static int index = 0; // Current read position
	static int hw = 0;    // Current array end
	return nmea_readMessage_buf(handle, out, buf, &index, &hw);
}

/*!
 * Pulls data from `handle` and stores it in `buf`, tracking the current search
 * position in `index` and the current fill level/buffer high water mark in `hw`
 *
 * If a valid message is found then it is written to the structure provided as
 * a parameter and the function returns true.
 *
 * If a message cannot be read, the function returns false and the first byte
 * of the raw array is set to an error value:
 *
 * - 0xFF means no message found yet, and more data is required
 * - 0xFD is a synonym for 0xFF, but indicates that zero bytes were read from source.
 *   This could indicate EOF if reading from file, but can be ignored when streaming from
 * a device.
 * - 0xAA means that an error occurred reading in data
 * - 0XEE means a valid message header was found, but no valid message
 *
 * @param[in] handle File descriptor from nmea_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @param[in,out] buf Serial data buffer
 * @param[in,out] index Current search position within `buf`
 * @param[in,out] hw End of current valid data in `buf`
 * @return True if out now contains a valid message, false otherwise.
 *
 */
bool nmea_readMessage_buf(int handle, nmea_msg_t *out, uint8_t buf[NMEA_SERIAL_BUFF], int *index, int *hw) {
	int ti = 0;
	if ((*hw) < NMEA_SERIAL_BUFF - 1) {
		errno = 0;
		ti = read(handle, &(buf[(*hw)]), NMEA_SERIAL_BUFF - (*hw));
		if (ti >= 0) {
			(*hw) += ti;
		} else {
			if (errno != EAGAIN) {
				fprintf(stderr, "Unexpected error while reading from serial port (handle ID: 0x%02x)\n",
				        handle);
				fprintf(stderr, "read returned \"%s\" in readMessage\n", strerror(errno));
				out->rawlen = 1;
				out->raw[0] = 0xAA;
				return false;
			}
		}
	}
	if (((*hw) == NMEA_SERIAL_BUFF) && (*index) > 0 && (*index) > (*hw) - 8) {
		// Full buffer, very close to the fill limit
		// Assume we're full of garbage before index
		memmove(buf, &(buf[(*index)]), NMEA_SERIAL_BUFF - (*index));
		(*hw) -= (*index);
		(*index) = 0;
		out->rawlen = 1;
		out->raw[0] = 0xFF;
		if (ti == 0) { out->raw[0] = 0xFD; }
		return false;
	}
	// Check buf[index] matches either of the valid start bytes
	while (!(buf[(*index)] == NMEA_START_BYTE1 || buf[(*index)] == NMEA_START_BYTE2) && (*index) < (*hw)) {
		(*index)++; // Current byte cannot be start of a message, so advance
	}
	if ((*index) == (*hw)) {
		if ((*hw) > 0 && (*index) > 0) {
			// Move data from index back to zero position
			memmove(buf, &(buf[(*index)]), NMEA_SERIAL_BUFF - (*index));
			(*hw) -= (*index);
			(*index) = 0;
		}
		out->rawlen = 1;
		out->raw[0] = 0xFF;
		if (ti == 0) { out->raw[0] = 0xFD; }
		return false;
	}

	if (((*hw) - (*index)) < 8) {
		// Not enough data for any valid message, come back later
		out->rawlen = 1;
		out->raw[0] = 0xFF;
		if (ti == 0) { out->raw[0] = 0xFD; }
		return false;
	}

	int eom = (*index) + 1;
	// Spec says \r\n, but the USB gateway seems to do \n\n at startup
	while (!(buf[eom] == NMEA_END_BYTE2 && (buf[eom - 1] == NMEA_END_BYTE1 || buf[eom - 1] == NMEA_END_BYTE2)) &&
	       eom < (*hw)) {
		eom++; // Current byte cannot be start of a message, so advance
	}

	if ((eom - (*index)) > 82) {
		// No end of message found, so mark as invalid and increment index
		(*index)++;
		out->rawlen = 1;
		out->raw[0] = 0xEE;
		return false;
	}

	if (eom == (*hw)) {
		// Not incrementing index or marking invalid, as could still be
		// a valid message once we read the rest of it in.
		out->rawlen = 1;
		out->raw[0] = 0xFF;
		if (ti == 0) { out->raw[0] = 0xFD; }
		return false;
	}

	// So we now have a candidate message that starts at (*index) and ends at eom
	int som = (*index);

	// Can only be one of the start bytes, so no explicit check for BYTE1
	out->encapsulated = (buf[som++] == NMEA_START_BYTE2);

	out->talker[0] = buf[som++];
	out->talker[1] = buf[som++];
	if (out->talker[0] == 'P') {
		out->talker[2] = buf[som++];
		out->talker[3] = buf[som++];
	}
	out->message[0] = buf[som++];
	out->message[1] = buf[som++];
	out->message[2] = buf[som++];

	if (buf[som++] != ',') {
		out->rawlen = 1;
		out->raw[0] = 0xEE; // Invalid message
		(*index)++;
		return false;
	}

	out->rawlen = 0;
	while (buf[som] != NMEA_CSUM_MARK && buf[som] != NMEA_END_BYTE1 && buf[som + 1] != NMEA_END_BYTE2) {
		out->raw[out->rawlen++] = buf[som++];
	}

	if (buf[som] == NMEA_CSUM_MARK) {
		som++; // Skip the delimiter
		uint8_t cs = 0;
		char csA = buf[som++];
		char csB = buf[som++];
		bool valid = true;

		switch (csA) {
			case 'f':
			case 'e':
			case 'd':
			case 'c':
			case 'b':
			case 'a':
				cs = (csA - 'a') + 10;
				break;
			case 'F':
			case 'E':
			case 'D':
			case 'C':
			case 'B':
			case 'A':
				cs = (csA - 'A') + 10;
				break;
			case '9':
			case '8':
			case '7':
			case '6':
			case '5':
			case '4':
			case '3':
			case '2':
			case '1':
			case '0':
				cs = (csA - '0');
				break;
			default:
				valid = false;
				break;
		}

		cs = (cs << 4);
		switch (csB) {
			case 'f':
			case 'e':
			case 'd':
			case 'c':
			case 'b':
			case 'a':
				cs += (csB - 'a') + 10;
				break;
			case 'F':
			case 'E':
			case 'D':
			case 'C':
			case 'B':
			case 'A':
				cs += (csB - 'A') + 10;
				break;
			case '9':
			case '8':
			case '7':
			case '6':
			case '5':
			case '4':
			case '3':
			case '2':
			case '1':
			case '0':
				cs += (csB - '0');
				break;
			default:
				valid = false;
				break;
		}
		out->checksum = cs;
		cs = 0;
		nmea_calc_checksum(out, &cs);
		if (cs != out->checksum) { valid = false; }

		if (!valid) {
			out->rawlen = 1;
			out->raw[0] = 0xEE;
			(*index)++;
			return false;
		}
	}

	if ((eom - som) > 1) {
		// Urmmm....Oops?
		out->rawlen = 1;
		out->raw[0] = 0xEE;
		(*index)++;
		return false;
	}

	(*index) = eom++;
	if ((*hw) > 0) {
		// Move data from index back to zero position
		memmove(buf, &(buf[(*index)]), NMEA_SERIAL_BUFF - (*index));
		(*hw) -= (*index);
		(*index) = 0;
	}
	return true;
}

/*!
 * Takes a message, validates the checksum and writes it out to the device or
 * file connected to `handle`.
 *
 * @param[in] handle File descriptor from nmea_openConnection()
 * @param[in] out Pointer to message structure to be sent.
 * @return True if data successfullt written to `handle`
 */
bool nmea_writeMessage(int handle, const nmea_msg_t *out) {
	char *buf = NULL;
	size_t size = nmea_flat_array(out, &buf);
	int ret = write(handle, buf, size);
	free(buf);
	return (ret == size);
}
