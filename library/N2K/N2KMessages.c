#include <stdio.h>
#include <stdlib.h>

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

void n2k_127257_print(const n2k_act_message *n) {
	if (n->PGN != 127257) {
		fprintf(stdout, "Bad PGN - Got %d, expected 127257\n", n->PGN);
		return;
	}

	if (!n->data || n->datalen < 7) {
		fprintf(stdout, "Insufficient data (%d bytes) for PGN 127257\n", n->datalen);
		return;
	}

	uint8_t seq = n2k_get_uint8(n, 0);
	double yaw = n2k_get_int16(n, 1) * 0.0057295779513082332;
	double pitch = n2k_get_int16(n, 3) * 0.0057295779513082332;
	double roll = n2k_get_int16(n, 5) * 0.0057295779513082332;
	fprintf(stdout, "Pitch: %.3lf, Roll: %.3lf, Yaw: %.3lf. Seq. ID: %03d\n", pitch, roll, yaw, seq);
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
	uint8_t magnetic = (n2k_get_uint8(n, 1) & 0x3);
	double course = n2k_get_int16(n, 2) * 0.0057295779513082332;
	double speed = n2k_get_int16(n, 4) * 0.01;

	char *magStr = NULL;
	switch (magnetic) {
		case 0:
			magStr = "True";
			break;
		case 1:
			magStr = "Magnetic";
			break;
		default:
			magStr = "Unknown Reference";
			break;
	}

	fprintf(stdout, "Speed: %.2lf @ %.3lf degrees [%s]. Seq. ID %03d\n", speed, course, magStr, seq);
}

void n2k_130306_print(const n2k_act_message *n) {
	if (n->PGN != 130306) {
		fprintf(stdout, "Bad PGN - Got %d, expected 130306\n", n->PGN);
		return;
	}
	if (!n->data || n->datalen < 8) {
		fprintf(stdout, "Insufficient data (%d bytes) for PGN 130306\n", n->datalen);
		return;
	}

	uint8_t seq = n2k_get_uint8(n, 0);
	double speed = n2k_get_int16(n, 1) * 0.01;
	double angle = n2k_get_int16(n, 3) * 0.0057295779513082332;
	uint8_t ref = (n2k_get_uint8(n, 5) & 0x07);

	char *refStr = NULL;
	switch (ref) {
		case 0:
			refStr = "Relative to North";
			break;
		case 1:
			refStr = "Magnetic";
			break;
		case 2:
			refStr = "Apparent";
			break;
		case 3:
			refStr = "Relative to boat";
			break;
		default:
			refStr = "Unknown reference";
			break;
	}
	fprintf(stdout, "Wind Speed: %.2lf @ %.3lf degrees [%s]. Seq. ID %03d\n", speed, angle, refStr, seq);
}
