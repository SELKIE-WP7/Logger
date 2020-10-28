#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "SELKIELoggerNMEA.h"

/*! @file NMEAMessagesFromFile.c
 *
 * @brief Test reading from a file
 *
 * Read data from file until end of file is reached, outputting total number of
 * entries read.
 *
 * Return codes:
 * - 0 for tests passed
 * - -1 for tests failed
 * - -2 for a failure to run the test (system error, unexpected condition etc.)
 *
 * Exact outputs dependent on supplied test data
 *
 * @addtogroup testing Test programs for use with CTest
 */
int main(int argc, char *argv[]) {
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

	int count = 0;
	int exit = 0;
	while (!(feof(testFile) || exit == 1)) {
		nmea_msg_t tmp = {0};
		if (nmea_readMessage(fileno(testFile), &tmp)) {
			// Successfully read message
			count++;
		} else {
			switch (tmp.raw[0]) {
				case 0xAA:
					fclose(testFile);
					return -2;
					break;
				case 0xEE:
					fclose(testFile);
					return -1;
					break;
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
