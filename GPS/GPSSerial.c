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

//! Set up a connection to the specified port
int ubx_openConnection(const char *port, const int initialBaud) {
	/* ! Opens port and sets the following modes:
	 * - read-write
	 * - Do not make us controlling terminal (ignore control codes!)
	 * - Data synchronised mode
	 * - Ignore flow control lines
	 */
	errno = 0;
	int handle = open(port, O_RDWR | O_NOCTTY | O_DSYNC | O_NDELAY);
	if (handle < 0) {
		fprintf(stderr, "Unexpected error while reading from serial port (Device: %s)\n", port);
		fprintf(stderr, "open() returned \"%s\"\n", strerror(errno));
		return -1;
	}

	fcntl(handle, F_SETFL, FNDELAY); // Make read() return 0 if no data

	struct termios options;
	// Get options, adjust baud and enable local control (cf. NoCTTY) and push to interface
	tcgetattr(handle, &options);

	// Assume operating in low speed after a reset, so connect at specified initial baud and command rate change
	cfsetispeed(&options, baud_to_flag(initialBaud));
	cfsetospeed(&options, baud_to_flag(initialBaud));
	options.c_oflag &= ~OPOST; // Disable any post processin
	options.c_cflag &= ~(PARENB|CSTOPB|CSIZE);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag |= CS8;
	options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	options.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	options.c_cc[VTIME] = 1;
	options.c_cc[VMIN] = 0;
	if(tcsetattr(handle, TCSANOW, &options)) {
		fprintf(stderr, "tcsetattr() failed!\n");
	}

	{
		struct termios check;
		tcgetattr(handle, &check);
		if (cfgetispeed(&check) != baud_to_flag(initialBaud)) {
			fprintf(stderr, "Unable to set target baud. Wanted %d, got %d\n", cfgetispeed(&check), baud_to_flag(initialBaud));
			return -1;
		}
	}

	if (!ubx_setBaudRate(handle, 115200)) {
		fprintf(stderr, "Unable to command baud rate change");
		perror("openConnection");
		return -1;
	}

	fsync(handle);
	usleep(5E4);

	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	// Set options using TCSADRAIN in case commands not yet set
	if(tcsetattr(handle, TCSADRAIN, &options)) {
		fprintf(stderr, "tcsetattr() failed!\n");
	}

	// The GPS module will stop listening for 1 second if the wrong baud rate is used
	// i.e. It was already in high rate mode
	sleep(1);

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
	usleep(5E-4);

	return handle;
}

//! Close existing connection
void ubx_closeConnection(int handle) {
	close(handle);
}

//! Static wrapper around ubx_readMessage_buf

/*!
 * For single threaded development and testing, uses static variables rather
 * than requiring state to be tracked by caller.
 */
bool ubx_readMessage(int handle, ubx_message *out) {
	static uint8_t buf[UBX_SERIAL_BUFF];
	static int index = 0; // Current read position
	static int hw = 0; // Current array end
	return ubx_readMessage_buf(handle, out, buf, &index, &hw);
}

//! Read data from handle, and parse message if able


/*!
 * This function maintains a static internal message buffer, filling it from
 * the file handle provided. This handle can be anything supported by read(),
 * but would usually be a file or a serial port.
 *
 * If a valid message is found then it is written to the structure provided as
 * a parameter and the function returns true.
 *
 * If a message cannot be read, the function returns false and the `sync1`
 * field is set to an error value:
 * - 0xFF means no message found yet, and more data is required
 * - 0xAA means that an error occurred reading in data
 * - 0XEE means a valid message header was found, but no valid message
 *
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

		ret = write(handle, tail, 2);
		if (ret != 2) {
			perror("writeMessage:tail");
			return false;
		}
	}
	return true;
}

int baud_to_flag(const int rate) {
	switch (rate) {
		case 0:
			return B0;
		case 1200:
			return B1200;
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		case 230400:
			return B230400;
		case 460800:
			return B460800;
		case 500000:
			return B500000;
		case 576000:
			return B576000;
		case 921600:
			return B921600;
		case 1000000:
			return B1000000;
		case 1152000:
			return B1152000;
		case 1500000:
			return B1500000;
		case 2000000:
			return B2000000;
		case 2500000:
			return B2500000;
		case 3000000:
			return B3000000;
		case 3500000:
			return B3500000;
		case 4000000:
			return B4000000;
		default:
			return -rate;
	}
}

int flag_to_baud(const int flag) {
	switch (flag) {
		case B0:
			return 0;
		case B1200:
			return 1200;
		case B2400:
			return 2400;
		case B4800:
			return 4800;
		case B9600:
			return 9600;
		case B19200:
			return 19200;
		case B38400:
			return 38400;
		case B57600:
			return 57600;
		case B115200:
			return 115200;
		case B230400:
			return 230400;
		case B460800:
			return 460800;
		case B500000:
			return 500000;
		case B576000:
			return 576000;
		case B921600:
			return 921600;
		case B1000000:
			return 1000000;
		case B1152000:
			return 1152000;
		case B1500000:
			return 1500000;
		case B2000000:
			return 2000000;
		case B2500000:
			return 2500000;
		case B3000000:
			return 3000000;
		case B3500000:
			return 3500000;
		case B4000000:
			return 4000000;
		default:
			return -flag;
	}
}
