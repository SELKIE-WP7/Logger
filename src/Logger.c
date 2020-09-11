#include <stdbool.h>
#include <stdio.h>

#include "GPSTypes.h"
#include "GPSMessages.h"
#include "GPSSerial.h"
#include "version.h"

int main(int argc, char *argv[]) {
	char *gpsPort = "/dev/ttyS0";
	char *outFileName = "out.dat";

	if (argc >= 2) {
		gpsPort = argv[1];
	}
	if (argc >= 3) {
		outFileName = argv[2];
	}
	if (argc > 3) {
		fprintf(stderr, "Usage: %s [gpsPort [output file]]\n", argv[0]);
		fprintf(stderr, "Version: %s\n\n", GIT_VERSION_STRING);
		return -1;
	}

	int gpsHandle = openConnection(gpsPort);
	if (gpsHandle < 0) {
		fprintf(stderr, "Unable to open a connection on port %s\n", gpsPort);
		perror("openConnection");
		return -1;
	}

	FILE *outFile = fopen(outFileName, "w");

	if (outFile == NULL) {
		perror("FILE");
		return -1;
	}


	int count = 0;
	while (1) {
		ubx_message out;
		if (readMessage(gpsHandle, &out)) {
			count++;
			ubx_print_hex(&out);
			printf("\n");
			uint8_t *data = NULL;
			ssize_t len = ubx_flat_array(&out, &data);
			fwrite(data, len, 1, outFile);
		} else {
			if (!(out.sync1 == 0xFF || out.sync1 == 0xFD)) {
				fprintf(stderr, "Error signalled from readMessage\n");
				break;
			}
		}
	}
	closeConnection(gpsHandle);
	fclose(outFile);
	fprintf(stdout, "%d messages read successfully\n\n", count);
	return 0;
}
