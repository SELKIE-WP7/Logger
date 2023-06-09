/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "DWMessages.h"
#include "DWTypes.h"

/*! @file
 *
 * @brief Test conversion of HXV format data
 *
 * @test Pass example HXV strings to dw_string_hxv() and check for failure
 *
 * @ingroup testing
 */

bool test(const char *in);

/*!
 * Test dw_string_hxv() for CTest
 *
 * @returns 1 on error, otherwise 0
 *
 */
int main(void) {
	const char *l1 = "0618,B34D,8EE9,2DE4,2F4C\r";
	const char *l2 = "0424,4CBA,2FC8,2F84,F09E\r";
	const char *l3 = "001E,7FFF,80E0,0300,1689\r";

	bool rv = true;
	rv &= test(l1);
	rv &= test(l2);
	rv &= test(l3);

	return rv ? 0 : 1;
}

/*!
 * Convert and print HXV data
 *
 * @param[in] in HXV data as string
 * @returns True on success, false on failure
 */
bool test(const char *in) {
	dw_hxv thxv = {0};
	size_t slen = strlen(in);
	bool rv = dw_string_hxv(in, &slen, &thxv);

	uint16_t cycdat = dw_hxv_cycdat(&thxv);
	int16_t north = dw_hxv_north(&thxv);
	int16_t west = dw_hxv_west(&thxv);
	int16_t vert = dw_hxv_vertical(&thxv);
	uint16_t parity = dw_hxv_parity(&thxv);

	fprintf(stdout,
	        "Line 0x%02x: Status %02d, North: %+5.2fm, West: %+5.2fm, Vertical: "
	        "%5.2fm.\n",
	        thxv.lines, thxv.status, north / 100.0, west / 100.0, vert / 100.0);
	fprintf(stdout, "\tCyclic data: 0x%04x. Parity bytes: 0x%04x\n", cycdat, parity);
	return rv;
}
