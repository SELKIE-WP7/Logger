#ifndef SL_LOGGER_NMEA_H
#define SL_LOGGER_NMEA_H

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerNMEA.h"

/*!
 * @addtogroup loggerNMEA Logger: NMEA Support
 * @ingroup logger
 *
 * Adds support for reading messages from an NMEA0183 serial device.
 *
 * Reading messages from the device and parsing to the internal message pack
 * format used by the logger is handled in SELKIELoggerNMEA.h
 * @{
 */

//! NMEA Device specific parameters
typedef struct {
	char *portName;    //!< Target port name
	char *sourceName;  //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	int baudRate;      //!< Baud rate for operations
	int handle;        //!< Handle for currently opened device
	                   // Future expansion: Talker/Message -> Source/Message map?
} nmea_params;

//! NMEA Setup
void *nmea_setup(void *ptargs);

//! NMEA logging (with pthread function signature)
void *nmea_logging(void *ptargs);

//! NMEA Shutdown
void *nmea_shutdown(void *ptargs);

//! NMEA Channel map
void *nmea_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks nmea_getCallbacks();

//! Fill out default NMEA parameters
nmea_params nmea_getParams();

//! Take a configuration section and parse parameters
bool nmea_parseConfig(log_thread_args_t *lta, config_section *s);
//! @}
#endif
