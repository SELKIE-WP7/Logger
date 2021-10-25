#ifndef SL_LOGGER_MP_H
#define SL_LOGGER_MP_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

/*!
 * @addtogroup loggerMP Logger: Native device support
 * @ingroup logger
 *
 * Adds support for reading messages from devices that directly output message
 * pack format messages.
 *
 * Reading messages from the device and validating the messages is handled by
 * the functions documented in SELKIELoggerMP.h
 * @{
 */
//! MP Source device specific parameters
typedef struct {
	char *portName; //!< Target port name
	int baudRate; //!< Baud rate for operations (currently unused)
	int handle; //!< Handle for currently opened device
} mp_params;

//! MP connection setup
void *mp_setup(void *ptargs);

//! MP source main logging loop
void *mp_logging(void *ptargs);

//! MP source shutdown
void *mp_shutdown(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks mp_getCallbacks();

//! Fill out default MP source parameters
mp_params mp_getParams();

//! Take a configuration section and parse parameters
bool mp_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
