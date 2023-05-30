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

#ifndef SL_LOGGER_NET_H
#define SL_LOGGER_NET_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

//! @file

/*!
 * @addtogroup loggerNet Logger: Generic network device support
 * @ingroup logger
 *
 * Adds support for reading arbitrary data from network devices that output
 * messages not otherwise covered or interpreted by this software.
 *
 * @{
 */
//! Network device specific parameters
typedef struct {
	char *sourceName;  //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	char *addr;        //!< Target name
	int port;          //!< Target port number
	int handle;        //!< Handle for currently opened device
	int minBytes;      //!< Minimum number of bytes to group into a message
	int maxBytes;      //!< Maximum number of bytes to group into a message
	int timeout;       //!< Reconnect if no data received after this interval [s]
} net_params;

//! Device thread setup
void *net_setup(void *ptargs);

//! Network  source main logging loop
void *net_logging(void *ptargs);

//! Network source shutdown
void *net_shutdown(void *ptargs);

//! Channel map
void *net_channels(void *ptargs);

//! Network connection helper function
bool net_connect(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks net_getCallbacks(void);

//! Fill out default MP source parameters
net_params net_getParams(void);

//! Take a configuration section and parse parameters
bool net_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
