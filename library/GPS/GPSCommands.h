#ifndef SELKIELoggerGPS_Commands
#define SELKIELoggerGPS_Commands

//! @file GPSCommands.h Functions representing u-blox GPS module commands

#include <stdbool.h>
#include <stdint.h>

#include "GPSTypes.h"

//! Send UBX port configuration to switch baud rate
bool ubx_setBaudRate(const int handle, const uint32_t baud);

//! Send UBX rate command to enable/disable message types
bool ubx_setMessageRate(const int handle, const uint8_t msgClass, const uint8_t msgID, const uint8_t rate);

//! Request specific message by class and ID
bool ubx_pollMessage(const int handle, const uint8_t msgClass, const uint8_t msgID);

//! Enable Galileo constellation use
bool ubx_enableGalileo(const int handle);

//! Set UBX navigation calculation and reporting rate
bool ubx_setNavigationRate(const int handle, const uint16_t interval, const uint16_t outputRate);

//! Enable log/information messages from GPS device
bool ubx_enableLogMessages(const int handle);

//! Disable log/information messages from GPS device
bool ubx_disableLogMessages(const int handle);

//! Set I2C address
bool ubx_setI2CAddress(const int handle, const uint8_t addr);
#endif

