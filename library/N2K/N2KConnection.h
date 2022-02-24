#ifndef SELKIELoggerN2K_Connection
#define SELKIELoggerN2K_Connection

#include <stdbool.h>

#include "N2KTypes.h"
#include "SELKIELoggerBase.h"

int n2k_openConnection(const char *device, const int baud);
void n2k_closeConnection(int handle);
#endif
