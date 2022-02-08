#include <stdio.h>
#include <stdlib.h>

#include "SELKIELoggerBase.h"

/*! @file SATests.c
 *
 * @brief String and String Array tests
 *
 * Creates 5 examples strings using str_new(), str_duplicate() and
 * str_update().
 *
 * These strings are then added to an array and an additional test string
 * created in place.
 *
 * Clearing a string array entry and moving an array's contents are also tested.
 *
 * Ideally this test should also be checked with valgrind to ensure memory is
 * not being leaked through basic operations.
 *
 * @ingroup testing
 */
int main(int argc, char *argv[]) {
	string *exA = str_new(20, "Testing String Lib.");
	if (exA == NULL) {
		fprintf(stderr, "Failed to create example string A\n");
		return EXIT_FAILURE;
	}

	string *exB = str_new(2, "AA");
	if (exB == NULL) {
		fprintf(stderr, "Failed to create example string B\n");
		return EXIT_FAILURE;
	}

	string *exC = str_new(0, "");
	if (exC == NULL) {
		fprintf(stderr, "Failed to create example string C\n");
		return EXIT_FAILURE;
	}

	string *exD = str_duplicate(exC);
	if (exD == NULL) {
		fprintf(stderr, "Failed to create example string D\n");
		return EXIT_FAILURE;
	}
	if (!str_copy(exD, exA)) {
		fprintf(stderr, "Failed to copy example string A into example string E\n");
		return EXIT_FAILURE;
	}

	string *exE = str_duplicate(exB);
	if (exE == NULL) {
		fprintf(stderr, "Failed to create example string E\n");
		return EXIT_FAILURE;
	}

	if (!str_update(exE, 50, "This is an updated test message for the Example E")) {
		fprintf(stderr, "Failed to update string\n");
		return EXIT_FAILURE;
	}

	strarray *SA = sa_new(6);
	if (SA == NULL) {
		fprintf(stderr, "Failed to allocate string array\n");
		return EXIT_FAILURE;
	}

	if (!sa_set_entry(SA, 0, exA)) {
		fprintf(stderr, "Failed to set SA entry 0\n");
		return EXIT_FAILURE;
	}

	if (!sa_set_entry(SA, 1, exB)) {
		fprintf(stderr, "Failed to set SA entry 1\n");
		return EXIT_FAILURE;
	}

	if (!sa_set_entry(SA, 2, exC)) {
		fprintf(stderr, "Failed to set SA entry 2\n");
		return EXIT_FAILURE;
	}

	if (!sa_set_entry(SA, 3, exD)) {
		fprintf(stderr, "Failed to set SA entry 3\n");
		return EXIT_FAILURE;
	}

	if (!sa_set_entry(SA, 4, exE)) {
		fprintf(stderr, "Failed to set SA entry 4\n");
		return EXIT_FAILURE;
	}

	if (!sa_create_entry(SA, 5, 35, "Creating string in place on array\n")) {
		fprintf(stderr, "Failed to set SA entry 5\n");
		return EXIT_FAILURE;
	}

	sa_clear_entry(SA, 3);
	if (SA->strings[3].data != NULL) {
		fprintf(stderr, "Failed to clear SA entry 3\n");
		return EXIT_FAILURE;
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
		fprintf(stderr, "Unable to move string array contents\n");
		return EXIT_FAILURE;
	}
	sa_destroy(SA);
	sa_destroy(SA2);
	free(SA);
	free(SA2);
	fprintf(stdout, "All tests succeeded\n");
	return EXIT_SUCCESS;
}
