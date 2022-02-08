#ifndef SELKIELoggerMP_Serial
#define SELKIELoggerMP_Serial

/*!
 * @file MPSerial.h Serial Input/Output functions for compatible MessagePack data sources
 * @ingroup SELKIELoggerMP
 */

#include "SELKIELoggerBase.h"

#include <msgpack.h>

/*!
 * @addtogroup SELKIELoggerMP
 * @{
 */

#include "MPTypes.h"
#define MP_SERIAL_BUFF 4096

//! Set up a connection to the specified port
int mp_openConnection(const char *port, const int baudRate);

//! Close existing connection
void mp_closeConnection(int handle);

//! Static wrapper around mp_readMessage_buf
bool mp_readMessage(int handle, msg_t *out);

//! Read data from handle, and parse message if able
bool mp_readMessage_buf(int handle, msg_t *out, uint8_t buf[MP_SERIAL_BUFF], int *index, int *hw);

//! Pack a message into a buffer
bool mp_packMessage(msgpack_sbuffer *sbuf, const msg_t *out);

//! Send message to attached device
bool mp_writeMessage(int handle, const msg_t *out);

//! Send message data (only!) to attached device
bool mp_writeData(int handle, const msg_t *out);

//! Pack string array
void mp_pack_strarray(msgpack_packer *pack, const strarray *sa);

//! Pack numeric/floating point array
void mp_pack_numarray(msgpack_packer *pack, const size_t entries, const float *fa);

//! Unpack msgpack array into string array
bool mp_unpack_strarray(strarray *sa, msgpack_object_array *obj);

//! Allocate array of floats and unpack a msgpack array into it
size_t mp_unpack_numarray(float **sa, msgpack_object_array *obj);
//! @}
#endif
