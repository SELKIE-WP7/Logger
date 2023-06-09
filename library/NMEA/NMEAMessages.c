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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

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
			for (unsigned int ffi = 0; ffi < msg->fields.strings[fi].length; ffi++) {
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
	if (msg->checksum == a) { return true; }
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
	asize += 2;       // Minimum talker size
	if (msg->talker[0] == 'P') {
		asize += 2; // Bring up to 4 for proprietary messages
	}
	asize += 3; // Message type
	if (msg->fields.entries > 0) {
		for (int fi = 0; fi < msg->fields.entries; fi++) {
			asize += msg->fields.strings[fi].length + 1; // +1 for field delimiter
		}
	} else {
		asize += msg->rawlen;
	}
	asize += 5; // Checksum and delimiter, CR, LF
	return asize;
}

/*!
 * Allocates a new array of bytes and copies message into array in transmission order
 * (e.g. out[0] to be sent first). Note that this will include a trailing CRLF pair!
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
			for (unsigned int ffi = 0; ffi < msg->fields.strings[fi].length; ffi++) {
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

	const char *hd = "0123456789ABCDEF";
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
char *nmea_string_hex(const nmea_msg_t *msg) {
	char *str = NULL;
	size_t s = nmea_flat_array(msg, &str);
	str = realloc(str, s + 1);
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
 * Parses fields from raw into a string array
 *
 * @param[in] nmsg Pointer to NMEA message
 * @returns Pointer to string array (NULL on failure)
 */
strarray *nmea_parse_fields(const nmea_msg_t *nmsg) {
	const uint8_t *fields[80] = {0};
	int lengths[80] = {0};
	int fc = 0;
	const uint8_t *sp = nmsg->raw;
	for (int fp = 0; fp < nmsg->rawlen; fp++) {
		if ((nmsg->raw[fp] == ',') || (nmsg->raw[fp] == 0)) {
			fields[fc] = sp;
			lengths[fc] = &(nmsg->raw[fp]) - sp;
			fc++;
			sp = &(nmsg->raw[fp + 1]);
		}
	}
	fields[fc] = sp;
	lengths[fc] = &(nmsg->raw[nmsg->rawlen]) - sp;
	fc++;

	strarray *sa = sa_new(fc);
	if (sa == NULL) { return false; }
	for (int fn = 0; fn < fc; fn++) {
		if (!sa_create_entry(sa, fn, lengths[fn], (const char *)fields[fn])) {
			sa_destroy(sa);
			free(sa);
			return NULL;
		}
	}
	return sa;
}

/*!
 * Converts time encoded in message to a libc struct tm representation
 *
 * Caller to free returned structure
 *
 * @param[in] msg Input message
 * @returns Pointer to struct tm, or NULL on failure
 */
struct tm *nmea_parse_zda(const nmea_msg_t *msg) {
	const char *tk = "II";
	const char *mt = "ZDA";
	if ((strncmp(msg->talker, tk, 2) != 0) || (strncmp(msg->message, mt, 3) != 0)) { return NULL; }
	strarray *sa = nmea_parse_fields(msg);
	if (sa == NULL) { return NULL; }
	if (sa->entries != 6) {
		sa_destroy(sa);
		free(sa);
		return NULL;
	}
	/*
	 * Should have 6 fields:
	 * - HHMMSS[.sss]
	 * - DD
	 * - MM
	 * - YYYY
	 * - TZ Hours+-
	 * - TZ Minutes+-
	 *
	 * struct tm has no fractional seconds, so those will be discarded if present
	 */

	// Check lengths of required fields
	if (sa->strings[0].length < 6 || sa->strings[1].length < 1 || sa->strings[2].length < 1 ||
	    sa->strings[3].length != 4) {
		sa_destroy(sa);
		free(sa);
		return NULL;
	}

	struct tm *tout = calloc(1, sizeof(struct tm));
	if (tout == NULL) {
		sa_destroy(sa);
		free(sa);
		return NULL;
	}

	{
		errno = 0;
		int day = strtol(sa->strings[1].data, NULL, 10);
		if (errno || day < 0 || day > 31) {
			free(tout);
			sa_destroy(sa);
			free(sa);
			return NULL;
		}
		tout->tm_mday = day;
	}
	{
		errno = 0;
		int mon = strtol(sa->strings[2].data, NULL, 10);
		if (errno || mon < 0 || mon > 12) {
			free(tout);
			sa_destroy(sa);
			free(sa);
			return NULL;
		}
		tout->tm_mon = mon - 1; // Months since January 1, not month number
	}
	{
		errno = 0;
		int year = strtol(sa->strings[3].data, NULL, 10);
		// Year boundaries are arbitrary, but allows for some historical and
		// future usage If anyone is using this software in the year 2100 then: a)
		// I'm surprised! b) Update the upper bound below
		if (errno || year < 1970 || year > 2100) {
			free(tout);
			sa_destroy(sa);
			free(sa);
			return NULL;
		}
		tout->tm_year = year - 1900;
	}

	/*
	 * At this point, we take advantage of mktime to tidy things up and
	 * fill out some of the other tm_ fields for us rather than working out
	 * day of week, year etc.
	 *
	 * mktime would obliterate timezone details though, so we'll set the
	 * time and zone information afterwards.
	 */
	if (mktime(tout) == (time_t)(-1)) {
		free(tout);
		sa_destroy(sa);
		free(sa);
		return NULL;
	}

	{
		errno = 0;
		int time = strtol(sa->strings[0].data, NULL, 10);
		if (errno || time < 0 || time > 235960) {
			free(tout);
			sa_destroy(sa);
			free(sa);
			return NULL;
		}
		tout->tm_hour = time / 10000;
		tout->tm_min = (time % 10000) / 100;
		tout->tm_sec = (time % 100);
	}

	{
		errno = 0;
		int tzhours = 0;
		int tzmins = 0;

		// Assume no offset if strings not present
		if (sa->strings[4].length > 0) { strtol(sa->strings[4].data, NULL, 10); }
		if (sa->strings[5].length > 0) { strtol(sa->strings[5].data, NULL, 10); }
		if (errno || tzhours < -13 || tzhours > 13 || tzmins < -60 || tzmins > 60) {
			free(tout);
			sa_destroy(sa);
			free(sa);
			return NULL;
		}

		tout->tm_gmtoff = 3600 * tzhours + 60 * tzmins;
	}

	sa_destroy(sa);
	free(sa);

	return tout;
}
