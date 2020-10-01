#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "messages.h"

msg_t * msg_new_float(const uint8_t source, const uint8_t type, const float val) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_FLOAT;
	newmsg->length = 1;
	newmsg->data.value = val;
	return newmsg;
}

msg_t * msg_new_timestamp(const uint8_t source, const uint8_t type, const uint32_t ts) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_TIMESTAMP;
	newmsg->length = 1;
	newmsg->data.timestamp = ts;
	return newmsg;
}

msg_t * msg_new_string(const uint8_t source, const uint8_t type, const size_t len, const char *str) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_STRING;
	//! The length value for a string message is redundant, and duplicates
	//! the value in the embedded string itself.
	newmsg->length = len;
	if (!str_update(&(newmsg->data.string), len, str)) {
		free(newmsg);
		return NULL;
	}
	return newmsg;
}

msg_t * msg_new_string_array(const uint8_t source, const uint8_t type, const strarray *array) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_STRARRAY;
	//! The length value duplicates the number of entries in the array.
	//! The length of the individual strings has to be extracted from the
	//! information in the array.
	newmsg->length = array->entries;
	if (!sa_copy(newmsg->data.names, array)) {
		free(newmsg);
		return NULL;
	}
	return newmsg;
}

msg_t * msg_new_bytes(const uint8_t source, const uint8_t type, const size_t len, const uint8_t *bytes) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_BYTES;
	/*!
	 * The length of a bytes message is the only way to reliably determine how much information is embedded in it.
	 */
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
 * Destroy a message, regardless of data type.
 *
 * Note that this function does not free the message itself, only the data
 * embedded within it.
 */
void msg_destroy(msg_t* msg) {
	switch (msg->dtype) {
		case MSG_FLOAT:
		case MSG_TIMESTAMP:
		case MSG_UNDEF:
			// No action required;
			break;
		case MSG_STRING:
			str_destroy(&(msg->data.string));
			break;
		case MSG_STRARRAY:
			sa_destroy(msg->data.names);
			break;
		case MSG_BYTES:
			free(msg->data.bytes);
			break;
		default:
			fprintf(stderr, "Unhandled message type in msg_destroy!\n");
			break;
	}
}
