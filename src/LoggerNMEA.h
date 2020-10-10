#ifndef SL_LOGGER_NMEA_H
#define SL_LOGGER_NMEA_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerNMEA.h"

//! NMEA Device specific parameters
typedef struct {
	char *portName; //!< Target port name
	uint8_t sourceNum; //!< Source ID for messages
	int baudRate; //!< Baud rate for operations
	int handle; //!< Handle for currently opened device
	// Future expansion: Talker/Message -> Source/Message map?
} nmea_params;

//! NMEA Setup
void *nmea_setup(void *ptargs);

//! NMEA logging (with pthread function signature)
void *nmea_logging(void *ptargs);

//! NMEA Shutdown
void *nmea_shutdown(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks nmea_getCallbacks();

//! Fill out default NMEA parameters
nmea_params nmea_getParams();
#endif
