#ifndef SELKIELoggerGPS_Serial
#define SELKIELoggerGPS_Serial

//! @file GPSSerial.h Serial Input/Output functions for communication with u-blox GPS modules

#include "SELKIELoggerBase.h"
#include "GPSTypes.h"

#define UBX_SERIAL_BUFF 4096

//! Set up a connection to a UBlox module on a given port
int ubx_openConnection(const char *port, const int initialBaud);

//! Close a connection opened with ubx_openConnection()
void ubx_closeConnection(int handle);


//! Static wrapper around ubx_readMessage_buf()
bool ubx_readMessage(int handle, ubx_message *out);

//! Read data from handle, and parse message if able
bool ubx_readMessage_buf(int handle, ubx_message *out, uint8_t buf[UBX_SERIAL_BUFF], int *index, int *hw);

//! Read (and discard) messages until required message seen or timeout reached
bool ubx_waitForMessage(const int handle, const uint8_t msgClass, const uint8_t msgID, const int maxDelay, ubx_message *out);

//! Send message to attached device
bool ubx_writeMessage(int handle, const ubx_message *out);
#endif
