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
int openConnection(const char *port) {
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

	// Assume operating in low speed after a reset, so connect at 9600 and command rate change
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
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

	if (!setBaudRate(handle, 115200)) {
		fprintf(stderr, "Unable to command baud rate change");
		perror("openConnection");
		return -1;
	}

	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);
	if(tcsetattr(handle, TCSANOW, &options)) {
		fprintf(stderr, "tcsetattr() failed!\n");
	}

	// The GPS module will stop listening for 1 second if the wrong baud rate is used
	// i.e. It was already in high rate mode
	sleep(1);

	return handle;
}

//! Close existing connection
void closeConnection(int handle) {
	close(handle);
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
bool readMessage(int handle, ubx_message *out) {
	static uint8_t buf[UBX_SERIAL_BUFF];
	static int index = 0; // Current read position
	static int hw = 0; // Current array end

	int ti = 0;
	if (hw < UBX_SERIAL_BUFF - 1) {
		errno = 0;
		ti = read(handle, &(buf[hw]), UBX_SERIAL_BUFF - hw);
		if (ti >= 0) {
			hw += ti;
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
	while (!(buf[index] == 0xB5) && index < hw) {
		index++; // Current byte cannot be start of a message, so advance
	}
	if (index == hw) {
		if (hw > 0 && index > 0) {
			// Move data from index back to zero position
			memmove(buf, &(buf[index]), UBX_SERIAL_BUFF - index);
			hw -= index;
			index = 0;
		}
		//fprintf(stderr, "Buffer empty - returning\n");
		out->sync1 = 0xFF;
		if (ti == 0) {
			out->sync1 = 0xFD;
		}
		return false;
	}

	if ((hw - index) < 8) {
		// Not enough data for any valid message, come back later
		out->sync1 = 0xFF;
		return false;
	}

	// Set length of data required for message

	out->sync1 = buf[index];
	out->sync2 = buf[index+1];
	out->msgClass = buf[index + 2];
	out->msgID = buf[index + 3];
	out->length = buf[index + 4] + (buf[index + 5] << 8);

	if (out->sync2 != 0x62) {
		// Found first sync byte, but second not valid
		// Advance the index so we skip this message and go back around
		index++;
		out->sync1 = 0xFF;
		return false;
	}

	if ((hw - index) < (out->length + 8) ) {
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
			out->data[i] = buf[index + 6 + i];
		}
	} else {
		out->extdata = calloc(out->length, 1);
		for (uint16_t i = 0; i < out->length; i++) {
			out->extdata[i] = buf[index + 6 + i];
		}
	}

	out->csumA = buf[index + 6 + out->length];
	out->csumB = buf[index + 7 + out->length];


	bool valid = ubx_check_checksum(out);
	if (valid) {
		index += 8 + out->length ;
	} else {
		out->sync1 = 0xEE; // Use 0xEE as "Found, but invalid", leaving 0xFF as "No message"
		index++;
	}
	if (hw > 0) {
		// Move data from index back to zero position
		memmove(buf, &(buf[index]), UBX_SERIAL_BUFF - index);
		hw -= index;
		index = 0;
	}
	return valid;
}

bool waitForMessage(const int handle, const uint8_t msgClass, const uint8_t msgID, const int maxDelay, ubx_message *out) {
	const time_t deadline = time(NULL) + maxDelay;
	while (time(NULL) < deadline) {
		bool rms = readMessage(handle, out);
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

bool writeMessage(int handle, const ubx_message *out) {
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
	return true;
}
