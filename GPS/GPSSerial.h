#ifndef SELKIELoggerGPSSerial
#define SELKIELoggerGPSSerial

#include "GPSTypes.h"

#define UBX_SERIAL_BUFF 4096

int openConnection(const char *port);
void closeConnection(int handle);
bool readMessage(int handle, ubx_message *out);
bool waitForMessage(const int handle, const uint8_t msgClass, const uint8_t msgID, const int maxDelay, ubx_message *out);
bool writeMessage(int handle, const ubx_message *out);

#endif
