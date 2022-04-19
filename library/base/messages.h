#ifndef SELKIELoggerBase_Messages
#define SELKIELoggerBase_Messages
#include "strarray.h"
#include <stdint.h>

/*!
 * @file messages.h Message data types and handling functions
 * @ingroup SELKIELoggerBase
 */

/*!
 * @addtogroup MPMessages Internal message format and functions
 * @ingroup SELKIELoggerBase
 * @{
 */

/*!
 * Messages are queueable items that can be serialised to file if required
 *
 * In order to allow a range of data to be processed, the message data can be
 * any of the types contained in this union.
 *
 * Creation, destruction and handling of these messages needs to account for this.
 */
typedef union {
	float value;        //!< Generic numerical data
	uint32_t timestamp; //!< Intended to represent millisecond level clock
	uint8_t *bytes;     //!< Our "raw" binary type
	string string;      //!< Single character array with length
	strarray names;     //!< Array of strings, intended for use to provide channel names
	float *farray;      //!< Array of floats
} msg_data_t;

//! Each data type should map to an entry in the msg_data_t union
typedef enum {
	MSG_ERROR = -1, //!< An error code is returned in data.value
	MSG_UNDEF = 0,  //!< Undefined/Uninitialised message
	MSG_FLOAT,      //!< Generic numerical data
	MSG_TIMESTAMP,  //!< Timestamp (milliseconds since defined epoch/event)
	MSG_BYTES,      //!< Raw binary data
	MSG_STRING,     //!< Single string @sa string
	MSG_STRARRAY,   //!< Array of strings. @sa strarray
	MSG_NUMARRAY,   //!< Array of floating point values
} msg_dtype_t;

//! Queuable message

/*!
 * Designed to be flexible mapping between multiple sources and data types.
 */
typedef struct {
	uint8_t source;    //!< Maps to a specific sensor unit or data source
	uint8_t type;      //!< Message type. Common types to be documented
	size_t length;     //!< Data type dependent, see the msg_new functions.
	msg_dtype_t dtype; //!< Embedded data type
	msg_data_t data;   //!< Embedded data
} msg_t;

//! Create new message with a single numeric value
msg_t *msg_new_float(const uint8_t source, const uint8_t type, const float val);

//! Create a timestamp message
msg_t *msg_new_timestamp(const uint8_t source, const uint8_t type, const uint32_t ts);

//! Create a new message with a single string embedded
msg_t *msg_new_string(const uint8_t source, const uint8_t type, const size_t len, const char *str);

//! Create a new message containing an array of strings
msg_t *msg_new_string_array(const uint8_t source, const uint8_t type, const strarray *array);

//! Create a new message containing raw binary data
msg_t *msg_new_bytes(const uint8_t source, const uint8_t type, const size_t len, const uint8_t *bytes);

//! Create a new message containing an array of floating point data
msg_t *msg_new_float_array(const uint8_t source, const uint8_t type, const size_t entries, const float *array);

//! Generate string representation of message
char *msg_to_string(const msg_t *msg);

//! Generate string representation of message data
char *msg_data_to_string(const msg_t *msg);

//! Convert numerical array to string
char *msg_data_narr_to_string(const msg_t *msg);

//! Destroy a message
void msg_destroy(msg_t *msg);
//! @}
#endif
