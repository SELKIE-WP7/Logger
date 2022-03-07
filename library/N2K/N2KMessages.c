#include <stdio.h>

#include "N2KMessages.h"
#include "N2KTypes.h"

uint8_t n2k_get_uint8(const n2k_act_message *n, size_t offset) {
	return n->data[offset];
}

int8_t n2k_get_int8(const n2k_act_message *n, size_t offset) {
	uint8_t u = n2k_get_uint8(n, offset);
	int8_t v = (u & 0x7F);
	if (u & 0x80) { v = -1 * ((1 << 7) - v); }
	return v;
}

int16_t n2k_get_int16(const n2k_act_message *n, size_t offset) {
	uint16_t u = n2k_get_uint16(n, offset);
	int16_t v = (u & 0x7FFF);
	if (u & 0x8000) { v = -1 * ((1 << 15) - v); }
	return v;
}

uint16_t n2k_get_uint16(const n2k_act_message *n, size_t offset) {
	return n->data[offset] + (n->data[offset + 1] << 8);
}

int32_t n2k_get_int32(const n2k_act_message *n, size_t offset) {
	uint32_t u = n2k_get_uint32(n, offset);
	int32_t v = (u & 0x7FFFFFFF);
	if (u & 0x80000000) { v = -1 * ((1 << 31) - v); }
	return v;
}

uint32_t n2k_get_uint32(const n2k_act_message *n, size_t offset) {
	uint32_t v = n->data[offset] + (uint32_t)(n->data[offset + 1] << 8) + (uint32_t)(n->data[offset + 2] << 16) +
	             (uint32_t)(n->data[offset + 3] << 24);
	return v;
}

void n2k_129025_print(const n2k_act_message *n) {
	if (n->PGN != 129025) {
		fprintf(stdout, "Bad PGN - Got %d, expected 129025\n", n->PGN);
		return;
	}

	if (!n->data || n->datalen < 8) {
		fprintf(stdout, "Insufficient data (%d bytes) for PGN 129025\n", n->datalen);
		return;
	}
	double lat = n2k_get_int32(n, 0) * 1E-7;
	double lon = n2k_get_int32(n, 4) * 1E-7;
	fprintf(stdout, "GPS Position: %lf, %lf\n", lat, lon);
}

void n2k_129026_print(const n2k_act_message *n) {
	if (n->PGN != 129026) {
		fprintf(stdout, "Bad PGN - Got %d, expected 129026\n", n->PGN);
		return;
	}
	if (!n->data || n->datalen < 8) {
		fprintf(stdout, "Insufficient data (%d bytes) for PGN 129026\n", n->datalen);
		return;
	}
	uint8_t seq = n2k_get_uint8(n, 0);
	uint8_t magnetic = (n2k_get_uint8(n, 1) & 0xC0) >> 6;
	double course = n2k_get_int16(n, 2) * 0.0057295779513082332;
	double speed = n2k_get_int16(n, 4) * 0.01;

	fprintf(stdout, "Speed: %.2lf @ %.3lf degrees [True/Magnetic: %02d]. Seq. ID %03d\n", speed, course, magnetic,
	        seq);
}
