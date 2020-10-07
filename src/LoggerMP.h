#ifndef SL_LOGGER_MP_H
#define SL_LOGGER_MP_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

int mp_setup(program_state *pstate, const char* mpPortName, const int baudRate);
void *mp_logging(void *ptargs);

#endif
