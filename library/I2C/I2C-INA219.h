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

#ifndef SELKIELoggerI2C_INA219
#define SELKIELoggerI2C_INA219

#include <stdbool.h>

/*!
 * @file I2C-INA219.h
 * @ingroup SELKIELoggerI2C
 *
 * Sources used for INA219 interface:
 * - https://www.kernel.org/doc/html/latest/i2c/smbus-protocol.html
 * - https://www.kernel.org/doc/html/latest/i2c/dev-interface.html
 * - https://www.waveshare.com/w/upload/1/10/Ina219.pdf
 *
 */

/*!
 * @defgroup ina219reg INA219: Register addresses
 * @ingroup SELKIELoggerI2C
 * @{
 */
//! INA219 configuration register
#define INA219_REG_CONFIG      0x00

//! INA219 shunt voltage register
#define INA219_REG_SHUNT       0x01

//! INA219 bus voltage register
#define INA219_REG_BUS         0x02

//! INA219 calculated power register
#define INA219_REG_POWER       0x03

//! INA219 current register
#define INA219_REG_CURRENT     0x04

//! INA219 calibration registor
#define INA219_REG_CALIBRATION 0x05

//! @}

/*!
 * @addtogroup ina219conf INA219: Configuration values
 * @ingroup SELKIELoggerI2C
 * @{
 */

/*!
 * @brief Default sensor configuration for this library.
 *
 * Sets the following options:
 *  - 32V measurement range
 *  - 320mV Shunt voltage range
 *  - 12 bit ADC resolution
 *  - Average over 8 samples
 *  - Sample continuously
 */
#define INA219_CONFIG_DEF      0x3DDF

/*!
 * Set the device reset bit in the configuration register
 */
#define INA219_CONFIG_RESET    0x8000

typedef struct {
	float scale;  //!< Scale received value by this quantity
	float offset; //!< Add this amount to received value
	float min;    //!< If not NaN, smallest value considered valid
	float max;    //!< If not NaN, largest value considered valid
} i2c_ina219_options;

#define I2C_INA219_DEFAULTS \
	{ .scale = 1.0, .offset = 0.0, .min = -INFINITY, .max = INFINITY }
//! @}

/*!
 * @addtogroup ina219 INA219: Interface functions
 * @ingroup SELKIELoggerI2C
 * @{
 */
//! Send configuration command to the specified device
bool i2c_ina219_configure(const int busHandle, const int devAddr);

//! Read configuration from device
uint16_t i2c_ina219_read_configuration(const int busHandle, const int devAddr);

//! Get voltage across the shunt resistor in millivolts
float i2c_ina219_read_shuntVoltage(const int busHandle, const int devAddr, const void *opts);

//! Get bus voltage (at V- terminal) in volts
float i2c_ina219_read_busVoltage(const int busHandle, const int devAddr, const void *opts);

//! Get power consumption in watts
float i2c_ina219_read_power(const int busHandle, const int devAddr, const void *opts);

//! Get current flow through shunt resistor in amps
float i2c_ina219_read_current(const int busHandle, const int devAddr, const void *opts);

//! @}
#endif
