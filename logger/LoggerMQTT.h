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

#ifndef SL_LOGGER_MQTT_H
#define SL_LOGGER_MQTT_H

#include <stdbool.h>
#include <stdint.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMQTT.h"

//! @file

//! MQTT source specific parameters
typedef struct {
	char *sourceName;        //!< User defined name for this source
	uint8_t sourceNum;       //!< Source ID for messages
	char *addr;              //!< Target host
	int port;                //!< Target port number
	bool victron_keepalives; //!< Victron compatible keep alives
	int keepalive_interval;  //!< Interval between keepalives
	char *sysid;             //!< Portal/System ID for use with victron_keepalive
	mqtt_conn *conn;         //!< Connection
	mqtt_queue_map qm;       //!< Topic mapping
} mqtt_params;

//! Device thread setup
void *mqtt_setup(void *ptargs);

//! Network  source main logging loop
void *mqtt_logging(void *ptargs);

//! Network source shutdown
void *mqtt_shutdown(void *ptargs);

//! Channel map
void *mqtt_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks mqtt_getCallbacks(void);

//! Fill out default MP source parameters
mqtt_params mqtt_getParams(void);

//! Take a configuration section and parse parameters
bool mqtt_parseConfig(log_thread_args_t *lta, config_section *s);
#endif
