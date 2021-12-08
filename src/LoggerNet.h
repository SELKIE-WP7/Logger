#ifndef SL_LOGGER_NET_H
#define SL_LOGGER_NET_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

/*!
 * @addtogroup loggerNet Logger: Generic network device support
 * @ingroup logger
 *
 * Adds support for reading arbitrary data from network devices that output
 * messages not otherwise covered or interpreted by this software.
 *
 * @{
 */
//! Serial device specific parameters
typedef struct {
	char *sourceName; //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	char *addr; //!< Target name
	int port; //!< Target port number
	int handle; //!< Handle for currently opened device
	int minBytes; //!< Minimum number of bytes to group into a message
	int maxBytes; //!< Maximum number of bytes to group into a message
	int timeout; //!< Reconnect if no data received after this interval [s]
} net_params;

//! Device thread setup
void *net_setup(void *ptargs);

//! Network  source main logging loop
void *net_logging(void *ptargs);

//! Network source shutdown
void *net_shutdown(void *ptargs);

//! Channel map
void *net_channels(void *ptargs);

//! Network connection helper function
bool net_connect(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks net_getCallbacks();

//! Fill out default MP source parameters
net_params net_getParams();

//! Take a configuration section and parse parameters
bool net_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
