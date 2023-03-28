#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "I2C-ADS1015.h"
#include "I2CConnection.h"

/*!
 * Connect to specified device address and read configuration
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 sensor
 * @return configuration word
 */
uint16_t i2c_ads1015_read_configuration(const int busHandle, const int devAddr) {
	if (ioctl(busHandle, I2C_SLAVE, devAddr) < 0) { return false; }

	int32_t res = i2c_smbus_read_word_data(busHandle, ADS1015_REG_CONFIG);
	if (res < 0) { return -1; }
	return ((uint16_t)i2c_swapbytes(res));
}

/*!
 * Convert PGA setting (or extract it from a 16bit configuration word) and
 * return the value of the least significant bit in millivolts.
 *
 * @param[in] config Configuration word or ADS1015_CONFIG_PGA_ constant
 * @return LSB value in millivolts
 */
float i2c_ads1015_pga_to_scale_factor(const uint16_t config) {
	uint16_t pga = (config & ADS1015_CONFIG_PGA_SELECT);
	if (pga == ADS1015_CONFIG_PGA_6144MV) {
		return 3;
	} else if (pga == ADS1015_CONFIG_PGA_4096MV) {
		return 2;
	} else if (pga == ADS1015_CONFIG_PGA_2048MV) {
		return 1;
	} else if (pga == ADS1015_CONFIG_PGA_1024MV) {
		return 0.5;
	} else if (pga == ADS1015_CONFIG_PGA_0512MV) {
		return 0.25;
	} else if (pga == ADS1015_CONFIG_PGA_0256MV) {
		return 0.125;
	} else if (pga == ADS1015_CONFIG_PGA_0256MV2) {
		return 0.125;
	} else if (pga == ADS1015_CONFIG_PGA_0256MV3) {
		return 0.125;
	} else {
		return NAN;
	}
	return NAN;
}

/*!
 * Perform a single shot conversion on an ADS1015 device using provided mux and pga
 * values.
 *
 * mux and pga parameters will be masked before use.
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 sensor
 * @param[in] mux ADS1015_CONFIG_MUX_ constant to select measurement to be performed
 * @param[in] opts Pointer to i2c_ads1015_options structure
 * @param[in] pga ADS1015_CONFIG_PGA_ constant to determine measurement range
 * @returns MUX voltage value
 */
float i2c_ads1015_read_mux(const int busHandle, const int devAddr, const uint16_t mux,
                           const i2c_ads1015_options *opts) {
	if (ioctl(busHandle, I2C_SLAVE, devAddr) < 0) { return NAN; }

	uint16_t pga = ADS1015_CONFIG_PGA_DEFAULT;
	float min = -INFINITY;
	float max = INFINITY;
	float scale = 1.0;
	float offset = 0;

	if (opts) {
		min = opts->min;
		max = opts->max;
		scale = opts->scale;
		offset = opts->offset;
		pga = opts->pga;
	}

	while (!(i2c_ads1015_read_configuration(busHandle, devAddr) & ADS1015_CONFIG_STATE_CONVERT)) {
		usleep(100);
	}

	uint16_t confClear = (ADS1015_CONFIG_DEFAULT & ADS1015_CONFIG_MUX_CLEAR & ADS1015_CONFIG_PGA_CLEAR);
	uint16_t muxToSet = (mux & ADS1015_CONFIG_MUX_SELECT);
	uint16_t pgaToSet = (pga & ADS1015_CONFIG_PGA_SELECT);

	/*
	 * Default configuration with MUX and PGA cleared, OR'd with the MUX
	 * bits of mux and PGA bits of pga
	 *
	 * Finally, the top bit is set (using ADS1015_CONFIG_STATE_CONVERT) to
	 * trigger the measurement
	 */
	if (i2c_smbus_write_word_data(
		    busHandle, ADS1015_REG_CONFIG,
		    (uint16_t)i2c_swapbytes(confClear | muxToSet | pgaToSet | ADS1015_CONFIG_STATE_CONVERT)) < 0) {
		return NAN;
	}
	while (i2c_ads1015_read_configuration(busHandle, devAddr) & ADS1015_CONFIG_STATE_CONVERT) {
		usleep(100);
	}

	int32_t res = i2c_smbus_read_word_data(busHandle, ADS1015_REG_RESULT);
	if (res < 0) { return NAN; }

	uint16_t sres = i2c_swapbytes(res);
	sres = (sres & 0xFFF0) >> 4;

	float sv = i2c_ads1015_pga_to_scale_factor(pga);

	float adcV = 0;
	if ((sres & 0x800)) {
		uint16_t t = (sres & 0x7FF) + 1;
		adcV = ~t * sv;
	} else {
		adcV = (sres & 0x7FF) * sv;
	}

	adcV = scale * adcV + offset;
	if ((adcV < min) || (adcV > max)) { adcV = NAN; }

	return adcV;
}

/*!
 * Wrapper around i2c_ads1015_read_mux()
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 ADC
 * @param[in] opts Pointer to i2c_ads1015_options structure
 * @return ADC voltage in millivolts, or NAN on error
 */
float i2c_ads1015_read_ch0(const int busHandle, const int devAddr, const void *opts) {
	i2c_ads1015_options *o = NULL;
	if (opts) { o = (i2c_ads1015_options *)opts; }
	return i2c_ads1015_read_mux(busHandle, devAddr, ADS1015_CONFIG_MUX_SINGLE_0, o);
}

/*!
 * Wrapper around i2c_ads1015_read_mux()
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 ADC
 * @param[in] opts Pointer to i2c_ads1015_options structure
 * @return ADC voltage in millivolts, or NAN on error
 */
float i2c_ads1015_read_ch1(const int busHandle, const int devAddr, const void *opts) {
	i2c_ads1015_options *o = NULL;
	if (opts) { o = (i2c_ads1015_options *)opts; }
	return i2c_ads1015_read_mux(busHandle, devAddr, ADS1015_CONFIG_MUX_SINGLE_1, o);
}

/*!
 * Wrapper around i2c_ads1015_read_mux()
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 ADC
 * @param[in] opts Pointer to i2c_ads1015_options structure
 * @return ADC voltage in millivolts, or NAN on error
 */
float i2c_ads1015_read_ch2(const int busHandle, const int devAddr, const void *opts) {
	i2c_ads1015_options *o = NULL;
	if (opts) { o = (i2c_ads1015_options *)opts; }
	return i2c_ads1015_read_mux(busHandle, devAddr, ADS1015_CONFIG_MUX_SINGLE_2, o);
}

//! Get single-ended voltage measurement from channel 3
/*!
 * Wrapper around i2c_ads1015_read_mux()
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 ADC
 * @param[in] opts Pointer to i2c_ads1015_options structure
 * @return ADC voltage in millivolts, or NAN on error
 */
float i2c_ads1015_read_ch3(const int busHandle, const int devAddr, const void *opts) {
	i2c_ads1015_options *o = NULL;
	if (opts) { o = (i2c_ads1015_options *)opts; }
	return i2c_ads1015_read_mux(busHandle, devAddr, ADS1015_CONFIG_MUX_SINGLE_3, o);
}

/*!
 * Wrapper around i2c_ads1015_read_mux()
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 ADC
 * @param[in] opts Pointer to i2c_ads1015_options structure
 * @return ADC voltage in millivolts, or NAN on error
 */
float i2c_ads1015_read_diff_ch0_ch1(const int busHandle, const int devAddr, const void *opts) {
	i2c_ads1015_options *o = NULL;
	if (opts) { o = (i2c_ads1015_options *)opts; }
	return i2c_ads1015_read_mux(busHandle, devAddr, ADS1015_CONFIG_MUX_DIFF_0_1, o);
}

/*!
 * Wrapper around i2c_ads1015_read_mux()
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 ADC
 * @param[in] opts Pointer to i2c_ads1015_options structure
 * @return ADC voltage in millivolts, or NAN on error
 */
float i2c_ads1015_read_diff_ch0_ch3(const int busHandle, const int devAddr, const void *opts) {
	i2c_ads1015_options *o = NULL;
	if (opts) { o = (i2c_ads1015_options *)opts; }
	return i2c_ads1015_read_mux(busHandle, devAddr, ADS1015_CONFIG_MUX_DIFF_0_3, o);
}

/*!
 * Wrapper around i2c_ads1015_read_mux()
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 ADC
 * @param[in] opts Pointer to i2c_ads1015_options structure
 * @return ADC voltage in millivolts, or NAN on error
 */
float i2c_ads1015_read_diff_ch1_ch3(const int busHandle, const int devAddr, const void *opts) {
	i2c_ads1015_options *o = NULL;
	if (opts) { o = (i2c_ads1015_options *)opts; }
	return i2c_ads1015_read_mux(busHandle, devAddr, ADS1015_CONFIG_MUX_DIFF_1_3, o);
}

/*!
 * Wrapper around i2c_ads1015_read_mux()
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an ADS1015 ADC
 * @param[in] opts Pointer to i2c_ads1015_options structure
 * @return ADC voltage in millivolts, or NAN on error
 */
float i2c_ads1015_read_diff_ch2_ch3(const int busHandle, const int devAddr, const void *opts) {
	i2c_ads1015_options *o = NULL;
	if (opts) { o = (i2c_ads1015_options *)opts; }
	return i2c_ads1015_read_mux(busHandle, devAddr, ADS1015_CONFIG_MUX_DIFF_2_3, o);
}
