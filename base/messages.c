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

msg_t * str_msg_string(const uint8_t source, const uint8_t type, const size_t len, const char *str) {
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

msg_t * msg_new_string_array(const uint8_t source, const uint8_t type, const strarray *array) {
	msg_t *newmsg = calloc(1, sizeof(msg_t));
	newmsg->source = source;
	newmsg->type = type;
	newmsg->dtype = MSG_STRARRAY;
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
			// TODO
			break;
		default:
			fprintf(stderr, "Unhandled message type in msg_destroy!\n");
			break;
	}
	free(msg);
}
