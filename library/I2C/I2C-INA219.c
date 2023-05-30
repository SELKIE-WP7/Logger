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

#include <math.h>
#include <stdint.h>
#include <unistd.h>

#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "I2C-INA219.h"
#include "I2CConnection.h"

/*!
 * Connect to specified device address and set configuration to defaults set at compile
 * time.
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an INA219 sensor
 * @return True on success, false otherwise
 */
bool i2c_ina219_configure(const int busHandle, const int devAddr) {
	if (ioctl(busHandle, I2C_SLAVE, devAddr) < 0) { return false; }

	if (i2c_smbus_write_word_data(busHandle, INA219_REG_CONFIG, INA219_CONFIG_DEF | INA219_CONFIG_RESET) < 0) {
		return false;
	}

	if (i2c_smbus_write_word_data(busHandle, INA219_REG_CALIBRATION, i2c_swapbytes(4096)) < 0) { return false; }
	usleep(500);
	return true;
}

/*!
 * Connect to specified device address and read configuration
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an INA219 sensor
 * @return configuration word
 */
uint16_t i2c_ina219_read_configuration(const int busHandle, const int devAddr) {
	if (ioctl(busHandle, I2C_SLAVE, devAddr) < 0) { return false; }

	int32_t res = i2c_smbus_read_word_data(busHandle, INA219_REG_CONFIG);
	if (res < 0) { return -1; }
	return ((uint16_t)i2c_swapbytes(res));
}

/*!
 * Connects to specified device address, reads the contents of the shunt
 * voltage register and converts the value to a floating point number.
 *
 * Out of range values are pinned to zero.
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an INA219 sensor
 * @return Shunt voltage in millivolts, or NAN on error
 */
float i2c_ina219_read_shuntVoltage(const int busHandle, const int devAddr, const void *opts) {
	if (ioctl(busHandle, I2C_SLAVE, devAddr) < 0) { return NAN; }
	if (!i2c_ina219_configure(busHandle, devAddr)) { return NAN; }

	int32_t res = i2c_smbus_read_word_data(busHandle, INA219_REG_SHUNT);
	if (res < 0) { return NAN; }

	uint16_t sres = i2c_swapbytes(res);
	float shuntV = 0;
	if ((sres & 0x8000)) {
		uint16_t t = (sres & 0x7FFF) + 1;
		shuntV = ~t * 1E-2;
	} else {
		shuntV = (sres & 0x7FFF) * 1E-2;
	}

	if (shuntV > 320.0 || shuntV < -320.0) { return NAN; }
	if (opts) {
		i2c_ina219_options *o = (i2c_ina219_options *)opts;
		float t = shuntV * o->scale + o->offset;
		if ((t < o->min) || (t > o->max)) { return NAN; }
		return t;
	}
	return shuntV; // If no options supplied, return unscaled value
}

/*!
 * Connects to specified device address, reads the contents of the bus
 * voltage register and converts the value to a floating point number.
 *
 * Bus voltage is measured at the V- terminal.
 *
 * If the overflow or invalid data flags are set, will return NAN
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an INA219 sensor
 * @return Bus voltage in volts, or NAN in case of error
 */
float i2c_ina219_read_busVoltage(const int busHandle, const int devAddr, const void *opts) {
	if (ioctl(busHandle, I2C_SLAVE, devAddr) < 0) { return NAN; }
	if (!i2c_ina219_configure(busHandle, devAddr)) { return NAN; }

	int32_t res = i2c_smbus_read_word_data(busHandle, INA219_REG_BUS);
	if (res < 0) { return NAN; }

	uint16_t sres = i2c_swapbytes(res);
	uint8_t flags = (sres & 0x03);
	if ((flags & 0x01) || !(flags & 0x02)) { return NAN; }

	float busV = (sres >> 3) * 4E-3;
	if (opts) {
		i2c_ina219_options *o = (i2c_ina219_options *)opts;
		float t = busV * o->scale + o->offset;
		if ((t < o->min) || (t > o->max)) { return NAN; }
		return t;
	}
	return busV;
}

/*!
 * Connects to specified device address, reads the contents of the power
 * register and converts the value to a floating point number.
 *
 * This is calculated internally from successive (but not concurrent) voltage
 * and current measurements, so may be inaccurate for very high frequency
 * variations.
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an INA219 sensor
 * @return Power consumption in watts, or NAN in case of error
 */
float i2c_ina219_read_power(const int busHandle, const int devAddr, const void *opts) {
	if (ioctl(busHandle, I2C_SLAVE, devAddr) < 0) { return NAN; }
	if (!i2c_ina219_configure(busHandle, devAddr)) { return NAN; }

	int32_t res = i2c_smbus_read_word_data(busHandle, INA219_REG_POWER);
	if (res < 0) { return NAN; }
	uint16_t sres = i2c_swapbytes(res);
	float power = sres * 2E-3;
	if (opts) {
		i2c_ina219_options *o = (i2c_ina219_options *)opts;
		float t = power * o->scale + o->offset;
		if ((t < o->min) || (t > o->max)) { return NAN; }
		return t;
	}
	return power;
}

/*!
 * Connects to specified device address, reads the contents of the current
 * register and converts the value to a floating point number.
 *
 * @param[in] busHandle Handle from i2c_openConnection()
 * @param[in] devAddr I2C Address for an INA219 sensor
 * @return Measured current in amperes, or NAN in case of error
 */
float i2c_ina219_read_current(const int busHandle, const int devAddr, const void *opts) {
	if (ioctl(busHandle, I2C_SLAVE, devAddr) < 0) { return NAN; }
	if (!i2c_ina219_configure(busHandle, devAddr)) { return NAN; }

	int32_t res = i2c_smbus_read_word_data(busHandle, INA219_REG_CURRENT);
	if (res < 0) { return NAN; }
	uint16_t sres = i2c_swapbytes(res);
	float current = 0;
	if ((sres & 0x8000)) {
		uint16_t t = ~(sres & 0x7FFF) + 1;
		current = t * 1E-4;
	} else {
		current = (sres & 0x7FFF) * 1E-4;
	}
	if (opts) {
		i2c_ina219_options *o = (i2c_ina219_options *)opts;
		float t = current * o->scale + o->offset;
		if ((t < o->min) || (t > o->max)) { return NAN; }
		return t;
	}
	return current;
}
