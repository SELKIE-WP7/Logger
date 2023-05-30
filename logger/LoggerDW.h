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

#ifndef SL_LOGGER_DW_H
#define SL_LOGGER_DW_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
//! @file

/*!
 * @addtogroup loggerDW Logger: Datawell WaveBuoy / Receiver support
 * @ingroup logger
 *
 * Adds support for reading data from Datawell Wavebuoys in HXV format
 *
 * @{
 */

//! Configuration is as per simple network sources
typedef struct {
	char *sourceName;   //!< User defined name for this source
	uint8_t sourceNum;  //!< Source ID for messages
	char *addr;         //!< Target name
	int handle;         //!< Handle for currently opened device
	int timeout;        //!< Reconnect if no data received after this interval [s]
	bool recordRaw;     //!< Enable retention of raw data
	bool parseSpectrum; //!< Enable parsing of spectral data
} dw_params;

/*!
 * @addtogroup loggerDWChannels Logger: Datawell WaveBuoy channel numbering
 * @ingroup loggerDW
 * @{
 */
#define DWCHAN_NAME   SLCHAN_NAME   //!< Source name
#define DWCHAN_MAP    SLCHAN_MAP    //!< Channel map
#define DWCHAN_TSTAMP SLCHAN_TSTAMP //!< Timestamps
#define DWCHAN_RAW    SLCHAN_RAW    //!< Raw data (recorded unmodified)
#define DWCHAN_SIG    4             //!< Reported signal strength
#define DWCHAN_DN     5             //!< Displacement (North)
#define DWCHAN_DW     6             //!< Displacement (West)
#define DWCHAN_DV     7             //!< Displacement (Vertical)

#define DWCHAN_LAT    8  //!< Latitude (Decimal degrees)
#define DWCHAN_LON    9  //!< Longitude (Decimal degrees)
#define DWCHAN_ORIENT 10 //!< Device orientation
#define DWCHAN_INCLIN 11 //!< Device inclination
#define DWCHAN_GPSFIX 12 //!< GPS quality
#define DWCHAN_HRMS   13 //!< RMS Wave Height
#define DWCHAN_TREF   14 //!< Reference Temperature
#define DWCHAN_TWTR   15 //!< Water Temperature
#define DWCHAN_WEEKS  16 //!< Estimated weeks of battery remaining

#define DWCHAN_SPF    17 //!< Spectral data: Frequency
#define DWCHAN_SPD    18 //!< Spectral data: Direction
#define DWCHAN_SPS    19 //!< Spectral data: Spread
#define DWCHAN_SPM    20 //!< Spectral data: M2 coefficient
#define DWCHAN_SPN    21 //!< Spectral data: N2 coefficient
#define DWCHAN_SPR    22 //!< Spectral data: Relative PSD
#define DWCHAN_SPK    23 //!< Spectral data: K factor

//! @}

//! Datawell thread setup
void *dw_setup(void *ptargs);

//! Datawell source main logging loop
void *dw_logging(void *ptargs);

//! Datawell source shutdown
void *dw_shutdown(void *ptargs);

//! DW Network connection helper
bool dw_net_connect(void *ptargs);

//! Channel map
void *dw_channels(void *ptargs);

//! Create and push messages to queue
void dw_push_message(log_thread_args_t *args, uint8_t sNum, uint8_t cNum, float data);

//! Fill out device callback functions for logging
device_callbacks dw_getCallbacks(void);

//! Fill out default MP source parameters
dw_params dw_getParams(void);

//! Take a configuration section and parse parameters
bool dw_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
