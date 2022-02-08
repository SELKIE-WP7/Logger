#ifndef SL_LOGGER_TIME_H
#define SL_LOGGER_TIME_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"
/*!
 * @addtogroup loggerTime Logger: Timer functions
 * @ingroup logger
 *
 * Generate timestamps at as close to a regular frequency as possible, which can be used
 * to synchronise the output.
 *
 * @{
 */

//! Timer specific parameters
typedef struct {
	uint8_t sourceNum; //!< Source ID for messages
	char *sourceName;
	int frequency; //!< Aim to sample this many times per second
} timer_params;

//! Check parameters, but no other setup required
void *timer_setup(void *ptargs);

//! Generate timer message at requested frequency
void *timer_logging(void *ptargs);

//! No shutdown required - does nothing.
void *timer_shutdown(void *ptargs);

//! Generate channel map and push to logging queue
void *timer_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks timer_getCallbacks();

//! Fill out default timer parameters
timer_params timer_getParams();

//! Take a configuration section and parse parameters
bool timer_parseConfig(log_thread_args_t *lta, config_section *s);
//! @}
#endif
