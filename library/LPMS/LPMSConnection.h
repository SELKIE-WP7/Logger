#ifndef SELKIELoggerLPMS_Connection
#define SELKIELoggerLPMS_Connection

#include <stdbool.h>

#include "LPMSTypes.h"
#include "SELKIELoggerBase.h"

/*!
 * @file LPMSConnection.h LPMS connection handling
 * @ingroup SELKIELoggerLPMS
 */

//! Default serial buffer allocation size
#define LPMS_BUFF 1024

//! Open connection to an LPMS serial device
int lpms_openConnection(const char *device, const int baud);

//! Close LPMS serial connection
void lpms_closeConnection(int handle);

//! Static wrapper around lpms_readMessage_buf
bool lpms_readMessage(int handle, lpms_message *out);

//! Read data from handle, and parse message if able
bool lpms_readMessage_buf(int handle, lpms_message *out, uint8_t buf[LPMS_BUFF], size_t *index, size_t *hw);

#endif
