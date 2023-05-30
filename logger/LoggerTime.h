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

#ifndef SL_LOGGER_TIME_H
#define SL_LOGGER_TIME_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

//! @file

/*!
 * @addtogroup loggerTime Logger: Timer functions
 * @ingroup logger
 *
 * Generate timestamps at as close to a regular frequency as possible, which can be used
 * to synchronise the output.
 *
 * @{
 */

//! Timer specific parameters
typedef struct {
	uint8_t sourceNum; //!< Source ID for messages
	char *sourceName;  //!< Name to report for this timer
	int frequency;     //!< Aim to sample this many times per second
} timer_params;

//! Check parameters, but no other setup required
void *timer_setup(void *ptargs);

//! Generate timer message at requested frequency
void *timer_logging(void *ptargs);

//! No shutdown required - does nothing.
void *timer_shutdown(void *ptargs);

//! Generate channel map and push to logging queue
void *timer_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks timer_getCallbacks(void);

//! Fill out default timer parameters
timer_params timer_getParams(void);

//! Take a configuration section and parse parameters
bool timer_parseConfig(log_thread_args_t *lta, config_section *s);
//! @}
#endif
