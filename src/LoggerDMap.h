#ifndef SL_LOGGER_DMAP_H
#define SL_LOGGER_DMAP_H

#include "Logger.h"

//! Return device_callbacks structure for specified data source
device_callbacks dmap_getCallbacks(const char * source);

//! Get data source specific configuration handler
dc_parser dmap_getParser(const char * source);

#endif
