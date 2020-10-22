#ifndef SL_LOGGER_GPS_H
#define SL_LOGGER_GPS_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerGPS.h"

//! GPS Device specific parameters
typedef struct {
	char *portName; //!< Target port name
	uint8_t sourceNum; //!< Source ID for messages
	int initialBaud; //!< Baud rate for initial configuration
	int targetBaud; //!< Baud rate for operations (currently unused)
	int handle; //!< Handle for currently opened device
	bool dumpAll; //!< Dump all GPS messages to output queue
} gps_params;

//! GPS Setup
void *gps_setup(void *ptargs);

//! GPS logging (with pthread function signature)
void *gps_logging(void *ptargs);

//! GPS Shutdown
void *gps_shutdown(void *ptargs);

//! GPS Channel map
void *gps_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks gps_getCallbacks();

//! Fill out default GPS parameters
gps_params gps_getParams();
#endif
