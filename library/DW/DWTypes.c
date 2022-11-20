#include <stddef.h>
#include <unistd.h>

#include "DWTypes.h"

/*!
 * Character array pointed to by `in` must contain at least two characters
 * (i.e. char[1] must be a valid read).
 *
 * @param[in] in Input string containing two hexadecimal characters
 * @param[out] out Pointer to output variable. Zeroed for error cases.
 * @return True on success, false on error.
 */
bool hexpair_to_uint(const char *in, uint8_t *out) {
	if (in == NULL || out == NULL) { return false; }

	uint8_t v = 0;
	for (int i = 0; i < 2; i++) {
		switch (in[i]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				v += (in[i] - '0') << (4 * (1 - i));
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				v += (in[i] - 'a' + 10) << (4 * (1 - i));
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				v += (in[i] - 'A' + 10) << (4 * (1 - i));
				break;
			default:
				(*out) = 0;
				return false;
		}
	}
	(*out) = v;
	return true;
}

/*!
 * Read up to `end` characters from `in` and populate the dw_hxv structure
 * pointed to by `out`.
 *
 * Looks for a carriage return character to mark the end of a line, then
 * attempts to read a HXV line from the data immediately preceding it.
 *
 * The `data` parameter initially indicates the number of characters available,
 * and will be set to the number of characters read at exit.
 *
 * @param[in] in Pointer to character array containing input data
 * @param[in,out] end Data length/position marker
 * @param[out] out Pointer to dw_hxv structure to fill
 * @return True on success, false on error.
 */
bool dw_string_hxv(const char *in, size_t *end, dw_hxv *out) {
	if (in == NULL || out == NULL || (*end) < 25) { return false; }

	ssize_t le = -1; // Line end
	for (unsigned int i = 0; i < (*end); ++i) {
		if (in[i] == '\r') {
			le = i;
			break;
		}
	}
	if (le < 0) { return false; }

	// Work backwards from the carriage return
	bool ret = true;
	ret &= hexpair_to_uint(&(in[le - 24]), &(out->status));
	ret &= hexpair_to_uint(&(in[le - 22]), &(out->lines));
	ret &= hexpair_to_uint(&(in[le - 19]), &(out->data[0]));
	ret &= hexpair_to_uint(&(in[le - 17]), &(out->data[1]));
	ret &= hexpair_to_uint(&(in[le - 14]), &(out->data[2]));
	ret &= hexpair_to_uint(&(in[le - 12]), &(out->data[3]));
	ret &= hexpair_to_uint(&(in[le - 9]), &(out->data[4]));
	ret &= hexpair_to_uint(&(in[le - 7]), &(out->data[5]));
	ret &= hexpair_to_uint(&(in[le - 4]), &(out->data[6]));
	ret &= hexpair_to_uint(&(in[le - 2]), &(out->data[7]));

	(*end) = ++le;
	return ret;
}
