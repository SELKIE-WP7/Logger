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

#ifndef SL_LOGGER_NMEA_H
#define SL_LOGGER_NMEA_H

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerNMEA.h"

//! @file

/*!
 * @addtogroup loggerNMEA Logger: NMEA Support
 * @ingroup logger
 *
 * Adds support for reading messages from an NMEA0183 serial device.
 *
 * Reading messages from the device and parsing to the internal message pack
 * format used by the logger is handled in SELKIELoggerNMEA.h
 * @{
 */

//! NMEA Device specific parameters
typedef struct {
	char *portName;    //!< Target port name
	char *sourceName;  //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	int baudRate;      //!< Baud rate for operations
	int handle;        //!< Handle for currently opened device
	                   // Future expansion: Talker/Message -> Source/Message map?
} nmea_params;

//! NMEA Setup
void *nmea_setup(void *ptargs);

//! NMEA logging (with pthread function signature)
void *nmea_logging(void *ptargs);

//! NMEA Shutdown
void *nmea_shutdown(void *ptargs);

//! NMEA Channel map
void *nmea_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks nmea_getCallbacks(void);

//! Fill out default NMEA parameters
nmea_params nmea_getParams(void);

//! Take a configuration section and parse parameters
bool nmea_parseConfig(log_thread_args_t *lta, config_section *s);
//! @}
#endif
