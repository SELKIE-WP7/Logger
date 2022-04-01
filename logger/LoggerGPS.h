#ifndef SL_LOGGER_GPS_H
#define SL_LOGGER_GPS_H

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerGPS.h"

//! @file

/*!
 * @addtogroup loggerGPS Logger: GPS Support
 * @ingroup logger
 *
 * Adds support for reading messages from a u-blox GPS receiver.
 *
 * These functions are device agnostic, but it uses the functions from
 * SELKIELoggerGPS.h to communicate with the module and these are currently
 * u-blox specific.
 *
 * @{
 */

//! GPS Device specific parameters
typedef struct {
	char *portName;    //!< Target port name
	char *sourceName;  //!< User defined name for this GPS
	uint8_t sourceNum; //!< Source ID for messages
	int initialBaud;   //!< Baud rate for initial configuration
	int targetBaud;    //!< Baud rate for operations (currently unused)
	int handle;        //!< Handle for currently opened device
	bool dumpAll;      //!< Dump all GPS messages to output queue
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

//! Parse configuration section
bool gps_parseConfig(log_thread_args_t *lta, config_section *s);
//! @}
#endif
