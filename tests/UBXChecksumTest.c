#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "SELKIELoggerGPS.h"

/*! @file UBXChecksumTest.c
 *
 * @brief Test UBX Checksum functions
 *
 * Checks that the checksum functions correctly recognise valid and invalid
 * checksums, and that updating an invalid message checksum results in a new
 * and correct checksum.
 *
 * Sample values used have been verified against hardware results.
 *
 * @ingroup testing
 */
int main(int argc, char *argv[]) {
	bool passed = true;

	ubx_message validCS = {0xB5, 0x62, 0x06, 0x02, 0x0001, {0x01}, 0x0A, 0x2A};
	ubx_message invalidCS = {0xB5, 0x62, 0x06, 0x02, 0x0001, {0x01}, 0xFF, 0xFF};

	char *hex = ubx_string_hex(&validCS);
	if (ubx_check_checksum(&validCS) == false) {
		fprintf(stderr, "[Failed] %s\n", hex);
		fprintf(stderr, "[Error] Valid checksum failed test\n");
		passed = false;
	} else {
		printf("[Pass] Valid checksum: %s\n", hex);
	}
	free(hex);
	hex = NULL;

	hex = ubx_string_hex(&invalidCS);
	if (ubx_check_checksum(&invalidCS) == true) {
		fprintf(stderr, "[Failed] %s\n", hex);
		fprintf(stderr, "[Error] Invalid checksum passed test\n");
		passed = false;
	} else {
		printf("[Pass] Invalid checksum: %s\n", hex);
	}
	free(hex);
	hex = NULL;

	ubx_set_checksum(&invalidCS);

	hex = ubx_string_hex(&invalidCS);
	if (ubx_check_checksum(&invalidCS) == false) {
		fprintf(stderr, "[Failed] %s\n", hex);
		fprintf(stderr, "[Error] Corrected checksum failed test\n");
		passed = false;
	} else {
		printf("[Pass] Corrected checksum: %s\n", hex);
	}
	free(hex);
	hex = NULL;

	if (passed) { return 0; }

	return -1;
}
