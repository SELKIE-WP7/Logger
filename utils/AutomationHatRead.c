#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "SELKIELoggerI2C.h"

/*!
 * @file AutomationHatRead.c
 * @brief Read values from a Pimoronoi Automation Hat
 * @ingroup Executables
 *
 * Will read the ADC values from the automation hat, assuming it to be connected to /dev/i2c-1
 *
 */
int main(int argc, char *argv[]) {
	int handle = i2c_openConnection("/dev/i2c-1");

	if (handle < 0) {
		fprintf(stderr, "Unable to open I2C bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	const float cfact = 7.88842;
	float ch0 = i2c_ads1015_read_mux(handle, ADS1015_ADDR_DEFAULT, ADS1015_CONFIG_MUX_SINGLE_0, ADS1015_CONFIG_PGA_4096MV);
	float ch1 = i2c_ads1015_read_mux(handle, ADS1015_ADDR_DEFAULT, ADS1015_CONFIG_MUX_SINGLE_1, ADS1015_CONFIG_PGA_4096MV) * cfact;
	float ch2 = i2c_ads1015_read_mux(handle, ADS1015_ADDR_DEFAULT, ADS1015_CONFIG_MUX_SINGLE_2, ADS1015_CONFIG_PGA_4096MV) * cfact;
	float ch3 = i2c_ads1015_read_mux(handle, ADS1015_ADDR_DEFAULT, ADS1015_CONFIG_MUX_SINGLE_3, ADS1015_CONFIG_PGA_4096MV) * cfact;

	float delta_1_3 = i2c_ads1015_read_diff_ch1_ch3(handle, ADS1015_ADDR_DEFAULT);
	float delta_2_3 = i2c_ads1015_read_diff_ch2_ch3(handle, ADS1015_ADDR_DEFAULT);

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
