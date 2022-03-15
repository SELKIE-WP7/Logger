#include <stdio.h>
#include <stdlib.h>

#include <math.h>

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
	int32_t v = (u & 0x7FFFFFFFUL);
	if (u & 0x80000000UL) {
		v = -1 * ((1UL << 31) - v);
	}
	return v;
}

uint32_t n2k_get_uint32(const n2k_act_message *n, size_t offset) {
	uint32_t v = n->data[offset] + (uint32_t)(n->data[offset + 1] << 8) + (uint32_t)(n->data[offset + 2] << 16) +
	             (uint32_t)(n->data[offset + 3] << 24);
	return v;
}

bool n2k_127257_values(const n2k_act_message *n, uint8_t *seq, double *yaw, double *pitch, double *roll) {
	if (n->PGN != 127257 || !n->data || n->datalen < 7) { return false; }
	if (seq) { *seq = n2k_get_uint8(n, 0); }
	bool success = true;
	if (yaw) {
		int16_t y = n2k_get_int16(n, 1);
		if (y == INT16_MAX) {
			(*yaw) = NAN;
			success = false;
		} else {
			(*yaw) = y * N2K_TO_DEGREES;
		}
	}
	if (pitch) {
		int16_t p = n2k_get_int16(n, 3);
		if (p == INT16_MAX) {
			(*pitch) = NAN;
			success = false;
		} else {
			(*pitch) = p * N2K_TO_DEGREES;
		}
	}
	if (roll) {
		int16_t r = n2k_get_int16(n, 5);
		if (r == INT16_MAX) {
			(*roll) = NAN;
			success = false;
		} else {
			(*roll) = r * N2K_TO_DEGREES;
		}
	}
	return success;
}

bool n2k_129025_values(const n2k_act_message *n, double *lat, double *lon) {
	if (n->PGN != 129025 || !n->data || n->datalen < 8) { return false; }
	if (lat) {
		int32_t l = n2k_get_int32(n, 0);
		if (l == INT32_MAX) {
			(*lat) = NAN;
		} else {
			(*lat) = l * 1E-7;
		}
	}
	if (lon) {
		int32_t l = n2k_get_int32(n, 4);
		if (l == INT32_MAX) {
			(*lon) = NAN;
		} else {
			(*lon) = n2k_get_int32(n, 4) * 1E-7;
		}
	}
	return !(isnan(*lat) || isnan(*lon));
}

bool n2k_129026_values(const n2k_act_message *n, uint8_t *seq, uint8_t *mag, double *course, double *speed) {
	if (n->PGN != 129026 || !n->data || n->datalen < 8) { return false; }
	if (seq) { *seq = n2k_get_uint8(n, 0); }
	if (mag) { (*mag) = n2k_get_uint8(n, 1) & 0x03; }

	bool success = true;
	if (course) {
		int16_t c = n2k_get_int16(n, 2);
		if (c == INT16_MAX) {
			(*course) = NAN;
			success = false;
		} else {
			(*course) = c * N2K_TO_DEGREES;
		}
	}
	if (speed) {
		int16_t s = n2k_get_int16(n, 4);
		if (s == INT16_MAX) {
			(*speed) = NAN;
			success = false;
		} else {
			(*speed) = s * 0.01;
		}
	}
	return success;
}

bool n2k_130306_values(const n2k_act_message *n, uint8_t *seq, uint8_t *ref, double *speed, double *angle) {
	if (n->PGN != 130306 || !n->data || n->datalen < 8) { return false; }

	if (seq) { *seq = n2k_get_uint8(n, 0); }

	if (ref) { *ref = n2k_get_uint8(n, 5) & 0x07; }

	bool success = true;
	if (speed) {
		int16_t s = n2k_get_int16(n, 1);
		if (s == INT16_MAX) {
			(*speed) = NAN;
			success = false;
		} else {
			(*speed) = s * 0.01;
		}
	}

	if (angle) {
		int16_t a = n2k_get_int16(n, 3);
		if (a == INT16_MAX) {
			(*angle) = NAN;
			success = false;
		} else {
			(*angle) = a * N2K_TO_DEGREES;
		}
	}
	return success;
}

void n2k_127257_print(const n2k_act_message *n) {
	if (!n) { return; }

	uint8_t seq = 0;
	double yaw = 0;
	double pitch = 0;
	double roll = 0;
	bool s = n2k_127257_values(n, &seq, &yaw, &pitch, &roll);
	if (!s) { fprintf(stdout, "[!] "); }
	fprintf(stdout, "Pitch: %.3lf, Roll: %.3lf, Yaw: %.3lf. Seq. ID: %03d\n", pitch, roll, yaw, seq);
}

void n2k_129025_print(const n2k_act_message *n) {
	double lat = 0;
	double lon = 0;
	if (!n) { return; }
	if (!n2k_129025_values(n, &lat, &lon)) { fprintf(stdout, "[!] "); }
	fprintf(stdout, "GPS Position: %lf, %lf\n", lat, lon);
}

void n2k_129026_print(const n2k_act_message *n) {
	uint8_t seq = 0;
	uint8_t magnetic = 0;
	double course = 0;
	double speed = 0;

	if (!n2k_129026_values(n, &seq, &magnetic, &course, &speed)) { fprintf(stdout, "[!] "); }
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
	uint8_t seq = 0;
	double speed = 0;
	double angle = 0;
	uint8_t ref = 0;

	if (!n2k_130306_values(n, &seq, &ref, &speed, &angle)) { fprintf(stdout, "[!] "); }

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
