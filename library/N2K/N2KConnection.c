#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "N2KConnection.h"

#include "SELKIELoggerBase.h"

int n2k_openConnection(const char *device, const int baud) {
	if (device == NULL) { return -1; }
	return openSerialConnection(device, baud);
}

void n2k_closeConnection(int handle) {
	errno = 0;
	close(handle);
}
