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
 * @brief Read values from a Pimoronoi Automation Hat
 * @ingroup Executables
 */

/*!
 * Will read the ADC values from the automation hat, assuming it to be connected to
 * /dev/i2c-1
 *
 * @returns -1 on error, otherwise 0
 */
int main(void) {
	int handle = i2c_openConnection("/dev/i2c-1");

	if (handle < 0) {
		fprintf(stderr, "Unable to open I2C bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	const float cfact = 7.88842;
	i2c_ads1015_options opts = I2C_ADS1015_DEFAULTS;
	opts.pga = ADS1015_CONFIG_PGA_4096MV;

	// clang-format off
	float ch0 = i2c_ads1015_read_mux(handle, ADS1015_ADDR_DEFAULT, ADS1015_CONFIG_MUX_SINGLE_0, &opts);
	float ch1 = i2c_ads1015_read_mux(handle, ADS1015_ADDR_DEFAULT, ADS1015_CONFIG_MUX_SINGLE_1, &opts) * cfact;
	float ch2 = i2c_ads1015_read_mux(handle, ADS1015_ADDR_DEFAULT, ADS1015_CONFIG_MUX_SINGLE_2, &opts) * cfact;
	float ch3 = i2c_ads1015_read_mux(handle, ADS1015_ADDR_DEFAULT, ADS1015_CONFIG_MUX_SINGLE_3, &opts) * cfact;
	// clang-format on

	float delta_1_3 = i2c_ads1015_read_diff_ch1_ch3(handle, ADS1015_ADDR_DEFAULT, &opts);
	float delta_2_3 = i2c_ads1015_read_diff_ch2_ch3(handle, ADS1015_ADDR_DEFAULT, &opts);

	fprintf(stdout, "Single shot conversion on all mux options:\n");

	fprintf(stdout, "Channel 0:\t%+.3f mV\n", ch0);
	fprintf(stdout, "Channel 1:\t%+.3f mV\n", ch1);
	fprintf(stdout, "Channel 2:\t%+.3f mV\n", ch2);
	fprintf(stdout, "Channel 3:\t%+.3f mV\n", ch3);

	fprintf(stdout, "Ch1 - Ch3:\t%+.3f mV\n", delta_1_3);
	fprintf(stdout, "Ch2 - Ch3:\t%+.3f mV\n", delta_2_3);

	i2c_closeConnection(handle);
	return 0;
}
