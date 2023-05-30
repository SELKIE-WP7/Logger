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

#ifndef SL_LOGGER_GPS_H
#define SL_LOGGER_GPS_H

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerGPS.h"

//! @file

/*!
 * @addtogroup loggerGPS Logger: GPS Support
 * @ingroup logger
 *
 * Adds support for reading messages from a u-blox GPS receiver.
 *
 * These functions are device agnostic, but it uses the functions from
 * SELKIELoggerGPS.h to communicate with the module and these are currently
 * u-blox specific.
 *
 * @{
 */

//! GPS Device specific parameters
typedef struct {
	char *portName;    //!< Target port name
	char *sourceName;  //!< User defined name for this GPS
	uint8_t sourceNum; //!< Source ID for messages
	int initialBaud;   //!< Baud rate for initial configuration
	int targetBaud;    //!< Baud rate for operations (currently unused)
	int handle;        //!< Handle for currently opened device
	bool dumpAll;      //!< Dump all GPS messages to output queue
} gps_params;

//! GPS Setup
void *gps_setup(void *ptargs);

//! GPS logging (with pthread function signature)
void *gps_logging(void *ptargs);

//! GPS Shutdown
void *gps_shutdown(void *ptargs);

//! GPS Channel map
void *gps_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks gps_getCallbacks(void);

//! Fill out default GPS parameters
gps_params gps_getParams(void);

//! Parse configuration section
bool gps_parseConfig(log_thread_args_t *lta, config_section *s);
//! @}
#endif
