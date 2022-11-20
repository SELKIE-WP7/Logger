#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <math.h>

#include "SELKIELoggerI2C.h"

/*!
 * @file
 * @brief Read values from a Raspberry Pi power hat (or compatible sensors)
 * @ingroup Executables
 */

/*!
 * Assumes that four INA219 chips are connected to /dev/i2c-1 using addresses
 * 0x40 to 0x43, and polls for current values. Identifies some out of range
 * conditions and will report an appropriate error message.
 *
 * @returns -1 on error, otherwise 0
 */
int main(void) {
	int handle = i2c_openConnection("/dev/i2c-1");

	if (handle < 0) {
		fprintf(stderr, "Unable to open I2C bus: %s\n", strerror(errno));
		return -1;
	}

	for (int i2cc = 0x40; i2cc < 0x44; i2cc++) {
		i2c_ina219_configure(handle, i2cc);
		fprintf(stdout, "Input %02x\n", i2cc - 0x40 + 1);
		float shuntVolts = i2c_ina219_read_shuntVoltage(handle, i2cc);
		float busVolts = i2c_ina219_read_busVoltage(handle, i2cc);
		float current = i2c_ina219_read_current(handle, i2cc);
		float power = i2c_ina219_read_power(handle, i2cc);
		if (isfinite(shuntVolts) && isfinite(busVolts)) {
			fprintf(stdout, "\tShunt Voltage: %+.3f mV\n", shuntVolts);
			fprintf(stdout, "\tBus Voltage: %+.6f V\n", busVolts);
			fprintf(stdout, "\tCurrent: %+.6f A\n", current);
			fprintf(stdout, "\tPower: %+.6f W\n", power);
		} else {
			fprintf(stdout, "\tBad voltage readings - input disconnected?\n");
		}
	}
	i2c_closeConnection(handle);
	return 0;
}
