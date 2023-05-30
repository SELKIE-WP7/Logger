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
		float shuntVolts = i2c_ina219_read_shuntVoltage(handle, i2cc, NULL);
		float busVolts = i2c_ina219_read_busVoltage(handle, i2cc, NULL);
		float current = i2c_ina219_read_current(handle, i2cc, NULL);
		float power = i2c_ina219_read_power(handle, i2cc, NULL);
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
