#ifndef SL_LOGGER_SERIAL_H
#define SL_LOGGER_SERIAL_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

/*!
 * @addtogroup loggerSerial Logger: Generic serial device support
 * @ingroup logger
 *
 * Adds support for reading arbitrary serial data from devices that output
 * messages not otherwise covered or interpreted by this software.
 *
 * @{
 */
//! Serial device specific parameters
typedef struct {
	char *sourceName;  //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	char *portName;    //!< Target port name
	int baudRate;      //!< Baud rate for operations (currently unused)
	int handle;        //!< Handle for currently opened device
	int minBytes;      //!< Minimum number of bytes to group into a message
	int maxBytes;      //!< Maximum number of bytes to group into a message
	int pollFreq;      //!< Minimum number of times per second to check for data
} rx_params;

//! Generic serial connection setup
void *rx_setup(void *ptargs);

//! Serial source main logging loop
void *rx_logging(void *ptargs);

//! Serial source shutdown
void *rx_shutdown(void *ptargs);

//! Serial source channel map
void *rx_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks rx_getCallbacks();

//! Fill out default MP source parameters
rx_params rx_getParams();

//! Take a configuration section and parse parameters
bool rx_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
