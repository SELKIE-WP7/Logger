#ifndef SL_LOGGER_DW_H
#define SL_LOGGER_DW_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

/*!
 * @addtogroup loggerDW Logger: Datawell WaveBuoy / Receiver support
 * @ingroup logger
 *
 * Adds support for reading data from Datawell Wavebuoys in HVX format
 *
 * @{
 */

//! Configuration is as per simple network sources
typedef struct {
	char *sourceName;  //!< User defined name for this source
	uint8_t sourceNum; //!< Source ID for messages
	char *addr;        //!< Target name
	int handle;        //!< Handle for currently opened device
	int timeout;       //!< Reconnect if no data received after this interval [s]
	bool recordRaw;
	bool parseSpectrum;
} dw_params;

#define DWCHAN_NAME   SLCHAN_NAME
#define DWCHAN_MAP    SLCHAN_MAP
#define DWCHAN_TSTAMP SLCHAN_TSTAMP
#define DWCHAN_RAW    SLCHAN_RAW
#define DWCHAN_SIG    4
#define DWCHAN_DN     5
#define DWCHAN_DW     6
#define DWCHAN_DV     7

#define DWCHAN_LAT    8
#define DWCHAN_LON    9
#define DWCHAN_ORIENT 10
#define DWCHAN_INCLIN 11
#define DWCHAN_GPSFIX 12
#define DWCHAN_HRMS   13
#define DWCHAN_TREF   14
#define DWCHAN_TWTR   15
#define DWCHAN_WEEKS  16

#define DWCHAN_SPF    17
#define DWCHAN_SPD    18
#define DWCHAN_SPS    19
#define DWCHAN_SPM    20
#define DWCHAN_SPN    21
#define DWCHAN_SPR    22
#define DWCHAN_SPK    23

//! Datawell thread setup
void *dw_setup(void *ptargs);

//! Datawell source main logging loop
void *dw_logging(void *ptargs);

//! Datawell source shutdown
void *dw_shutdown(void *ptargs);

//! DW Network connection helper
bool dw_net_connect(void *ptargs);

//! Channel map
void *dw_channels(void *ptargs);

//! Create and push messages to queue
void dw_push_message(log_thread_args_t *args, uint8_t sNum, uint8_t cNum, float data);

//! Fill out device callback functions for logging
device_callbacks dw_getCallbacks();

//! Fill out default MP source parameters
dw_params dw_getParams();

//! Take a configuration section and parse parameters
bool dw_parseConfig(log_thread_args_t *lta, config_section *s);

//! @}
#endif
