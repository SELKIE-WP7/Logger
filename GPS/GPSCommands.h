#ifndef SELKIELoggerGPSCommands
#define SELKIELoggerGPSCommands

#include <stdbool.h>
#include <stdint.h>

#include "GPSTypes.h"

//! Send UBX port configuration to switch baud rate
bool setBaudRate(const int handle, const uint32_t baud);

#endif

