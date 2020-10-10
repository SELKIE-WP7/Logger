#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "NMEAMessages.h"

/*!
 * Calculates the checksum for an NMEA0183 style message
 *
 * @param[in] msg Input message
 * @param[out] cs Checksum byte
 */
void nmea_calc_checksum(const nmea_msg_t *msg, uint8_t *cs) {
	uint8_t a = 0;

	// Checksum does not include start byte
	a ^= msg->talker[0];
	a ^= msg->talker[1];
	if (msg->talker[0] == 'P') {
		a ^= msg->talker[2];
		a ^= msg->talker[3];
	}
	a ^= msg->message[0];
	a ^= msg->message[1];
	a ^= msg->message[2];

	if (msg->fields.entries > 0) {
		for (int fi = 0; fi < msg->fields.entries; fi++) {
			a ^= ',';
			for (int ffi = 0; ffi < msg->fields.strings[fi].length; ffi++) {
				a ^= msg->fields.strings[fi].data[ffi];
			}
		}
	} else {
		a ^= ',';
		for (int ri = 0; ri < msg->rawlen; ri++) {
			a ^= msg->raw[ri];
		}
	}
	*cs = a;
	return;
}


/*!
 * Simple wrapper around nmea_calc_checksum() to update the checksum bytes
 * within the structure.
 *
 * @param[in,out] msg Message to update
 */
void nmea_set_checksum(nmea_msg_t *msg) {
	nmea_calc_checksum(msg, &(msg->checksum));
}

/*!
 * Calculates checksum with nmea_calc_checksum() and compares to checksumstored in
 * the provided message.
 *
 * @param[in] msg Input message
 * @return Checksum validity (true/false)
 */
bool nmea_check_checksum(const nmea_msg_t *msg) {
	uint8_t a = 0;
	nmea_calc_checksum(msg, &a);
	if (msg->checksum == a) {
		return true;
	}
	return false;
}

/*!
 * Calculate the number of bytes/characters required to represent or transmit
 * an NMEA message
 *
 * @param[in] msg Pointer to NMEA message structure
 * @return Bytes required
 */
size_t nmea_message_length(const nmea_msg_t *msg) {
	size_t asize = 3; // Minimum - start byte + CRLF
	asize += 2; // Minimum talker size
	if (msg->talker[0] == 'P') {
		asize += 2; // Bring up to 4 for proprietary messages
	}
	asize += 3; // Message type
	if (msg->fields.entries > 0) {
		for (int fi=0; fi < msg->fields.entries; fi++) {
			asize += msg->fields.strings[fi].length + 1; // +1 for field delimiter
		}
	} else {
		asize += msg->rawlen;
	}
	asize += 5; // Checksum and delimiter, CR, LF
	return asize;
}

/*!
 * Allocates a new array of bytes and copies message into array in transmission order (e.g. out[0] to be sent first).
 * Note that this will include a trailing CRLF pair!
 *
 * Output array must be freed by caller.
 *
 * @param[in] msg Input message
 * @param[out] out Base address of output array
 * @return Size of output array
 */
size_t nmea_flat_array(const nmea_msg_t *msg, char **out) {
	char *outarray = calloc(nmea_message_length(msg), 1);
	size_t ix = 0;
	outarray[ix++] = msg->encapsulated ? NMEA_START_BYTE2 : NMEA_START_BYTE1;
	outarray[ix++] = msg->talker[0];
	outarray[ix++] = msg->talker[1];
	if (msg->talker[0] == 'P') {
		outarray[ix++] = msg->talker[2];
		outarray[ix++] = msg->talker[3];
	}
	outarray[ix++] = msg->message[0];
	outarray[ix++] = msg->message[1];
	outarray[ix++] = msg->message[2];
	if (msg->fields.entries > 0) {
		for (int fi = 0; fi < msg->fields.entries; fi++) {
			outarray[ix++] = ',';
			for (int ffi = 0; ffi < msg->fields.strings[fi].length; ffi++) {
				outarray[ix++] = msg->fields.strings[fi].data[ffi];
			}
		}
	} else {
		outarray[ix++] = ',';
		for (int ri = 0; ri < msg->rawlen; ri++) {
			outarray[ix++] = msg->raw[ri];
		}
	}
	outarray[ix++] = NMEA_CSUM_MARK;

	const char* hd = "0123456789ABCDEF";
	outarray[ix++] = hd[(msg->checksum >> 8) & 0xF];
	outarray[ix++] = hd[msg->checksum & 0xF];
	(*out) = outarray;
	return ix;
}

/*!
 * Allocates a character array and fills with hexadecimal character pairs
 * representing the message content.
 *
 * As we're dealing with ASCII anyway, this just wraps nmea_flat_array and adds
 * a null terminator
 *
 * Character array must be freed by caller.
 *
 * @param[in] msg Input message
 * @return Pointer to character array
 */
char * nmea_string_hex(const nmea_msg_t *msg) {
	char *str = NULL;
	size_t s = nmea_flat_array(msg, &str);
	str = realloc(str, s+1);
	str[s] = 0x00; // Null terminate string representation!
	return str;
}

/*!
 * Primarily for debug purposes. Will print string generated by
 * nmea_string_hex() to stdout.
 *
 * @param[in] *msg Input message
 */
void nmea_print_hex(const nmea_msg_t *msg) {
	char *out = nmea_string_hex(msg);
	if (out) {
		printf("%s\n", out);
		free(out);
		out = NULL;
	}
}

/*!
 * Parses fields from raw[] into the fields array
 *
 * @param[in] nmsg Pointer to NMEA message
 */
bool nmea_parse_fields(nmea_msg_t *nmsg) {
	//! TODO
	return false;
}
