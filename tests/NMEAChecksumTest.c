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

#include "SELKIELoggerNMEA.h"

/*! @file NMEAChecksumTest.c
 *
 * @brief Test NMEA Checksum functions
 *
 * @test Check that correct and incorrect NMEA checksums are recognised, and
 * that incorrect checksums can be corrected. Sample values used have been
 * verified against hardware results.
 *
 * @ingroup testing
 */

/*!
 * Check NMEA message checksumming
 *
 * @returns 0 (Pass), -1 (Fail)
 */
int main(void) {
	bool passed = true;

	//$IIXDR,C,,C,ENV_WATER_T,C,16.24,C,ENV_OUTAIR_T,P,101800,P,ENV_ATMOS_P*61
	nmea_msg_t validCS = {.encapsulated = false,
	                      .talker = {'I', 'I'},
	                      .message = {'X', 'D', 'R'},
	                      .rawlen = 62,
	                      .raw = {0x43, 0x2c, 0x2c, 0x43, 0x2c, 0x45, 0x4e, 0x56, 0x5f,
	                              0x57, 0x41, 0x54, 0x45, 0x52, 0x5f, 0x54, 0x2c, 0x43,
	                              0x2c, 0x31, 0x36, 0x2e, 0x32, 0x34, 0x2c, 0x43, 0x2c,
	                              0x45, 0x4e, 0x56, 0x5f, 0x4f, 0x55, 0x54, 0x41, 0x49,
	                              0x52, 0x5f, 0x54, 0x2c, 0x50, 0x2c, 0x31, 0x30, 0x31,
	                              0x38, 0x30, 0x30, 0x2c, 0x50, 0x2c, 0x45, 0x4e, 0x56,
	                              0x5f, 0x41, 0x54, 0x4d, 0x4f, 0x53, 0x5f, 0x50}};
	validCS.checksum = 0x61;

	nmea_msg_t invalidCS = {.encapsulated = false,
	                        .talker = "II",
	                        .message = "XDR",
	                        .rawlen = 62,
	                        .raw = {0x43, 0x2c, 0x2c, 0x43, 0x2c, 0x45, 0x4e, 0x56, 0x5f,
	                                0x57, 0x41, 0x54, 0x45, 0x52, 0x5f, 0x54, 0x2c, 0x43,
	                                0x2c, 0x31, 0x36, 0x2e, 0x32, 0x34, 0x2c, 0x43, 0x2c,
	                                0x45, 0x4e, 0x56, 0x5f, 0x4f, 0x55, 0x54, 0x41, 0x49,
	                                0x52, 0x5f, 0x54, 0x2c, 0x50, 0x2c, 0x31, 0x30, 0x31,
	                                0x38, 0x30, 0x30, 0x2c, 0x50, 0x2c, 0x45, 0x4e, 0x56,
	                                0x5f, 0x41, 0x54, 0x4d, 0x4f, 0x53, 0x5f, 0x50}};
	invalidCS.checksum = 0xFF;

	char *hex = nmea_string_hex(&validCS);
	if (nmea_check_checksum(&validCS) == false) {
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

	hex = nmea_string_hex(&invalidCS);
	if (nmea_check_checksum(&invalidCS) == true) {
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

	nmea_set_checksum(&invalidCS);

	hex = nmea_string_hex(&invalidCS);
	if (nmea_check_checksum(&invalidCS) == false) {
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
