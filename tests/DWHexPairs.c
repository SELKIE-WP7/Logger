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

#include "DWTypes.h"

/*! @file
 *
 * @brief Test conversion of hexadecimal character pairs to integers
 *
 * @test Pass a selection of known strings to hexpair_to_int() and check that the correct value is
 * returned
 *
 * @ingroup testing
 */

bool test(const char *in, const uint8_t target);

/*!
 * Test hexpair_to_uint() for CTest
 *
 * @returns 1 on error, otherwise 0
 */
int main(void) {
	const char *ts1 = "00";
	const char *ts2 = "01";
	const char *ts3 = "0a";
	const char *ts4 = "0A";
	const char *ts5 = "10";
	const char *ts6 = "ff";
	const char *ts7 = "FF";

	bool res = true;

	res &= test(ts1, 0);
	res &= test(ts2, 1);
	res &= test(ts3, 10);
	res &= test(ts4, 10);
	res &= test(ts5, 16);
	res &= test(ts6, 255);
	res &= test(ts7, 255);

	return (res ? 0 : 1);
}

/*!
 * Confirm characters passed as input convert to target value.
 * @param[in] in Hexadecimal character pair
 * @param[in] target Target integer value
 * @returns True on success, false on failure
 */
bool test(const char *in, const uint8_t target) {
	uint8_t val = 0;
	bool res = hexpair_to_uint(in, &val);
	if (!res) {
		// LCOV_EXCL_START
		fprintf(stderr, "Conversion of '%c%c' failed\n", in[0], in[1]);
		return false;
		// LCOV_EXCL_STOP
	}

	if (val == target) {
		fprintf(stdout, "Conversion of '%c%c' successfully yielded %d\n", in[0], in[1],
		        target);
		return true;
	}
	// LCOV_EXCL_START
	fprintf(stdout, "Conversion of '%c%c' failed (returned %d, not %d)\n", in[0], in[1], val,
	        target);
	return false;
	// LCOV_EXCL_STOP
}
