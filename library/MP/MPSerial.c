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

#include <msgpack.h>
#include "MPTypes.h"
#include "MPSerial.h"

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
	static int hw = 0; // Current array end
	return mp_readMessage_buf(handle, out, buf, &index, &hw);
}


/*!
 * This function maintains a static internal message buffer, filling it from
 * the file handle provided. This handle can be anything supported by read(),
 * but would usually be a file or a serial port.
 *
 * If a valid message is found then it is written to the structure provided as
 * a parameter and the function returns true.
 *
 * If a message cannot be read, the function returns false and the float value
 * field is set to an error value:
 * - 0xFF means no message found yet, and more data is required
 * - 0xFD is a synonym for 0xFF, but indicates that zero bytes were read from source.
 *   This could indicate EOF if reading from file, but can be ignored when streaming from a device.
 * - 0xAA means that an error occurred reading in data
 * - 0XEE means a valid message header was found, but no valid message
 *
 * @param[in] handle File descriptor from mp_openConnection()
 * @param[out] out Pointer to message structure to fill with data
 * @param[in,out] buf Serial data buffer
 * @param[in,out] index Current search position within `buf`
 * @param[in,out] hw End of current valid data in `buf`
 * @return True if out now contains a valid message, false otherwise.
 *
 */
bool mp_readMessage_buf(int handle, msg_t *out, uint8_t buf[MP_SERIAL_BUFF], int *index, int *hw) {
	int ti = 0;
	if ((*hw) < MP_SERIAL_BUFF - 1) {
		errno = 0;
		ti = read(handle, &(buf[(*hw)]), MP_SERIAL_BUFF - (*hw));
		if (ti >= 0) {
			(*hw) += ti;
		} else {
			if (errno != EAGAIN) {
				fprintf(stderr, "Unexpected error while reading from serial port (handle ID: 0x%02x)\n", handle);
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
		//fprintf(stderr, "Buffer empty - returning\n");
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		if (ti == 0) {
			out->data.value = 0xFD;
		}
		return false;
	}

	if (((*hw) - (*index)) < 8) {
		// Not enough data for any valid message, come back later
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		return false;
	}

	if (buf[(*index)] != MP_SYNC_BYTE2) {
		// Found first sync byte, but second not valid
		// Advance the index so we skip this message and go back around
		(*index)++;
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		return false;
	}

	// We now know we have a good candidate for a valid MessagePacked message
	/*X
	if (((*hw) - (*index)) < (out->length + 8) ) {
		// Not enough data for this message yet, so mark output invalid
		out->dtype = MSG_ERROR;
		out->data.value = 0xFF;
		if (ti == 0) {
		out->dtype = MSG_ERROR;
			out->data.value = 0xFD;
		}
		// Go back around, but leave index where it is so we will pick up
		// from the same point in the buffer
		return false;
	}
	X*/

	bool valid = false;
	/*X
	bool valid = mp_check_checksum(out);
	if (valid) {
		(*index) += 8 + out->length ;
	} else {
		out->dtype = MSG_ERROR;
		out->data.value = 0xEE; // Use 0xEE as "Found, but invalid", leaving 0xFF as "No message"
		(*index)++;
	}
	X*/
	if ((*hw) > 0) {
		// Move data from index back to zero position
		memmove(buf, &(buf[(*index)]), MP_SERIAL_BUFF - (*index));
		(*hw) -= (*index);
		(*index) = 0;
	}
	return valid;
}

/*!
 * Takes a message, validates the source and message type and then writes MessagePacked data
 * to the device or file connected to `handle`.
 *
 * @param[in] handle File descriptor from mp_openConnection()
 * @param[in] out Pointer to message structure to be sent.
 * @return True if data successfullt written to `handle`
 */
bool mp_writeMessage(int handle, const msg_t *out) {
	msgpack_sbuffer sbuf;
	msgpack_packer pack;
	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pack, &sbuf, msgpack_sbuffer_write);
	msgpack_pack_array(&pack,4); // MP_SYNC_BYTE1
	msgpack_pack_int(&pack, MP_SYNC_BYTE2);
	msgpack_pack_int(&pack, out->source);
	msgpack_pack_int(&pack, out->type);
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
			msgpack_pack_str(&pack, out->data.string.length);
			msgpack_pack_str_body(&pack, out->data.string.data, out->data.string.length);
			break;

		case MSG_STRARRAY:
			mp_pack_strarray(&pack, &(out->data.names));
			break;

		case MSG_ERROR:
		case MSG_UNDEF:
		default:
			return false;
	}
	int ret = write(handle, sbuf.data, sbuf.size);
	msgpack_sbuffer_destroy(&sbuf);
	return (ret==sbuf.size);
}

/*!
 * Helper function for mp_writeMessage()
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
		msgpack_pack_str(pack, s->length);
		msgpack_pack_str_body(pack, s->data, s->length);
	}
}
