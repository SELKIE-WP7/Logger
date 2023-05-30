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

#ifndef SL_LOGGER_N2K_H
#define SL_LOGGER_N2K_H

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerN2K.h"

//! @file

/*!
 * @addtogroup loggerN2K Logger: N2K Support
 * @ingroup logger
 *
 * Adds support for reading messages from an N2K0183 serial device.
 *
 * Reading messages from the device and parsing to the internal message pack
 * format used by the logger is handled in SELKIELoggerN2K.h
 * @{
 */

#define N2KCHAN_NAME   SLCHAN_NAME   //!< Source name
#define N2KCHAN_MAP    SLCHAN_MAP    //!< Channel map
#define N2KCHAN_TSTAMP SLCHAN_TSTAMP //!< Timestamps
#define N2KCHAN_RAW    SLCHAN_RAW    //!< Raw data (recorded unmodified)
#define N2KCHAN_LAT    4             //!< Latitude (from GPS)
#define N2KCHAN_LON    5             //!< Longitude (from GPS)

//! N2K Device specific parameters
typedef struct {
	char *portName;    //!< Target port name
	char *sourceName;  //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	int baudRate;      //!< Baud rate for operations
	int handle;        //!< Handle for currently opened device
} n2k_params;

//! N2K Setup
void *n2k_setup(void *ptargs);

//! N2K logging (with pthread function signature)
void *n2k_logging(void *ptargs);

//! N2K Shutdown
void *n2k_shutdown(void *ptargs);

//! N2K Channel map
void *n2k_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks n2k_getCallbacks(void);

//! Fill out default N2K parameters
n2k_params n2k_getParams(void);

//! Take a configuration section and parse parameters
bool n2k_parseConfig(log_thread_args_t *lta, config_section *s);
//! @}
#endif
