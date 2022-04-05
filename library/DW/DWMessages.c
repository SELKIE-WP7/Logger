#include <math.h>
#include <stdint.h>

#include "DWMessages.h"
#include "DWTypes.h"

uint16_t dw_hxv_cycdat(const dw_hxv *in) {
	return (in->data[0] << 8) + in->data[1];
}

int16_t dw_hxv_vertical(const dw_hxv *in) {
	// 12 bits, but stored left aligned in 2 and 3
	// High byte goes left 4 (left 8 then right 4)
	// Top nibble of low byte goes right
	int16_t vert = ((in->data[2] & 0x7F) << 4) + ((in->data[3] & 0xF0) >> 4);
	if (in->data[2] & 0x80) { vert *= -1; }
	return vert;
}

int16_t dw_hxv_north(const dw_hxv *in) {
	int16_t north = ((in->data[3] & 0x07) << 8) + in->data[4];
	if (in->data[3] & 0x08) { north *= -1; }
	return north;
}

int16_t dw_hxv_west(const dw_hxv *in) {
	int16_t west = ((in->data[5] & 0x7F) << 4) + ((in->data[6] & 0xF0) >> 4);
	if (in->data[5] & 0x80) { west *= -1; }
	return west;
}

uint16_t dw_hxv_parity(const dw_hxv *in) {
	return ((in->data[6] & 0x0F) << 8) + in->data[7];
}

bool dw_spectrum_from_array(const uint16_t *arr, dw_spectrum *out) {
	if (arr == NULL || out == NULL) { return false; }

	out->sysseq = (arr[1] & 0xF000) >> 12;
	out->sysword = (arr[1] & 0x0FFF);

	if (dw_spectral_block(arr, 0, out) && dw_spectral_block(arr, 1, out) && dw_spectral_block(arr, 2, out) &&
	    dw_spectral_block(arr, 3, out)) {
		return true;
	}
	return false;
}

bool dw_spectral_block(const uint16_t *arr, const int ix, dw_spectrum *out) {
	if (ix < 0 || ix > 3) { return false; }

	out->frequencyBin[ix] = (arr[2 + 4 * ix] & 0x3F00) >> 8;
	if (out->frequencyBin[ix] > 63) { return false; }

	if (out->frequencyBin[ix] < 16) {
		out->frequency[ix] = 0.025 + out->frequencyBin[ix] * 0.005;
	} else {
		out->frequency[ix] = 0.11 + out->frequencyBin[ix] * 0.01;
	}
	out->direction[ix] = (arr[2 + 4 * ix] & 0x00FF) * 360.0 / 256.0;
	out->spread[ix] = 0.4476 * (((arr[4 + 4 * ix] & 0xFF00) >> 8) + ((arr[2 + 4 * ix] & 0xC000) >> 14) / 4.0);
	out->rpsd[ix] = expf(-((float)(arr[3 + 4 * ix] & 0x00FF) / 200.0));
	out->m2[ix] = ((arr[4 + 4 * ix] & 0x00FF) + ((arr[3 + 4 * ix] & 0xC000) >> 14) / 4.0 - 128);
	out->m2[ix] /= 128;

	out->n2[ix] = (((arr[5 + 4 * ix] & 0xFF00) >> 8) + ((arr[3 + 4 * ix] & 0x3000) >> 12) / 4.0 - 128);
	out->n2[ix] /= 128;

	out->K[ix] = (arr[5 + 4 * ix] & 0x00FF) / 100.0;
	return true;
}

bool dw_system_from_array(const uint16_t *arr, dw_system *out) {
	if (arr == NULL || out == NULL) { return false; }

	out->number = (arr[0] & 0x0007) + 1;
	out->GPSfix = (arr[0] & 0x0010);
	out->Hrms = (arr[1] & 0x0FFF) / 400.0;
	out->fzero = (arr[2] & 0x0FFF) / 400.0;
	out->PSD = 5000 * exp(-(arr[3] & 0x0FFF) / 200.0);
	out->refTemp = (arr[4] & 0x01FF) / 20.0 - 5;
	out->waterTemp = (arr[5] & 0x01FF) / 20.0 - 5;
	out->opTime = (arr[6] & 0x0FF0) >> 4;
	out->battStatus = (arr[6] & 0x0007);
	out->a_z_off = (arr[7] & 0x07FF) / 800.0 * ((arr[7] & 0x0800) ? -1 : 1);
	out->a_x_off = (arr[8] & 0x07FF) / 800.0 * ((arr[8] & 0x0800) ? -1 : 1);
	out->a_y_off = (arr[9] & 0x07FF) / 800.0 * ((arr[9] & 0x0800) ? -1 : 1);
	out->lat = ((arr[10] & 0x0800) ? -1 : 1) * 90.0 * ((arr[11] & 0x0FFF) + ((uint32_t)(arr[10] & 0x07FF) << 12)) /
	           (2 << 22);
	out->lon = ((arr[12] & 0x0800) ? -1 : 1) * 180.0 * ((arr[13] & 0x0FFF) + ((uint32_t)(arr[12] & 0x07FF) << 12)) /
	           (2 << 22);
	out->orient = (arr[14] & 0x0FF) * 360.0 / 256.0;
	out->incl = (90.0 / 128.0) * ((arr[15] & 0x00FF) - 128 + ((arr[15] & 0x0F00) >> 8) / 16);
	return true;
}
