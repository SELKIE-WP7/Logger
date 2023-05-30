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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerGPS.h"

/*! @file UBXMessagesFromFile.c
 *
 * @brief Test reading u-Blox GPS messages from a file
 *
 * @test Read supplied u-Blox GPS messages and output summary information.
 *
 * @ingroup testing
 */

/*!
 * Read data from file until end of file is reached, outputting total number of
 * entries read.
 *
 * Exact outputs dependent on supplied test data
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns 0 (Pass), -1 (Fail), -2 (Failed to run / Error)
 */
int main(int argc, char *argv[]) {
	//LCOV_EXCL_START
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return -2;
	}

	errno = 0;
	FILE *testFile = fopen(argv[1], "r");
	if ((testFile == NULL) || errno) {
		fprintf(stderr, "Unable to open test file %s\n", argv[1]);
		perror("open");
		return -2;
	}
	//LCOV_EXCL_STOP

	int count = 0;
	int exit = 0;
	while (!(feof(testFile) || exit == 1)) {
		ubx_message tmp;
		if (ubx_readMessage(fileno(testFile), &tmp)) {
			// Successfully read message
			count++;
		} else {
			switch (tmp.sync1) {
				//LCOV_EXCL_START
				case 0xAA:
					fclose(testFile);
					return -2;
					break;
				case 0xEE:
					fclose(testFile);
					return -1;
					break;
				//LCOV_EXCL_STOP
				case 0xFD:
					exit = 1;
					break;
				case 0xFF:
				default:
					break;
			}
		}
	}
	fprintf(stdout, "%d messages read\n", count);
	fclose(testFile);
	return 0;
}
