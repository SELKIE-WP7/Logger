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

#ifndef SL_LOGGER_MP_H
#define SL_LOGGER_MP_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

//! @file

/*!
 * @addtogroup loggerMP Logger: Native device support
 * @ingroup logger
 *
 * Adds support for reading messages from devices that directly output message
 * pack format messages.
 *
 * Reading messages from the device and validating the messages is handled by
 * the functions documented in SELKIELoggerMP.h
 * @{
 */
//! MP Source device specific parameters
typedef struct {
	char *portName;  //!< Target port name
	int baudRate;    //!< Baud rate for operations (currently unused)
	int handle;      //!< Handle for currently opened device
	uint8_t csource; //!< Cache source ID
	char *cname;     //!< Cache latest device name
	strarray cmap;   //!< Cache latest channel map
} mp_params;

//! MP connection setup
void *mp_setup(void *ptargs);

//! MP source main logging loop
void *mp_logging(void *ptargs);

//! Push device information from cache to queue
void *mp_channels(void *ptargs);

//! MP source shutdown
void *mp_shutdown(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks mp_getCallbacks(void);

//! Fill out default MP source parameters
mp_params mp_getParams(void);

//! Take a configuration section and parse parameters
bool mp_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
