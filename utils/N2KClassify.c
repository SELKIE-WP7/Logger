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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

#include "SELKIELoggerN2K.h"

#include "version.h"
/*!
 * @file
 * @brief Read N2K messages and summarise
 * @ingroup Executables
 */

//! Largest possible PGN value
#define PGN_MAX 16777216

//! Allocated read buffer size
#define BUFSIZE 1024

/*!
 * Count of messages seen by PGN number
 *
 * Could be smarter, but it's a relatively small amount of memory in the grand
 * scheme of things.
 */
int msgCount[PGN_MAX];

/*!
 * Reads messages from file, and prints the number seen for each PGN number.
 * Zero counts are suppressed.
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns -1 on error, otherwise 0
 */
int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	char *usage = "Usage: %1$s datfile\n"
		      "\nVersion: " GIT_VERSION_STRING "\n";

	opterr = 0; // Handle errors ourselves
	int go = 0;
	bool doUsage = false;
	while ((go = getopt(argc, argv, "")) != -1) {
		switch (go) {
			case '?':
				log_error(&state, "Unknown option `-%c'", optopt);
				doUsage = true;
		}
	}

	// Should be 1 spare arguments: The file to convert
	if (argc - optind != 1) {
		log_error(&state, "Invalid arguments");
		doUsage = true;
	}

	if (doUsage) {
		fprintf(stderr, usage, argv[0]);
		return -1;
	}

	FILE *nf = fopen(argv[optind], "r");

	if (nf == NULL) {
		log_error(&state, "Unable to open input file \"%s\"", argv[optind]);
		return -1;
	}

	state.started = true;
	bool processing = true;

	uint8_t buf[BUFSIZE] = {0};
	size_t hw = 0;
	int count = 0;
	while (processing || hw > 18) {
		if (processing && (hw < BUFSIZE)) {
			ssize_t ret = fread(&(buf[hw]), sizeof(uint8_t), BUFSIZE - hw, nf);
			if (ret < 0) {
				log_error(&state, "Unable to read data from input");
				processing = false;
			} else {
				hw += ret;
			}
		}
		if (feof(nf)) {
			log_info(&state, 2, "End of file reached");
			processing = false;
		}
		n2k_act_message *nm = NULL;
		size_t end = 0;
		bool r = n2k_act_from_bytes(buf, hw, &nm, &end, (state.verbose > 2));
		if (r) {
			msgCount[nm->PGN]++;
			count++;
		} else {
			if (!processing && end == 0) {
				// If we're not making progress and we're at the end of file then
				// this must be a bad message Force the process to move onwards
				end = 1;
			}
		}

		if (nm) {
			if (nm->data) { free(nm->data); }
			free(nm);
			nm = NULL;
		}

		if ((hw - end) > 0) {
			memmove(buf, &(buf[end]), hw - end);
			hw -= end;
		} else {
			hw = 0;
			end = 0;
		}

		memset(&(buf[hw]), 0, BUFSIZE - hw);
	}
	log_info(&state, 0, "%d messages successfully read from file", count);
	fprintf(stdout, "\nPGN   \tCount\n");
	for (int i = 0; i < PGN_MAX; i++) {
		if (msgCount[i] > 0) { fprintf(stdout, "%6d\t%6d\n", i, msgCount[i]); }
	}
	fclose(nf);
	return 0;
}
