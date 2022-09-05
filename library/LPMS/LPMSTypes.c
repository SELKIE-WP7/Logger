#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "LPMSTypes.h"

/*!
 *
 * Populate lpms_message from array of bytes, searching for valid start byte if
 * required.
 *
 * @param[in] in Array of bytes
 * @param[in] len Number of bytes available in array
 * @param[out] msg Pointer to lpms_message
 * @param[out] pos Number of bytes consumed
 * @returns True on success, false on error
 */
bool lpms_from_bytes(const uint8_t *in, const size_t len, lpms_message *msg, size_t *pos) {
	if (in == NULL || msg == NULL || len < 10 || pos == NULL) { return NULL; }

	ssize_t start = -1;
	while (((*pos) + 10) < len) {
		if ((in[(*pos)] == LPMS_START)) {
			start = (*pos);
			break;
		}
		(*pos)++;
	}

	if (start < 0) {
		// Nothing found, so bail early
		// (*pos) marks the end of the searched data
		return false;
	}

	msg->id = in[start + 1] + ((uint16_t)in[start + 2] << 8);
	msg->command = in[start + 3] + ((uint16_t)in[start + 4] << 8);
	msg->length = in[start + 5] + ((uint16_t)in[start + 6] << 8);

	if (msg->length == 0) {
		msg->data = NULL;
	} else {
		// Check there's enough bytes in buffer to cover message header
		// (6 bytes), footer (4 bytes), and embedded data
		if ((len - start - 10) < msg->length) {
			msg->id = 0xFF;
			return false;
		}
		msg->data = calloc(msg->length, sizeof(uint8_t));
		if (msg->data == NULL) {
			msg->id = 0xAA;
			return false;
		}

		(*pos) += 7; // Move (*pos) up now we know we can read data in
		for (int i = 0; i < msg->length; i++) {
			msg->data[i] = in[(*pos)++];
		}
	}
	msg->checksum = in[(*pos)] + ((uint16_t)in[(*pos) + 1] << 8);
	(*pos) += 2;
	if ((in[(*pos)] != LPMS_END1) || (in[(*pos) + 1] != LPMS_END2)) {
		msg->id = 0xEE;
		return false;
	}
	(*pos) += 2;
	return true;
}

/*!
 * @param[in] msg Pointer to message structure
 * @param[out] csum Calculated checksum value
 * @returns True if calculated successfully, false on error
 */
bool lpms_checksum(const lpms_message *msg, uint16_t *csum) {
	if (msg == NULL || csum == NULL) { return false; }
	uint16_t cs = 0;
	cs += (msg->id & 0x00FF);
	cs += (msg->id & 0xFF00) >> 8;
	cs += (msg->command & 0x00FF);
	cs += (msg->command & 0xFF00) >> 8;
	cs += (msg->length & 0x00FF);
	cs += (msg->length & 0xFF00) >> 8;
	if (msg->length > 0) {
		if (msg->data == NULL) { return false; }
		for (int i = 0; i < msg->length; ++i) {
			cs += msg->data[i];
		}
	}
	(*csum) = cs;
	return true;
}
