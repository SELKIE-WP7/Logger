#include <stdbool.h>
#include <stdio.h>

#include "GPSTypes.h"
#include "GPSMessages.h"
#include "GPSSerial.h"

int main(int argc, char *argv[]) {
	char *gpsPort = "/dev/ttyS0";

	if (argc == 2) {
		gpsPort = argv[1];
	}

	int gpsHandle = openConnection(gpsPort);
	if (gpsHandle < 0) {
		fprintf(stderr, "Unable to open a connection on port %s\n", gpsPort);
		perror("openConnection");
		return -1;
	}

	int count = 0;
	while (1) {
		ubx_message out;
		if (readMessage(gpsHandle, &out)) {
			count++;
			ubx_print_hex(&out);
			printf("\n");
		} else {
			if (!(out.sync1 == 0xFF || out.sync1 == 0xFD)) {
				fprintf(stderr, "Error signalled from readMessage\n");
				break;
			}
		}
	}
	closeConnection(gpsHandle);
	fprintf(stdout, "%d messages read successfully\n\n", count);
	return 0;
}
