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

#ifndef SL_LOGGER_LPMS_H
#define SL_LOGGER_LPMS_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

#include "SELKIELoggerLPMS.h"

//! @file

/*!
 * @addtogroup loggerLPMS Logger: LPMS device support
 * @ingroup logger
 *
 * Adds support for LPMS IMU devices.
 *
 * @{
 */
//! Serial device specific parameters
typedef struct {
	char *sourceName;  //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	char *portName;    //!< Target port name
	int unitID;        //!< LPMS Sensor Address
	int baudRate;      //!< Baud rate for operations (Default 921600)
	int handle;        //!< Handle for currently opened device
	int pollFreq;      //!< Desired number of measurements per second
} lpms_params;

//! Generic serial connection setup
void *lpms_setup(void *ptargs);

//! Helper function: Create and queue data messages, with error handling
bool lpms_queue_message(msgqueue *Q, const uint8_t src, const uint8_t chan, const float val);

//! Serial source main logging loop
void *lpms_logging(void *ptargs);

//! Serial source shutdown
void *lpms_shutdown(void *ptargs);

//! Serial source channel map
void *lpms_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks lpms_getCallbacks(void);

//! Fill out default MP source parameters
lpms_params lpms_getParams(void);

//! Take a configuration section and parse parameters
bool lpms_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
