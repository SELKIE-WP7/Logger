#ifndef SL_LOGGER_GPS_H
#define SL_LOGGER_GPS_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerGPS.h"

int gps_setup(program_state *pstate, const char* gpsPortName, const int initialBaud);
void *gps_logging(void *ptargs);

#endif
