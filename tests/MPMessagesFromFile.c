#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerMP.h"

/*! @file MPMessagesFromFile.c
 *
 * @brief Test reading MessagePacked data from a file
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
 * @ingroup testing
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
	uint8_t sources[128] = {0};
	uint8_t types[128] = {0};

	while (!(feof(testFile) || exit == 1)) {
		msg_t tmp = {0};
		if (mp_readMessage(fileno(testFile), &tmp)) {
			// Successfully read message
			count++;
			sources[tmp.source]++;
			types[tmp.type]++;
			msg_destroy(&tmp);
		} else {
			switch ((uint8_t)tmp.data.value) {
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
	for (int i = 0; i < 128; i++) {
		if (sources[i] > 0) {
			fprintf(stdout, "%d messages read from source 0x%02x\n", sources[i], i);
		}
	}
	for (int i = 0; i < 128; i++) {
		if (types[i] > 0) {
			fprintf(stdout, "%d messages read were of type 0x%02x\n", types[i], i);
		}
	}
	fclose(testFile);
	return 0;
}
