#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

#include "SELKIELoggerLPMS.h"

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
	size_t end = 0;
	while (processing || hw > 10) {
		if (feof(nf) && processing) {
			log_info(&state, 2, "End of file reached");
			processing = false;
		}
		lpms_message *m = calloc(1, sizeof(lpms_message));
		if (!m) {
			perror("lpms_message calloc");
			return EXIT_FAILURE;
		}
		bool r = lpms_readMessage_buf(fileno(nf), m, buf, &end, &hw);
		if (r) {
			uint16_t cs = 0;
			bool OK = (lpms_checksum(m, &cs) && cs == m->checksum);
			log_info(&state, 2,
			         "%02x: Command %02x, %u bytes, checksum %04x/%04x (%s)", m->id,
			         m->command, m->length, m->checksum, cs, OK ? "OK" : "not OK");
			count++;
		} else {
			if (m->id == 0xAA || m->id == 0xEE) {
				log_error(&state,
				          "Error reading messages from file (Code: 0x%02x)\n",
				          (uint8_t)m->id);
			}

			if (m->id == 0xFD) {
				processing = false;
				if (end < hw) { end++; }
			}
		}

		if (m) {
			if (m->data) { free(m->data); }
			free(m);
			m = NULL;
		}
	}
	log_info(&state, 0, "%d messages successfully read from file", count);
	fclose(nf);
	return 0;
}
