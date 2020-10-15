#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "SELKIELoggerI2C.h"

int main(int argc, char *argv[]) {
	int handle = i2c_openConnection("/dev/i2c-1");

	if (handle < 0) {
		fprintf(stderr, "Unable to open I2C bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	for (int i2cc=0x40; i2cc < 0x44; i2cc++) {
		fprintf(stdout, "Input %02x\n", i2cc-0x40+1);
		fprintf(stdout, "\tShunt Voltage: %+.3fmV\n", i2c_ina219_read_shuntVoltage(handle, i2cc));
		fprintf(stdout, "\tBus Voltage: %+.6fV\n", i2c_ina219_read_busVoltage(handle, i2cc));
		fprintf(stdout, "\tCurrent: %+.6fA\n", i2c_ina219_read_current(handle, i2cc));
		fprintf(stdout, "\tPower: %+.6fW\n", i2c_ina219_read_power(handle, i2cc));
	}
	i2c_closeConnection(handle);
	return 0;
}
