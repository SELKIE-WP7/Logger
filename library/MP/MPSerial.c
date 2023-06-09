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
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "MPSerial.h"
#include "MPTypes.h"
#include <msgpack.h>

/*!
 * Currently just wraps openSerialConnection()
 *
 * @param[in] port Target character device (i.e. Serial port)
 * @param[in] baudRate Target baud rate
 * @return Return value of openSerialConnection()
 */
int mp_openConnection(const char *port, const int baudRate) {
	return openSerialConnection(port, baudRate);
}

/*!
 * Currently just closes the file descriptor passed as `handle`
 *
 * @param[in] handle File descriptor opened with mp_openConnection()
 */
void mp_closeConnection(int handle) {
	close(handle);
}

/*!
 * For single threaded development and testing, uses static variables rather
 * than requiring state to be tracked by caller.
 *
 * See mp_readMessage_buf() for full description.
 *
 * @param[in] handle File descriptor from mp_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @return True if out now contains a valid message, false otherwise.
 */
bool mp_readMessage(int handle, msg_t *out) {
	static uint8_t buf[MP_SERIAL_BUFF];
	static int index = 0; // Current read position
	static int hw = 0;    // Current array end
	return mp_readMessage_buf(handle, out, buf, &index, &hw);
}

/*!
 * This function maintains a message buffer (allocated by the caller), filling
 * it from the file handle provided. This handle can be anything supported by
 * read(), but would usually be a file or a serial port.
 *
 * The index (current search position) and hw (high water / end of valid data)
 * values are also provided by the caller, but will be updated by this
 * function.
 *
 * If a valid message is found then it is written to the structure provided as
 * a parameter and the function returns true.
 *
 * If a message cannot be read, the function returns false and the float value
 * field is set to an error value:
 * - 0xFF means no message found yet, and more data is required
 * - 0xFD is a synonym for 0xFF, but indicates that zero bytes were read from source.
 *   This could indicate EOF if reading from file, but can be ignored when streaming from
 *   a device.
 * - 0xAA means that an error occurred reading in data
 * - 0XEE means a valid message header was found, but no valid message
 *
 * @param[in] handle File descriptor from mp_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @param[in,out] buf Serial data buffer
 * @param[in,out] index Current search position within `buf`
 * @param[in,out] hw End of current valid data in `buf`
 * @return True if out now contains a valid message, false otherwise.
 */
bool mp_readMessage_buf(int handle, msg_t *out, uint8_t buf[MP_SERIAL_BUFF], int *index, int *hw) {
	int ti = 0;
	if (out == NULL || (*index) < 0 || (*hw) < 0 || buf == NULL) { return false; }
	if ((*hw) < MP_SERIAL_BUFF - 1) {
		errno = 0;
		ti = read(handle, &(buf[(*hw)]), MP_SERIAL_BUFF - (*hw));
		if (ti >= 0) {
			(*hw) += ti;
		} else {
			if (errno != EAGAIN) {
				fprintf(stderr, "Unexpected error while reading from serial port (handle ID: 0x%02x)\n",
				        handle);
				fprintf(stderr, "read returned \"%s\" in readMessage\n", strerror(errno));
				out->dtype = MSG_ERROR;
				out->data.value = 0xAA;
				return false;
			}
		}
	}

	// Check buf[index] is valid ID
	while (!(buf[(*index)] == MP_SYNC_BYTE1) && (*index) < (*hw)) {
		(*index)++; // Current byte cannot be start of a message, so advance
	}
	if ((*index) == (*hw)) {
		if ((*hw) > 0 && (*index) > 0) {
			// Move data from index back to zero position
			memmove(buf, &(buf[(*index)]), MP_SERIAL_BUFF - (*index));
			(*hw) -= (*index);
			(*index) = 0;
		}
		// fprintf(stderr, "Buffer empty - returning\n");
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		if (ti == 0) { out->data.value = 0xFD; }
		return false;
	}

	if (((*hw) - (*index)) < 8) {
		// Not enough data for any valid message, come back later
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		if (ti == 0) { out->data.value = 0xFD; }
		return false;
	}

	if (buf[(*index) + 1] != MP_SYNC_BYTE2) {
		// Found first sync byte, but second not valid
		// Advance the index so we skip this message and go back around
		(*index)++;
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		return false;
	}

	// We now know we have a good candidate for a valid MessagePacked message
	msgpack_unpacker unpack;
	if (!msgpack_unpacker_init(&unpack, 64 + (*hw) - (*index))) {
		out->dtype = MSG_ERROR;
		out->data.value = 0xAA;
		fprintf(stderr, "Unable to initialise msgpack unpacker\n");
		return false;
	}
	const size_t upInitialOffset = unpack.off;
	memcpy(msgpack_unpacker_buffer(&unpack), &(buf[(*index)]), (*hw) - (*index));
	msgpack_unpacker_buffer_consumed(&unpack, (*hw) - (*index));

	msgpack_unpacked mpupd = {0};
	{
		msgpack_unpack_return rs = msgpack_unpacker_next(&unpack, &mpupd);
		switch (rs) {
			case MSGPACK_UNPACK_SUCCESS:
				// Will continue after the switch
				break;
			case MSGPACK_UNPACK_CONTINUE:
				// Need more data
				out->dtype = MSG_ERROR;
				out->data.value = 0xFF;
				msgpack_unpacker_destroy(&unpack);
				// Could still be a good message, so do not advance index
				return false;

			case MSGPACK_UNPACK_PARSE_ERROR:
			default:
				// Treat any unknown status as an error
				out->dtype = MSG_ERROR;
				out->data.value = 0xFF;
				msgpack_unpacker_destroy(&unpack);
				(*index)++; // Assume bad message, so advance 1 byte
				            // further into buffer
				return false;
		}

		if (mpupd.data.type != MSGPACK_OBJECT_ARRAY || mpupd.data.via.array.size != 4) {
			msgpack_unpacked_destroy(&mpupd);
			out->dtype = MSG_ERROR;
			out->data.value = 0xFF;
			msgpack_unpacker_destroy(&unpack);
			(*index)++;
			return false;
		}
	}

	// So at this point we have a 4 element array unpacked in mpupd.data
	// This is msgpack_object_array with two members:
	//  - size, which we've verified (4)
	//  - ptr, which is an array of msgpack_objects
	// Give the array we're interested in a shorter name
	msgpack_object *inArr = mpupd.data.via.array.ptr;

	// Verify our marker byte
	if (inArr[0].type != MSGPACK_OBJECT_POSITIVE_INTEGER || inArr[0].via.u64 != MP_SYNC_BYTE2) {
		msgpack_unpacked_destroy(&mpupd);
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		msgpack_unpacker_destroy(&unpack);
		(*index)++;
		return false;
	}

	// Next should be our Source ID
	if (inArr[1].type != MSGPACK_OBJECT_POSITIVE_INTEGER || inArr[1].via.u64 >= 128) {
		msgpack_unpacked_destroy(&mpupd);
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		msgpack_unpacker_destroy(&unpack);
		(*index)++;
		return false;
	}
	out->source = inArr[1].via.u64;

	// Then our Channel ID
	if (inArr[2].type != MSGPACK_OBJECT_POSITIVE_INTEGER || inArr[2].via.u64 >= 128) {
		msgpack_unpacked_destroy(&mpupd);
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		msgpack_unpacker_destroy(&unpack);
		(*index)++;
		return false;
	}
	out->type = inArr[2].via.u64;

	// Now for the really fun bit, dealing with whatever input data this message has
	// Our valid types are:
	// MSG_FLOAT -> MSGPACK_OBJECT_FLOAT
	// MSG_TIMESTAMP -> MSGPACK_OBECT_POSITIVE_INTEGER
	// MSG_STRING -> MSGPACK_OBJECT_STR
	// MSG_STRARRAY -> MSGPACK_OBJECT_ARRAY (of MSG_PACK_OBJECT_STRs)
	// MSG_NUMARRAY -> MSGPACK_OBJECT_ARRAY (of MSG_PACK_OBJECT_FLOAT32)
	bool valid = false;
	switch (inArr[3].type) {
		case MSGPACK_OBJECT_FLOAT32:
		case MSGPACK_OBJECT_FLOAT64: // Shouldn't happen, but we can accept it
			out->dtype = MSG_FLOAT;
			out->data.value = inArr[3].via.f64;
			valid = true;
			break;
		case MSGPACK_OBJECT_POSITIVE_INTEGER:
			out->dtype = MSG_TIMESTAMP;
			out->data.timestamp = inArr[3].via.u64;
			valid = true;
			break;
		case MSGPACK_OBJECT_STR:
			out->dtype = MSG_STRING;
			out->length = inArr[3].via.str.size;
			valid = str_update(&(out->data.string), inArr[3].via.str.size, inArr[3].via.str.ptr);
			break;
		case MSGPACK_OBJECT_ARRAY:
			// Switch based on first item type
			switch (inArr[3].via.array.ptr[0].type) {
				case MSGPACK_OBJECT_STR:
					out->dtype = MSG_STRARRAY;
					valid = mp_unpack_strarray(&(out->data.names), &(inArr[3].via.array));
					break;
				case MSGPACK_OBJECT_FLOAT32:
				case MSGPACK_OBJECT_FLOAT64:
					out->dtype = MSG_NUMARRAY;
					out->length = mp_unpack_numarray(&(out->data.farray), &(inArr[3].via.array));
					if (out->length > 0) { valid = true; }
					break;
				default:
					valid = false;
					break;
			}
			break;
		case MSGPACK_OBJECT_BIN:
			out->dtype = MSG_BYTES;
			out->length = inArr[3].via.bin.size;
			out->data.bytes = malloc(out->length);
			if (out->data.bytes == NULL) {
				valid = false;
				break;
			}
			memcpy(out->data.bytes, inArr[3].via.bin.ptr, out->length);
			valid = true;
			break;
		default:
			valid = false;
			break;
	}

	(*index) += (unpack.off - upInitialOffset);
	msgpack_unpacked_destroy(&mpupd);
	msgpack_unpacker_destroy(&unpack);

	if ((*hw) > 0) {
		// Move data from index back to zero position
		memmove(buf, &(buf[(*index)]), MP_SERIAL_BUFF - (*index));
		(*hw) -= (*index);
		(*index) = 0;
	}
	if (valid == false) {
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF; // Invalid message
		                        // Index already incremented and reset above
	}
	return valid;
}

/*!
 * Pack a message into the provided buffer, ready for writing.
 * The buffer is initialised by this function, and destroyed on error.
 *
 * Caller is responsible for destroying buffer after use
 *
 * @param[out] sbuf 	msgpack_sbuffer to write into
 * @param[in]  out	Message to pack into buffer
 * @returns true on success, false on error
 */
bool mp_packMessage(msgpack_sbuffer *sbuf, const msg_t *out) {
	msgpack_packer pack = {0};
	msgpack_sbuffer_init(sbuf);
	msgpack_packer_init(&pack, sbuf, msgpack_sbuffer_write);
	msgpack_pack_array(&pack, 4); // MP_SYNC_BYTE1
	msgpack_pack_int(&pack, MP_SYNC_BYTE2);
	msgpack_pack_int(&pack, out->source);
	msgpack_pack_int(&pack, out->type);
	size_t sl = 0;
	switch (out->dtype) {
		case MSG_FLOAT:
			msgpack_pack_float(&pack, out->data.value);
			break;

		case MSG_TIMESTAMP:
			msgpack_pack_uint32(&pack, out->data.timestamp);
			break;

		case MSG_BYTES:
			msgpack_pack_bin(&pack, out->length);
			msgpack_pack_bin_body(&pack, out->data.bytes, out->length);
			break;

		case MSG_STRING:
			sl = out->data.string.length;
			if (strlen(out->data.string.data) < sl) { sl = strlen(out->data.string.data); }
			msgpack_pack_str(&pack, sl);
			msgpack_pack_str_body(&pack, out->data.string.data, sl);
			break;

		case MSG_STRARRAY:
			mp_pack_strarray(&pack, &(out->data.names));
			break;
		case MSG_NUMARRAY:
			mp_pack_numarray(&pack, out->length, out->data.farray);
			break;
		case MSG_ERROR:
		case MSG_UNDEF:
		default:
			msgpack_sbuffer_destroy(sbuf);
			return false;
	}
	return true;
}

/*!
 * Packs message using mp_packMessage and writes it to a file descriptor
 *
 * @param[in] handle File descriptor from mp_openConnection()
 * @param[in] out Pointer to message structure to be sent.
 * @return True if data successfully written to `handle`
 */
bool mp_writeMessage(int handle, const msg_t *out) {
	msgpack_sbuffer sbuf;
	if (!mp_packMessage(&sbuf, out)) { return false; }
	int ret = write(handle, sbuf.data, sbuf.size);
	msgpack_sbuffer_destroy(&sbuf);
	return (ret == (ssize_t) sbuf.size);
}

/*!
 * Extract message data and writes it (unformatted) to a file descriptor
 *
 * @param[in] handle File descriptor from mp_openConnection()
 * @param[in] out Pointer to message structure containing data to be extracted.
 * @return True if data successfully written to `handle`
 */
bool mp_writeData(int handle, const msg_t *out) {
	size_t sl = 0;
	switch (out->dtype) {
		case MSG_FLOAT:
			return (write(handle, &(out->data.value), sizeof(out->data.value)) == sizeof(out->data.value));

		case MSG_TIMESTAMP:
			return (write(handle, &(out->data.timestamp), sizeof(out->data.timestamp)) ==
			        (ssize_t) sizeof(out->data.timestamp));

		case MSG_BYTES:
			return (write(handle, out->data.bytes, out->length) == (ssize_t) out->length);

		case MSG_STRING:
			sl = out->data.string.length;
			if (strlen(out->data.string.data) < sl) { sl = strlen(out->data.string.data); }
			return (write(handle, &(out->data.string.data), sl) == (ssize_t) sl);

		case MSG_STRARRAY:
			for (int ix = 0; ix < out->data.names.entries; ix++) {
				sl = out->data.names.strings[ix].length;
				if (sl > 0 && strlen(out->data.names.strings[ix].data) < sl) {
					sl = strlen(out->data.names.strings[ix].data);
				}
				if (!(write(handle, out->data.names.strings[ix].data, sl) == (ssize_t) sl)) { return false; }
			}
			return true;

		case MSG_NUMARRAY:
			sl = sizeof(out->data.farray[0]) * out->length;
			return (write(handle, out->data.farray, sl) == ((ssize_t) sl));

		case MSG_ERROR:
		case MSG_UNDEF:
		default:
			return false;
	}

	return false;
}

/*!
 * Helper function for mp_packMessage()
 *
 * Packs string array into supplied msgpack_packer object
 *
 * @param[in] pack Pointer to msgpack packer object - must already be initialised
 * @param[in] sa Pointer to string array to be packed
 */
void mp_pack_strarray(msgpack_packer *pack, const strarray *sa) {
	msgpack_pack_array(pack, sa->entries);
	for (int ix = 0; ix < sa->entries; ix++) {
		string *s = &(sa->strings[ix]);
		size_t sl = s->length;
		if (sl > 0 && strlen(s->data) < sl) { sl = strlen(s->data); }
		msgpack_pack_str(pack, sl);
		msgpack_pack_str_body(pack, s->data, sl);
	}
}

/*!
 * Helper function for mp_packMessage()
 *
 * Packs float array into supplied msgpack_packer object
 *
 * @param[in] pack    Pointer to msgpack packer object - must already be initialised
 * @param[in] entries Number of entries in array
 * @param[in] fa      Pointer to float array to be packed
 */
void mp_pack_numarray(msgpack_packer *pack, const size_t entries, const float *fa) {
	msgpack_pack_array(pack, entries);
	for (unsigned int ix = 0; ix < entries; ix++) {
		msgpack_pack_float(pack, fa[ix]);
	}
}

/*!
 * Helper function for mp_readMessage_buf()
 *
 * Unpacks a msgpack_object_array into a string array, provided all
 * msgpack_objects are strings.
 *
 * @param[in] sa  Pointer to string array to be updated
 * @param[in] obj MessagePacked array
 * @return True on success, False on error
 */
bool mp_unpack_strarray(strarray *sa, msgpack_object_array *obj) {
	const int nEntries = obj->size;
	sa_destroy(sa); // Just in case
	sa->entries = nEntries;
	sa->strings = calloc(nEntries, sizeof(string));

	if (sa->strings == NULL) { return false; }

	for (int ix = 0; ix < nEntries; ix++) {
		if (obj->ptr[ix].type != MSGPACK_OBJECT_STR) {
			sa_destroy(sa);
			return false;
		}
		if (!sa_create_entry(sa, ix, obj->ptr[ix].via.str.size, obj->ptr[ix].via.str.ptr)) {
			sa_destroy(sa);
			return false;
		}
	}
	return true;
}

/*!
 * Helper function for mp_readMessage_buf()
 *
 * Unpacks a msgpack_object_array into an array of floats, provided all
 * msgpack_objects are strings.
 *
 * @param[out] fa  Pointer to float pointer (will be set to base address of array)
 * @param[in] obj MessagePacked array
 * @return Number of entries in array, -1 on failure
 */
size_t mp_unpack_numarray(float **fa, msgpack_object_array *obj) {
	const int nEntries = obj->size;
	(*fa) = calloc(nEntries, sizeof(float));

	if ((*fa) == NULL) { return -1; }

	for (int ix = 0; ix < nEntries; ix++) {
		if (!((obj->ptr[ix].type == MSGPACK_OBJECT_FLOAT32) || (obj->ptr[ix].type == MSGPACK_OBJECT_FLOAT64))) {
			free((*fa));
			(*fa) = NULL;
			return -1;
		}
		(*fa)[ix] = obj->ptr[ix].via.f64;
	}
	return nEntries;
}
