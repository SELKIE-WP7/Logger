#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

#include "version.h"

/*!
 * @file
 * @brief Print messages in a .dat file
 * @ingroup Executables
 */

/*!
 * Reads messages from a SELKIELogger data file and prints out a simple
 * representation.
 *
 * Does not attempt to use friendly names for sources or channels.
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns -1 on error, otherwise 0
 */
int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	char *usage = "Usage: %1$s [-v] datfile\n"
		      "\nVersion: " GIT_VERSION_STRING "\n";

	opterr = 0; // Handle errors ourselves
	int go = 0;
	bool doUsage = false;
	while ((go = getopt(argc, argv, "v")) != -1) {
		switch (go) {
			case 'v':
				state.verbose++;
				break;
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
		destroy_program_state(&state);
		return -1;
	}

	char *inFileName = strdup(argv[optind]);
	FILE *inFile = fopen(inFileName, "rb");
	if (inFile == NULL) {
		log_error(&state, "Unable to open input file");
		free(inFileName);
		destroy_program_state(&state);
		return -1;
	}

	state.started = 1;
	int msgCount = 0;
	while (!(feof(inFile))) {
		// Read message from data file
		msg_t tmp = {0};
		if (!mp_readMessage(fileno(inFile), &tmp)) {
			if (tmp.data.value == 0xAA || tmp.data.value == 0xEE) {
				log_error(&state,
				          "Error reading messages from file (Code: 0x%52x)\n",
				          (uint8_t)tmp.data.value);
			}
			if (tmp.data.value == 0xFD) {
				// No more data, exit cleanly
				log_info(&state, 1, "End of file reached");
			}
			break;
		}

		msgCount++;
		if (tmp.type >= 0x03 || state.verbose > 1) {
			char *msgstring = msg_to_string(&tmp);
			if (msgstring) { fprintf(stdout, "%s\n", msgstring); }
			free(msgstring);
		}
		msg_destroy(&tmp);
	}

	log_info(&state, 1, "%d messages processed", msgCount);
	free(inFileName);
	fclose(inFile);
	return 0;
}
