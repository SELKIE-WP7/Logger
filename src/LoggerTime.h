#ifndef SL_LOGGER_TIME_H
#define SL_LOGGER_TIME_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

//! Timer specific parameters
typedef struct {
	uint8_t sourceNum; //!< Source ID for messages
	int frequency; //!< Aim to sample this many times per second
} timer_params;

void *timer_setup(void *ptargs);
void *timer_logging(void *ptargs);
void *timer_shutdown(void *ptargs);
void *timer_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks timer_getCallbacks();

//! Fill out default timer parameters
timer_params timer_getParams();
#endif
