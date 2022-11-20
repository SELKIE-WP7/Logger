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
