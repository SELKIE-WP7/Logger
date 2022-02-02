#include <stdint.h>
#include <math.h>

#include "DWMessages.h"
#include "DWTypes.h"

uint16_t dw_hxv_cycdat(const dw_hxv *in) {
	return (in->data[0] << 8) + in->data[1];
}


int16_t dw_hxv_vertical(const dw_hxv *in) {
	// 12 bits, but stored left aligned in 2 and 3
	// High byte goes left 4 (left 8 then right 4), top nibble of low byte goes right 4
	int16_t vert =  ((in->data[2] & 0x8F) << 4) + ((in->data[3] & 0xF0) >> 4);
	if (in->data[2] & 0xF0) { vert *= -1;}
	return vert;
}

int16_t dw_hxv_north(const dw_hxv *in) {
	int16_t north = ((in->data[3] & 0x08) << 8) + in->data[4];
	if (in->data[3] & 0x0F) { north *= -1;}
	return north;
}

int16_t dw_hxv_west(const dw_hxv *in) {
	int16_t west =  ((in->data[5] & 0x8F) << 4) + ((in->data[6] & 0xF0) >> 4);
	if (in->data[5] & 0xF0) { west *= -1;}
	return west;
}

uint16_t dw_hxv_parity(const dw_hxv *in) {
	return ((in->data[6] & 0x0F) << 8) + in->data[7];
}

bool dw_spectrum_from_array(const uint16_t *arr, dw_spectrum *out) {
	if (arr == NULL || out == NULL) {
		return false;
	}

	out->sysseq = (arr[1] & 0xF000) >> 12;
	out->sysword = (arr[1] & 0x0FFF);

	if ( dw_spectral_block(arr, 0, out) &&
		dw_spectral_block(arr, 1, out) &&
		dw_spectral_block(arr, 2, out) &&
		dw_spectral_block(arr, 3, out)) {
		return true;
	}
	return false;
}

bool dw_spectral_block(const uint16_t *arr, const int ix, dw_spectrum *out) {
	out->frequencyBin[ix] = (arr[2 + 4 * ix] & 0x3F00) >> 8;
	if (out->frequencyBin[ix] > 63) {
		return false;
	}

	if (out->frequencyBin[ix] < 16) {
		out->frequency[ix] = 0.025 + out->frequencyBin[ix] * 0.005;
	} else {
		out->frequency[ix] = 0.11 + out->frequencyBin[ix] * 0.01;
	}
	out->direction[ix] = (arr[2 + 4 * ix] & 0x00FF) * 360.0 / 256.0;
	out->spread[ix] = 0.4476 * (((arr[4 + 4 * ix] & 0xFF00) >> 8) + ((arr[2 + 4 * ix] & 0xC000) >> 14) / 4.0);
	out->rpsd[ix] = expf(-((float) (arr[3 + 4 * ix] & 0x00FF) / 200.0));
	out->m2[ix] = ((arr[4 + 4 * ix] & 0x00FF) + ((arr[3 + 4 * ix] & 0xC000) >> 14) / 4.0 - 128) / 128;
	out->n2[ix] = (((arr[5 + 4 * ix] & 0xFF00) >> 8) + ((arr[3 + 4 * ix] & 0x3000) >> 12) / 4.0 - 128) / 128;
	out->checkFactor[ix] = (arr[5 + 4 * ix] & 0x00FF) / 100.0;
	return true;
}
