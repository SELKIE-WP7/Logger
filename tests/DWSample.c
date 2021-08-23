#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "DWTypes.h"
#include "DWMessages.h"

bool test(const char *in) {
	dw_hxv thxv = {0};
	size_t slen = strlen(in);
	bool rv =  dw_string_hxv(in, &slen, &thxv);

	uint16_t cycdat = dw_hxv_cycdat(&thxv);
	int16_t north = dw_hxv_north(&thxv);
	int16_t west = dw_hxv_west(&thxv);
	int16_t vert = dw_hxv_vertical(&thxv);
	uint16_t parity = dw_hxv_parity(&thxv);


	fprintf(stdout, "Line 0x%02x: Status %02d, North: %+5.2fm, West: %+5.2fm, Vertical: %5.2fm.\n", thxv.lines, thxv.status, north/100.0, west/100.0, vert/100.0);
	fprintf(stdout, "\tCyclic data: 0x%04x. Parity bytes: 0x%04x\n", cycdat, parity);
	return rv;
}

int main(int argc, char *argv[]) {
	const char *l1 = "0618,B34D,8EE9,2DE4,2F4C\r";
	const char *l2 = "0424,4CBA,2FC8,2F84,F09E\r";
	const char *l3 = "001E,7FFF,80E0,0300,1689\r";

	bool rv = true;
	rv &= test(l1);
	rv &= test(l2);
	rv &= test(l3);

	return rv ? 0 : 1;

}
