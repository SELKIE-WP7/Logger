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

#ifndef SL_LOGGER_I2C_H
#define SL_LOGGER_I2C_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerI2C.h"
#include "SELKIELoggerMP.h"

//! @file

/*!
 * @addtogroup loggerI2C Logger: I2C Support
 * @ingroup logger
 *
 * Adds support for reading messages from an I2C bus directly connected to the logging
 * machine.
 *
 * General support functions are detailed in SELKIELoggerI2C.h. As the I2C bus
 * must be polled rather than streaming data to the logger, each sensor and
 * value that needs to be logged must be registered into the channel map in
 * i2c_params before i2c_logging is called.
 *
 * @{
 */

//! Map device functions to message IDs
typedef struct {
	uint8_t messageID;    //!< Message ID to report
	string message_name;  //!< Message name to report
	uint8_t deviceAddr;   //!< I2C Device address
	i2c_dev_read_fn func; //!< Pointer to device read function
	void *ext;            //!< If not NULL, pointer to additional device data
} i2c_msg_map;

//! I2C Source device specific parameters
typedef struct {
	char *busName;        //!< Target port name
	uint8_t sourceNum;    //!< Source ID for messages
	char *sourceName;     //!< Reported source name
	int handle;           //!< Handle for currently opened device
	int frequency;        //!< Aim to sample this many times per second
	int en_count;         //!< Number of messages in chanmap
	i2c_msg_map *chanmap; //!< Map of device functions to poll
} i2c_params;

//! I2C Connection setup
void *i2c_setup(void *ptargs);

//! I2C main logging loop
void *i2c_logging(void *ptargs);

//! I2C shutdown
void *i2c_shutdown(void *ptargs);

//! Generate I2C channel map
void *i2c_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks i2c_getCallbacks(void);

//! Fill out default I2C parameters
i2c_params i2c_getParams(void);

//! Check channel mapping is valid
bool i2c_validate_chanmap(i2c_params *ip);

//! Add INA219 voltage and current readings to channel map
bool i2c_chanmap_add_ina219(i2c_params *ip, const uint8_t devAddr, const uint8_t baseID);

//! Add ADS1015 single ended voltage measurements to channel map
bool i2c_chanmap_add_ads1015(i2c_params *ip, const uint8_t devAddr, const uint8_t baseID,
                             const float scale, const float offset, const float minV,
                             const float maxV);

//! Take a configuration section, parse parameters and register devices
bool i2c_parseConfig(log_thread_args_t *lta, config_section *s);
//!@}
#endif
