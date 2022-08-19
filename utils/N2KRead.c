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
 * @brief Read and print N2K messages
 * @ingroup Executables
 */

//! Allocated read buffer size
#define BUFSIZE 1024

/*!
 * Read messages from file and print details to screen.
 *
 * Detail printed depends on whether structure of message is known, although
 * brief data can be printed for all messages at higher verbosities.
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns -1 on error, otherwise 0
 */
int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	char *usage = "Usage: %1$s [-v] [-q] datfile\n"
		      "\t-v\tIncrease verbosity\n"
		      "\t-q\tDecrease verbosity\n"
		      "\nVersion: " GIT_VERSION_STRING "\n";

	opterr = 0; // Handle errors ourselves
	int go = 0;
	bool doUsage = false;
	while ((go = getopt(argc, argv, "vq")) != -1) {
		switch (go) {
			case 'v':
				state.verbose++;
				break;
			case 'q':
				state.verbose--;
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
			log_info(&state, 2, "%d=>%d: PGN %d, Priority %d", nm->src, nm->dst,
			         nm->PGN, nm->priority);
			switch (nm->PGN) {
				case 60928:
					n2k_60928_print(nm);
					break;
				case 127250:
					n2k_127250_print(nm);
					break;
				case 127251:
					n2k_127251_print(nm);
					break;
				case 127257:
					n2k_127257_print(nm);
					break;
				case 128267:
					n2k_128267_print(nm);
					break;
				case 129025:
					n2k_129025_print(nm);
					break;
				case 129026:
					n2k_129026_print(nm);
					break;
				case 129033:
					n2k_129033_print(nm);
					break;
				case 130306:
					n2k_130306_print(nm);
					break;
			}
			count++;
		} else {
			log_info(&state, 3, "%zu bytes read, failed to decode message", end);
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
	fclose(nf);
	return 0;
}
