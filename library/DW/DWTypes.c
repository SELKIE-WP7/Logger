#include <stddef.h>

#include "DWTypes.h"

bool hexpair_to_uint(const char *in, uint8_t *out) {
	if (in == NULL || out == NULL) {
		return false;
	}

	uint8_t v = 0;
	for (int i=0; i < 2; i++) {
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
				v += (in[i] - '0') << (4*(1-i));
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				v += (in[i] - 'a' + 10) << (4*(1-i));
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				v += (in[i] - 'A' + 10) << (4*(1-i));
				break;
			default:
				(*out) = 0;
				return false;
		}
	}
	(*out) = v;
	return true;
}

