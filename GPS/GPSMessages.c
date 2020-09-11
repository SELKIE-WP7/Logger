#include <stdlib.h>
#include <stdio.h>

#include "GPSMessages.h"

void ubx_calc_checksum(const ubx_message *msg, uint8_t *csA, uint8_t *csB) {
	uint8_t a = 0;
	uint8_t b = 0;
	a += msg->msgClass;
	b += a;
	a += msg->msgID;
	b += a;
	a += (msg->length & 0xFF);
	b += a;
	a += (msg->length >> 8);
	b += a;
	if (msg->length <= 256) {
		for (uint8_t dx = 0; dx < msg->length; dx++) {
			a += msg->data[dx];
			b += a;
		}
	} else {
		for (uint16_t dx = 0; dx < msg->length; dx++) {
			a += msg->extdata[dx];
			b += a;
		}
	}
	*csA = a;
	*csB = b;
	return;
}


void ubx_set_checksum(ubx_message *msg) {
	ubx_calc_checksum(msg, &(msg->csumA), &(msg->csumB));
}

bool ubx_check_checksum(const ubx_message *msg) {
	uint8_t a = 0;
	uint8_t b = 0;
	ubx_calc_checksum(msg, &a, &b);
	if ((msg->csumA == a) && (msg->csumB == b)) {
		return true;
	}
	return false;
}


char * ubx_string_hex(const ubx_message *msg) {
	int strlength = 24 + 3 * msg->length;
	char *str = calloc(strlength, 1);
	sprintf(str, "%02x %02x %02x %02x ", msg->sync1, msg->sync2, msg->msgClass, msg->msgID);
	sprintf(str+12, "%02x %02x ", (uint8_t) (msg->length & 0xFF), (uint8_t) (msg->length >> 8));
	if (msg->length <= 256) {
		for (uint8_t ix = 0; ix < msg->length; ix++) {
			sprintf(str+18+3*ix, "%02x ", msg->data[ix]);
		}
	} else {
		for (uint16_t ix = 0; ix < msg->length; ix++) {
			sprintf(str+18+3*ix,"%02x ", msg->extdata[ix]);
		}
	}
	sprintf(str+18+3*msg->length, "%02x %02x", msg->csumA, msg->csumB);
	return str;
}

void ubx_print_hex(const ubx_message *msg) {
	char *out = ubx_string_hex(msg);
	if (out) {
		printf("%s\n", out);
		free(out);
		out = NULL;
	}
}
