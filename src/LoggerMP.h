#ifndef SL_LOGGER_MP_H
#define SL_LOGGER_MP_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

//! MP Source device specific parameters
typedef struct {
	char *portName; //!< Target port name
	int baudRate; //!< Baud rate for operations (currently unused)
	int handle; //!< Handle for currently opened device
} mp_params;

void *mp_setup(void *ptargs);
void *mp_logging(void *ptargs);
void *mp_shutdown(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks mp_getCallbacks();

//! Fill out default GPS parameters
mp_params mp_getParams();
#endif
