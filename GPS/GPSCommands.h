#ifndef SELKIELoggerGPSCommands
#define SELKIELoggerGPSCommands

#include <stdbool.h>
#include <stdint.h>

#include "GPSTypes.h"

//! Send UBX port configuration to switch baud rate
bool setBaudRate(const int handle, const uint32_t baud);

//! Send UBX rate command to enable/disable message types
bool setMessageRate(const int handle, const uint8_t msgClass, const uint8_t msgID, const uint8_t rate);
#endif

