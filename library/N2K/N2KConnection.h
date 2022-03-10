#ifndef SELKIELoggerN2K_Connection
#define SELKIELoggerN2K_Connection

#include <stdbool.h>

#include "N2KTypes.h"
#include "SELKIELoggerBase.h"

#define N2K_BUFF 1024

int n2k_openConnection(const char *device, const int baud);
void n2k_closeConnection(int handle);

//! Static wrapper around n2k_readMessage_buf
bool n2k_act_readMessage(int handle, n2k_act_message *out);

//! Read data from handle, and parse message if able
bool n2k_act_readMessage_buf(int handle, n2k_act_message *out, uint8_t buf[N2K_BUFF], size_t *index, size_t *hw);

#endif
