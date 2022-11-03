#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerNMEA.h"

/*! @file NMEAMessagesFromFile.c
 *
 * @brief Test reading NMEA data from a file
 *
 * @test Read supplied NMEA data and output summary information.
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
		nmea_msg_t tmp = {0};
		if (nmea_readMessage(fileno(testFile), &tmp)) {
			// Successfully read message
			count++;
			strarray *f = nmea_parse_fields(&tmp);
			if (f == NULL) { return -1; }
			if (strncmp(tmp.message, "ZDA", 3) == 0) {
				struct tm *t = nmea_parse_zda(&tmp);
				//LCOV_EXCL_START
				if (t == NULL) {
					sa_destroy(f);
					free(f);
					return -1;
				}
				//LCOV_EXCL_STOP
				fprintf(stdout, "%s", asctime(t));
				free(t);
			}
			sa_destroy(f);
			free(f);
		} else {
			switch (tmp.raw[0]) {
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
