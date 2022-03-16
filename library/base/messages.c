#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messages.h"

/*!
 * Allocates a new msg_t, copies in the source, type and value and sets the data type to
 * MSG_FLOAT.
 *
 * @param[in] source Message source
 * @param[in] type   Message type
 * @param[in] val    General numeric value, represented as single precision float
 * @return Pointer to new message
 */
msg_t *msg_new_float(const uint8_t source, const uint8_t type, const float val) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_FLOAT;
	newmsg->length = 1;
	newmsg->data.value = val;
	return newmsg;
}

/*!
 * Allocates a new msg_t, copies in the source, type and value and sets the data type to
 * MSG_TIMESTAMP.
 *
 * @param[in] source Message source
 * @param[in] type   Message type
 * @param[in] ts     Timestamp, assumed to be milliseconds from an arbitrary epoch
 * @return Pointer to new message
 */
msg_t *msg_new_timestamp(const uint8_t source, const uint8_t type, const uint32_t ts) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_TIMESTAMP;
	newmsg->length = 1;
	newmsg->data.timestamp = ts;
	return newmsg;
}

/*!
 * Allocates a new msg_t, copies in the source, type and value and sets the data type to
 * MSG_STRING
 *
 * Intended for sending/receiving source/device names.
 *
 * String is created with str_update()
 *
 * The length value for a string message is redundant, and duplicates the value embedded
 * in the string itself.
 *
 * @param[in] source Message source
 * @param[in] type   Message type
 * @param[in] len    Maximum length of character array
 * @param[in] str    Pointer to character array
 * @return Pointer to new message, NULL on failure
 */
msg_t *msg_new_string(const uint8_t source, const uint8_t type, const size_t len, const char *str) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_STRING;
	newmsg->length = len;
	if (!str_update(&(newmsg->data.string), len, str)) {
		free(newmsg);
		return NULL;
	}
	return newmsg;
}

/*!
 * Allocates a new msg_t, copies in the source, type and values and sets the data type to
 * MSG_STRARRAY
 *
 * Intended for sending/receiving channel names
 *
 * String is copied with sa_copy()
 *
 * The length value embedded in the message is set to the number of entries in the array.
 * The length of the individual strings has to be extracted from the information in the
 * array itself.
 *
 * @param[in] source Message source
 * @param[in] type   Message type
 * @param[in] array  Pointer to existing string array
 * @return Pointer to new message, NULL on failure
 */
msg_t *msg_new_string_array(const uint8_t source, const uint8_t type, const strarray *array) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_STRARRAY;
	newmsg->length = array->entries;
	if (!sa_copy(&(newmsg->data.names), array)) {
		free(newmsg);
		return NULL;
	}
	return newmsg;
}

/*!
 * Allocates a new msg_t, copies in the source, type and values and sets the data type to
 * MSG_BYTES
 *
 * Intended for sending/receiving arbitrary binary data.
 *
 * The length stored in the message structure is the only way to reliably
 * determine how much information is embedded in it.
 *
 * @param[in] source Message source
 * @param[in] type   Message type
 * @param[in] len    Length of array to be copied
 * @param[in] bytes  Pointer to existing array of uint8_t
 * @return Pointer to new message, NULL on failure
 */

msg_t *msg_new_bytes(const uint8_t source, const uint8_t type, const size_t len, const uint8_t *bytes) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_BYTES;
	newmsg->length = len;
	newmsg->data.bytes = calloc(len, sizeof(uint8_t));
	errno = 0;
	memcpy(newmsg->data.bytes, bytes, len);
	if (errno) {
		free(newmsg->data.bytes);
		free(newmsg);
		return NULL;
	}
	return newmsg;
}

/*!
 * Allocates a new msg_t, copies in the source, type and array and sets the data type to
 * MSG_NUMARRAY
 *
 * The original array can be freed by the caller after creating the message.
 *
 * @param[in] source  Message source
 * @param[in] type    Message type
 * @param[in] entries Number of entries in array
 * @param[in] array   Pointer to array of floats
 * @return Pointer to new message
 */
msg_t *msg_new_float_array(const uint8_t source, const uint8_t type, const size_t entries, const float *array) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_NUMARRAY;
	newmsg->length = entries;
	newmsg->data.farray = calloc(entries, sizeof(float));
	errno = 0;
	memcpy(newmsg->data.farray, array, entries * sizeof(float));
	if (errno) {
		free(newmsg->data.farray);
		free(newmsg);
		return NULL;
	}
	return newmsg;
}

/*!
 * Destroy a message, regardless of data type.
 *
 * Note that this function does not free the message itself, only the data
 * embedded within it.
 *
 * @param[in] msg Message to be destroyed
 */
void msg_destroy(msg_t *msg) {
	if (msg == NULL) { return; }

	switch (msg->dtype) {
		case MSG_ERROR:
		case MSG_FLOAT:
		case MSG_TIMESTAMP:
		case MSG_UNDEF:
			// No action required;
			break;
		case MSG_STRING:
			str_destroy(&(msg->data.string));
			break;
		case MSG_STRARRAY:
			sa_destroy(&(msg->data.names));
			break;
		case MSG_BYTES:
			free(msg->data.bytes);
			break;
		case MSG_NUMARRAY:
			free(msg->data.farray);
			break;
		default:
			fprintf(stderr, "Unhandled message type in msg_destroy!\n");
			break;
	}
	msg->length = 0;
	msg->dtype = MSG_UNDEF;
}
