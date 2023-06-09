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
#include <stdio.h>
#include <stdlib.h>

#include "SELKIELoggerGPS.h"

/*! @file UBXChecksumTest.c
 *
 * @brief Test UBX Checksum functions
 *
 * @test Check that correct and incorrect UBX checksums are recognised, and
 * that incorrect checksums can be corrected. Sample values used have been
 * verified against hardware results.
 *
 * @ingroup testing
 */

/*!
 * Check UBX message checksumming
 *
 * @returns 0 (Pass), -1 (Fail)
 */
int main(void) {
	bool passed = true;

	ubx_message validCS = {0xB5, 0x62, 0x06, 0x02, 0x0001, {0x01}, 0x0A, 0x2A, 0x00};
	ubx_message invalidCS = {0xB5, 0x62, 0x06, 0x02, 0x0001, {0x01}, 0xFF, 0xFF, 0x00};

	char *hex = ubx_string_hex(&validCS);
	if (ubx_check_checksum(&validCS) == false) {
		// LCOV_EXCL_START
		fprintf(stderr, "[Failed] %s\n", hex);
		fprintf(stderr, "[Error] Valid checksum failed test\n");
		passed = false;
		// LCOV_EXCL_STOP
	} else {
		printf("[Pass] Valid checksum: %s\n", hex);
	}
	free(hex);
	hex = NULL;

	hex = ubx_string_hex(&invalidCS);
	if (ubx_check_checksum(&invalidCS) == true) {
		// LCOV_EXCL_START
		fprintf(stderr, "[Failed] %s\n", hex);
		fprintf(stderr, "[Error] Invalid checksum passed test\n");
		passed = false;
		// LCOV_EXCL_STOP
	} else {
		printf("[Pass] Invalid checksum: %s\n", hex);
	}
	free(hex);
	hex = NULL;

	ubx_set_checksum(&invalidCS);

	hex = ubx_string_hex(&invalidCS);
	if (ubx_check_checksum(&invalidCS) == false) {
		// LCOV_EXCL_START
		fprintf(stderr, "[Failed] %s\n", hex);
		fprintf(stderr, "[Error] Corrected checksum failed test\n");
		passed = false;
		// LCOV_EXCL_STOP
	} else {
		printf("[Pass] Corrected checksum: %s\n", hex);
	}
	free(hex);
	hex = NULL;

	if (passed) { return 0; }

	return -1;
}
