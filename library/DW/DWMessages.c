#include <stdint.h>
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

