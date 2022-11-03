#include <stdio.h>
#include <stdlib.h>

#include "SELKIELoggerBase.h"

/*! @file SATests.c
 *
 * @brief String and String Array tests
 *
 * @test Creates 5 examples strings using str_new(), str_duplicate() and
 * str_update().  These strings are then added to an array and an additional
 * test string created in place.  Clearing a string array entry and moving an
 * array's contents are also tested.
 *
 * Ideally this test should also be checked with valgrind to ensure memory is
 * not being leaked through basic operations.
 *
 * @ingroup testing
 */

/*!
 * Create and destroy strings and string arrays
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns 0 (Pass), -1 (Fail)
 */
int main(int argc, char *argv[]) {
	string *exA = str_new(20, "Testing String Lib.");
	if (exA == NULL) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to create example string A\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	string *exB = str_new(2, "AA");
	if (exB == NULL) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to create example string B\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	string *exC = str_new(0, "");
	if (exC == NULL) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to create example string C\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	string *exD = str_duplicate(exC);
	if (exD == NULL) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to create example string D\n");
		return -1;
		//LCOV_EXCL_STOP
	}
	if (!str_copy(exD, exA)) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to copy example string A into example string E\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	string *exE = str_duplicate(exB);
	if (exE == NULL) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to create example string E\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	if (!str_update(exE, 50, "This is an updated test message for the Example E")) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to update string\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	strarray *SA = sa_new(6);
	if (SA == NULL) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to allocate string array\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	if (!sa_set_entry(SA, 0, exA)) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to set SA entry 0\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	if (!sa_set_entry(SA, 1, exB)) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to set SA entry 1\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	if (!sa_set_entry(SA, 2, exC)) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to set SA entry 2\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	if (!sa_set_entry(SA, 3, exD)) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to set SA entry 3\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	if (!sa_set_entry(SA, 4, exE)) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to set SA entry 4\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	if (!sa_create_entry(SA, 5, 35, "Creating string in place on array\n")) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to set SA entry 5\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	sa_clear_entry(SA, 3);
	if (SA->strings[3].data != NULL) {
		//LCOV_EXCL_START
		fprintf(stderr, "Failed to clear SA entry 3\n");
		return -1;
		//LCOV_EXCL_STOP
	}

	str_destroy(exA);
	str_destroy(exB);
	str_destroy(exC);
	str_destroy(exD);
	str_destroy(exE);
	free(exA);
	free(exB);
	free(exC);
	free(exD);
	free(exE);

	strarray *SA2 = sa_new(0);
	if (!sa_move(SA2, SA)) {
		//LCOV_EXCL_START
		fprintf(stderr, "Unable to move string array contents\n");
		return -1;
		//LCOV_EXCL_STOP
	}
	sa_destroy(SA);
	// Should be safe to call twice, so check we can!
	sa_destroy(SA);
	// Then test destruction of a malformed array
	SA->entries=1;
	sa_destroy(SA);

	sa_destroy(SA2);
	free(SA);
	free(SA2);
	fprintf(stdout, "All tests succeeded\n");
	return 0;
}
