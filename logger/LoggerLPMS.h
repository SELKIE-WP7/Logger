#ifndef SL_LOGGER_LPMS_H
#define SL_LOGGER_LPMS_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

#include "SELKIELoggerLPMS.h"

//! @file

/*!
 * @addtogroup loggerLPMS Logger: LPMS device support
 * @ingroup logger
 *
 * Adds support for LPMS IMU devices.
 *
 * @{
 */
//! Serial device specific parameters
typedef struct {
	char *sourceName;  //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	char *portName;    //!< Target port name
	int unitID;        //!< LPMS Sensor Address
	int baudRate;      //!< Baud rate for operations (Default 921600)
	int handle;        //!< Handle for currently opened device
	int pollFreq;      //!< Desired number of measurements per second
} lpms_params;

//! Generic serial connection setup
void *lpms_setup(void *ptargs);

//! Helper function: Create and queue data messages, with error handling
bool lpms_queue_message(msgqueue *Q, const uint8_t src, const uint8_t chan, const float val);

//! Serial source main logging loop
void *lpms_logging(void *ptargs);

//! Serial source shutdown
void *lpms_shutdown(void *ptargs);

//! Serial source channel map
void *lpms_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks lpms_getCallbacks();

//! Fill out default MP source parameters
lpms_params lpms_getParams();

//! Take a configuration section and parse parameters
bool lpms_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
