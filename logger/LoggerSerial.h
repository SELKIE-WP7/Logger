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

#ifndef SL_LOGGER_SERIAL_H
#define SL_LOGGER_SERIAL_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

//! @file

/*!
 * @addtogroup loggerSerial Logger: Generic serial device support
 * @ingroup logger
 *
 * Adds support for reading arbitrary serial data from devices that output
 * messages not otherwise covered or interpreted by this software.
 *
 * @{
 */
//! Serial device specific parameters
typedef struct {
	char *sourceName;  //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	char *portName;    //!< Target port name
	int baudRate;      //!< Baud rate for operations (currently unused)
	int handle;        //!< Handle for currently opened device
	int minBytes;      //!< Minimum number of bytes to group into a message
	int maxBytes;      //!< Maximum number of bytes to group into a message
	int pollFreq;      //!< Minimum number of times per second to check for data
} rx_params;

//! Generic serial connection setup
void *rx_setup(void *ptargs);

//! Serial source main logging loop
void *rx_logging(void *ptargs);

//! Serial source shutdown
void *rx_shutdown(void *ptargs);

//! Serial source channel map
void *rx_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks rx_getCallbacks(void);

//! Fill out default MP source parameters
rx_params rx_getParams(void);

//! Take a configuration section and parse parameters
bool rx_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
