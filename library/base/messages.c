/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

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
		// LCOV_EXCL_START
		free(newmsg);
		return NULL;
		// LCOV_EXCL_STOP
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
		// LCOV_EXCL_START
		free(newmsg);
		return NULL;
		// LCOV_EXCL_STOP
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
 * Generate string representation of message.
 *
 * Allocates a suitable character array, which must be freed by the caller.
 *
 * @param[in] msg Message to be represented
 * @returns Pointer to string, or NULL
 */
char *msg_to_string(const msg_t *msg) {
	if (msg == NULL) { return NULL; }

	char *out = NULL;
	char *dstr = msg_data_to_string(msg);
	int ar = asprintf(&out, "0x%02x:0x%02x %s", msg->source, msg->type, dstr);
	free(dstr);

	if (ar >= 0) { return out; }

	return NULL;
}

/*!
 * Generate string representation of message data
 *
 * Allocates a suitable character array, which must be freed by the caller.
 *
 * @param[in] msg Message containing data to be represented
 * @returns Pointer to string, or NULL
 */
char *msg_data_to_string(const msg_t *msg) {
	if (msg == NULL) { return NULL; }

	char *out = NULL;
	int rv = 0;
	switch (msg->dtype) {
		case MSG_FLOAT:
			rv = asprintf(&out, "%.6f", msg->data.value);
			break;
		case MSG_TIMESTAMP:
			rv = asprintf(&out, "%09u", msg->data.timestamp);
			break;
		case MSG_STRING:
			rv = asprintf(&out, "%*s", (int)msg->data.string.length, msg->data.string.data);
			break;
		case MSG_STRARRAY:
			out = msg_data_sarr_to_string(msg);
			if (out == NULL) { out = strdup("[String array - error converting to string]"); }
			break;
		case MSG_BYTES:
			rv = asprintf(&out, "[Binary data, %zd bytes]", msg->length);
			break;
		case MSG_NUMARRAY:
			out = msg_data_narr_to_string(msg);
			if (out == NULL) { out = strdup("[Numeric array - error converting to string]"); }
			break;
		// LCOV_EXCL_START
		case MSG_ERROR:
			out = strdup("[Message flagged as error]");
			break;
		case MSG_UNDEF:
			out = strdup("[Message flagged as uninitialised]");
			break;
		default:
			out = strdup("[Message type unknown]");
			break;
			// LCOV_EXCL_STOP
	}
	if (rv < 0) {
		// LCOV_EXCL_START
		if (out) { free(out); }
		return NULL;
		// LCOV_EXCL_STOP
	}
	return out;
}

/*!
 * Generate string for numeric arrays
 *
 * Each value in the array is separated by a forward slash (/), to avoid issues
 * when using in CSV outputs.
 *
 * Allocated character array must be freed by the caller.
 *
 * @param[in] msg Message containing data to be represented
 * @returns Pointer to string, or NULL
 */
char *msg_data_narr_to_string(const msg_t *msg) {
	const size_t alen = 250;
	char *out = calloc(alen, sizeof(char));
	if (out == NULL) { return NULL; }
	size_t pos = 0;
	for (unsigned int i = 0; i < msg->length; i++) {
		int l = snprintf(&(out[pos]), (alen - pos), "%.4f/", msg->data.farray[i]);
		if (l < 0) {
			// LCOV_EXCL_START
			free(out);
			return NULL;
			// LCOV_EXCL_STOP
		}
		pos += l;
	}
	out[pos - 1] = '\0';
	return out;
}

/*!
 * Generate string for string arrays
 *
 * Each value in the array is separated by a forward slash (/), to avoid issues
 * when using in CSV outputs.
 *
 * Allocated character array must be freed by the caller.
 *
 * @param[in] msg Message containing data to be represented
 * @returns Pointer to string, or NULL
 */
char *msg_data_sarr_to_string(const msg_t *msg) {
	size_t len = 0;
	if ((msg == NULL) || (msg->data.names.entries <= 0)) { return NULL; }
	for (int i = 0; i < msg->data.names.entries; i++) {
		size_t l = msg->data.names.strings[i].length;
		if ((l == 0) || (msg->data.names.strings[i].data == NULL)) {
			len += 2;
		} else {
			len += l + 1;
		}
	}

	char *out = calloc(len, sizeof(char));
	size_t pos = 0;
	for (int i = 0; i < msg->data.names.entries; i++) {
		int l = 0;
		if ((msg->data.names.strings[i].length == 0) || (msg->data.names.strings[i].data == NULL)) {
			l = snprintf(&(out[pos]), (len - pos), "-/");
		} else {
			l = snprintf(&(out[pos]), (len - pos), "%s/", msg->data.names.strings[i].data);
		}
		if (l < 0) {
			free(out);
			return NULL;
		}
		pos += l;
	}
	out[pos - 1] = '\0';
	return out;
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
		// LCOV_EXCL_START
		default:
			fprintf(stderr, "Unhandled message type in msg_destroy!\n");
			break;
			// LCOV_EXCL_STOP
	}
	msg->length = 0;
	msg->dtype = MSG_UNDEF;
}
