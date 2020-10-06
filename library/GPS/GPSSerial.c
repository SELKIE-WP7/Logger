// Open serial port and communicate with UBlox GPS
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

#include "GPSTypes.h"
#include "GPSMessages.h"
#include "GPSSerial.h"
#include "GPSCommands.h"

/*!
 * Uses openSerialConnection() from the base library to open an initial
 * connection, then sends UBX commands to configure the module for 115200 baud
 * UBX output.
 *
 * If the module was already operating at 115200 then it will stop listening to
 * serial commands when we transmit at the wrong rate, so this function will
 * sleep for 1 second before configuring the protocols again.
 *
 * @param[in] port Path to character device connected to UBlox module
 * @param[in] initialBaud Initial baud rate for connection. Usually 9600, but may vary.
 * @return File descriptor for use with other commands
 */
int ubx_openConnection(const char *port, const int initialBaud) {
	// Use base library function to get initial connection
	int handle = openSerialConnection(port, initialBaud);

	// The set baud rate command also disables NMEA and enabled UBX output on UART1
	if (!ubx_setBaudRate(handle, 115200)) {
		fprintf(stderr, "Unable to command baud rate change");
		perror("openConnection");
		return -1;
	}

	fsync(handle);
	usleep(5E4);

	// Assuming that we're now configured for 115200 baud, adjust serial settings to match
	struct termios options;
	tcgetattr(handle, &options);
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	// Set options using TCSADRAIN in case commands not yet set
	if(tcsetattr(handle, TCSADRAIN, &options)) {
		fprintf(stderr, "tcsetattr() failed!\n");
	}

	// The GPS module will stop listening for 1 second if the wrong baud rate is used
	// i.e. It was already in high rate mode
	sleep(1);

	{
		struct termios check;
		tcgetattr(handle, &check);
		if (cfgetispeed(&check) != baud_to_flag(115200)) {
			fprintf(stderr, "Unable to set target baud. Wanted %d, got %d\n", 115200, flag_to_baud(cfgetispeed(&check)));
			return -1;
		}
	}

	/* Send the command again, to make sure the protocol settings get applied
	 *
	 * If we were already in high rate mode the initial rate change command
	 * would fail silently, and the GPS might be transmitting at the right
	 * baud but with the wrong protocols
	 */
	if (!ubx_setBaudRate(handle, 115200)) {
		fprintf(stderr, "Unable to command baud rate change");
		perror("openConnection");
		return -1;
	}
	usleep(5E4);

	return handle;
}

/*!
 * Currently just closes the handle, but could deconfigure and reset the module
 * or put it into a power saving mode in future.
 *
 * @param[in] handle File descriptor from ubx_openConnection()
 */

void ubx_closeConnection(int handle) {
	close(handle);
}


/*!
 * For single threaded development and testing, uses static variables rather
 * than requiring state to be tracked by caller.
 *
 * See ubx_readMessage_buf() for full description.
 * 
 * @param[in] handle File descriptor from ubx_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @return True if out now contains a valid message, false otherwise.
 */
bool ubx_readMessage(int handle, ubx_message *out) {
	static uint8_t buf[UBX_SERIAL_BUFF];
	static int index = 0; // Current read position
	static int hw = 0; // Current array end
	return ubx_readMessage_buf(handle, out, buf, &index, &hw);
}


/*!
 * Pulls data from `handle` and stores it in `buf`, tracking the current search
 * position in `index` and the current fill level/buffer high water mark in
 * `hw`.
 *
 * The source handle can be anything supported by read(), but would usually be
 * a file or a serial port.
 *
 * If a valid message is found then it is written to the structure provided as
 * a parameter and the function returns true.
 *
 * If a message cannot be read, the function returns false and the `sync1`
 * field is set to an error value:
 * - 0xFF means no message found yet, and more data is required
 * - 0xFD is a synonym for 0xFF, but indicates that zero bytes were read from source.
 *   This could indicate EOF if reading from file, but can be ignored when streaming from a device.
 * - 0xAA means that an error occurred reading in data
 * - 0XEE means a valid message header was found, but no valid message
 *
 * @param[in] handle File descriptor from ubx_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @param[in,out] buf Serial data buffer
 * @param[in,out] index Current search position within `buf`
 * @param[in,out] hw End of current valid data in `buf`
 * @return True if out now contains a valid message, false otherwise.
 */
bool ubx_readMessage_buf(int handle, ubx_message *out, uint8_t buf[UBX_SERIAL_BUFF], int *index, int *hw) {
	int ti = 0;
	if ((*hw) < UBX_SERIAL_BUFF - 1) {
		errno = 0;
		ti = read(handle, &(buf[(*hw)]), UBX_SERIAL_BUFF - (*hw));
		if (ti >= 0) {
			(*hw) += ti;
		} else {
			if (errno != EAGAIN) {
				fprintf(stderr, "Unexpected error while reading from serial port (handle ID: 0x%02x)\n", handle);
				fprintf(stderr, "read returned \"%s\" in readMessage\n", strerror(errno));
				out->sync1 = 0xAA;
				return false;
			}
		}
	}

	// Check buf[index] is valid ID
	while (!(buf[(*index)] == 0xB5) && (*index) < (*hw)) {
		(*index)++; // Current byte cannot be start of a message, so advance
	}
	if ((*index) == (*hw)) {
		if ((*hw) > 0 && (*index) > 0) {
			// Move data from index back to zero position
			memmove(buf, &(buf[(*index)]), UBX_SERIAL_BUFF - (*index));
			(*hw) -= (*index);
			(*index) = 0;
		}
		//fprintf(stderr, "Buffer empty - returning\n");
		out->sync1 = 0xFF;
		if (ti == 0) {
			out->sync1 = 0xFD;
		}
		return false;
	}

	if (((*hw) - (*index)) < 8) {
		// Not enough data for any valid message, come back later
		out->sync1 = 0xFF;
		return false;
	}

	// Set length of data required for message

	out->sync1 = buf[(*index)];
	out->sync2 = buf[(*index)+1];
	out->msgClass = buf[(*index) + 2];
	out->msgID = buf[(*index) + 3];
	out->length = buf[(*index) + 4] + (buf[(*index) + 5] << 8);

	if (out->sync2 != 0x62) {
		// Found first sync byte, but second not valid
		// Advance the index so we skip this message and go back around
		(*index)++;
		out->sync1 = 0xFF;
		return false;
	}

	if (((*hw) - (*index)) < (out->length + 8) ) {
		// Not enough data for this message yet, so mark output invalid
		out->sync1 = 0xFF;
		if (ti == 0) {
			out->sync1 = 0xFD;
		}
		// Go back around, but leave index where it is so we will pick up
		// from the same point in the buffer
		return false;
	}

	if (out->length <= 256) {
		for (uint8_t i = 0; i < out->length; i++) {
			out->data[i] = buf[(*index) + 6 + i];
		}
	} else {
		out->extdata = calloc(out->length, 1);
		for (uint16_t i = 0; i < out->length; i++) {
			out->extdata[i] = buf[(*index) + 6 + i];
		}
	}

	out->csumA = buf[(*index) + 6 + out->length];
	out->csumB = buf[(*index) + 7 + out->length];


	bool valid = ubx_check_checksum(out);
	if (valid) {
		(*index) += 8 + out->length ;
	} else {
		out->sync1 = 0xEE; // Use 0xEE as "Found, but invalid", leaving 0xFF as "No message"
		(*index)++;
	}
	if ((*hw) > 0) {
		// Move data from index back to zero position
		memmove(buf, &(buf[(*index)]), UBX_SERIAL_BUFF - (*index));
		(*hw) -= (*index);
		(*index) = 0;
	}
	return valid;
}

/*!
 * Messages are read with ubx_readMessage() and discarded until either a
 * message matches the supplied message class and ID values or the maximum
 * delay time is reached.
 *
 * Note that the function exits as soon as the time specified by `maxDelay` is
 * **exceeded**. The maximum delay specified in this function is 50us,
 * but no guarantees are given regarding time spent reading in messages.
 *
 * @param[in] handle File descriptor from ubx_openConnection()
 * @param[in] msgClass Message class to wait for
 * @param[in] msgID Message ID/Type to wait for
 * @param[in] maxDelay Timeout in seconds.
 * @param[out] out Pointer to message structure to fill with data
 * @return True if required message found, false otherwise (timeout reached)
 */
bool ubx_waitForMessage(const int handle, const uint8_t msgClass, const uint8_t msgID, const int maxDelay, ubx_message *out) {
	const time_t deadline = time(NULL) + maxDelay;
	while (time(NULL) < deadline) {
		bool rms = ubx_readMessage(handle, out);
		if (rms) {
			if ((out->msgClass == msgClass) && (out->msgID == msgID)) {
				return true;
			}
		} else {
			usleep(50);
		}
	}
	return false;
}

/*!
 * Takes a ubx_message, validates the checksum and sync bytes and writes data
 * to the device or file connected to `handle`.
 *
 * @param[in] handle File descriptor from ubx_openConnection()
 * @param[in] out Pointer to message structure to be sent.
 * @return True if data successfullt written to `handle`
 */
bool ubx_writeMessage(int handle, const ubx_message *out) {
	/*if (!validType(out->type)) {
		fprintf(stderr, "Invalid type in send\n");
		return false;
	}*/

	if (!ubx_check_checksum(out) || (out->sync1 != 0xB5) || (out->sync2 != 0x62)) {
		fprintf(stderr, "Attempted to send invalid message\n");
		return false;
	}

	uint8_t head[6] = {out->sync1, out->sync2, out->msgClass, out->msgID, (uint8_t) (out->length & 0xFF), (uint8_t) (out->length >>8)};
	uint8_t tail[2] = {out->csumA, out->csumB};
	ssize_t ret = write(handle, head, 6);
	if (ret != 6) {
		perror("writeMessage:head");
		return false;
	}

	/*
	 * The results of a zero length write are undefined for anything that
	 * isn't a regular file. Zero length messages are valid, so guard this
	 * whole section.
	 */
	if (out->length >0) {
		if (out->length <= 256) {
			ret = write(handle, out->data, out->length);
		} else {
			ret = write(handle, out->extdata, out->length);
		}
		if (ret != out->length) {
			perror("writeMessage:data");
			return false;
		}
	}

	ret = write(handle, tail, 2);
	if (ret != 2) {
		perror("writeMessage:tail");
		return false;
	}
	return true;
}
