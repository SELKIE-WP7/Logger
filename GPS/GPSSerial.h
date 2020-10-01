#ifndef SELKIELoggerGPSSerial
#define SELKIELoggerGPSSerial

#include "GPSTypes.h"

#define UBX_SERIAL_BUFF 4096

int ubx_openConnection(const char *port, const int initialBaud);
void ubx_closeConnection(int handle);
bool ubx_readMessage(int handle, ubx_message *out);
bool ubx_readMessage_buf(int handle, ubx_message *out, uint8_t buf[UBX_SERIAL_BUFF], int *index, int *hw);
bool ubx_waitForMessage(const int handle, const uint8_t msgClass, const uint8_t msgID, const int maxDelay, ubx_message *out);
bool ubx_writeMessage(int handle, const ubx_message *out);
int baud_to_flag(const int rate);
int flag_to_baudg(const int flag);

#endif
