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

#include "SELKIELoggerI2C.h"

/*!
 * @file
 * @brief Test LEDS on a Pimoronoi Automation Hat
 * @ingroup Executables
 */

/*!
 * Test LEDS on a Pimoronoi Automation Hat
 *
 * @returns -1 on error, otherwise 0
 */
int main(void) {
	int handle = i2c_openConnection("/dev/i2c-1");

	if (handle < 0) {
		fprintf(stderr, "Unable to open I2C bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	i2c_sn3218_state s = {0};

	s.global_enable = 1;
	for (int j = 0; j < 18; ++j) {
		for (int i = 0; i < 18; ++i) {
			s.led[i] = 0;
		}
		s.led[j] = 30;
		if (!i2c_sn3218_update(handle, &s)) {
			fprintf(stderr, "Unable to update state (LED %02d)\n", j + 1);
			return -1;
		}
		sleep(1);
	}

	s.global_enable = 0;
	i2c_sn3218_update(handle, &s);
	i2c_closeConnection(handle);
	return 0;
}
