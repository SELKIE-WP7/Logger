#ifndef SL_LOGGER_MQTT_H
#define SL_LOGGER_MQTT_H

#include <stdint.h>
#include <stdbool.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMQTT.h"

//! MQTT source specific parameters
typedef struct {
	char *sourceName; //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	char *addr; //!< Target host
	int port; //!< Target port number
	bool victron_keepalives; //!< Victron compatible keep alives
	char *sysid; //!< Portal/System ID for use with victron_keepalive
	mqtt_conn *conn; //!< Connection
	mqtt_queue_map qm; //!< Topic mapping
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
device_callbacks mqtt_getCallbacks();

//! Fill out default MP source parameters
mqtt_params mqtt_getParams();

//! Take a configuration section and parse parameters
bool mqtt_parseConfig(log_thread_args_t *lta, config_section *s);
#endif
