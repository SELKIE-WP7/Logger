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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "serial.h"

/*!
 * Opens port and sets the following modes:
 * - Read-write
 * - Do not make us controlling terminal (ignore control codes in data!)
 * - Data synchronised mode
 * - Ignore flow control lines
 *
 * The baud rate provided is compared to the mode requested and an error
 * returned if this does not match.
 *
 * @param[in] port Serial port device path
 * @param[in] baudRate Target baud rate (will be passed to baud_to_flag()
 * @return -1 on error, file descriptor number on success
 */
int openSerialConnection(const char *port, const int baudRate) {
	errno = 0;
	int handle = open(port, O_RDWR | O_NOCTTY | O_DSYNC | O_NDELAY);
	if (handle < 0) {
		fprintf(stderr, "Unexpected error while reading from serial port (Device: %s)\n", port);
		fprintf(stderr, "open() returned \"%s\"\n", strerror(errno));
		return -1;
	}

	fcntl(handle, F_SETFL, FNDELAY); // Make read() return 0 if no data

	struct termios options;
	// Get options, adjust baud and enable local control (cf. NoCTTY) and push to
	// interface
	tcgetattr(handle, &options);

	speed_t rate = baud_to_flag(baudRate);
	if (rate == (speed_t)-1) {
		fprintf(stderr, "Unsupported baud rate requested: %d\n", baudRate);
		return -1;
	}

	cfsetispeed(&options, rate);
	cfsetospeed(&options, rate);
	options.c_oflag &= ~OPOST; // Disable any post processing
	options.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag |= CS8;
	options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	options.c_cc[VTIME] = 1;
	options.c_cc[VMIN] = 0;
	if (tcsetattr(handle, TCSANOW, &options)) { fprintf(stderr, "tcsetattr() failed!\n"); }
	tcdrain(handle);
	{
		struct termios check;
		tcgetattr(handle, &check);
		if (cfgetispeed(&check) != rate) {
			fprintf(stderr, "Unable to set target baud. Wanted %d, got %d\n", baudRate,
			        flag_to_baud(cfgetispeed(&check)));
			/*close(handle); // Don't leave file descriptor dangling
			return -1; */
		}
	}

	return handle;
}

/*!
 * Defaults to returning -1 if a rate is not known/available at compile time.
 *
 * Earlier versions returned -rate.
 *
 * @param[in] rate Baud rate as integer value
 * @return Corresponding flag from termios.h
 */
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
#ifdef B2500000
		case 2500000:
			return B2500000;
#endif
#ifdef B3000000
		case 3000000:
			return B3000000;
#endif
#ifdef B3500000
		case 3500000:
			return B3500000;
#endif
#ifdef B4000000
		case 4000000:
			return B4000000;
#endif
		default:
			return -1;
	}
}

/*!
 * As with the baud_to_flag() counterpart, will return -1 flag if no matching
 * baud rate is found.
 *
 * @param[in] flag Corresponding flag from termios.h
 * @return Baud rate as integer value
 */
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
#ifdef B2500000
		case B2500000:
			return 2500000;
#endif
#ifdef B3000000
		case B3000000:
			return 3000000;
#endif
#ifdef B3500000
		case B3500000:
			return 3500000;
#endif
#ifdef B4000000
		case B4000000:
			return 4000000;
#endif
		default:
			return -1;
	}
}
