#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

#include "version.h"

/*!
 * @file
 * @brief Extract subset of messages from .dat file
 * @ingroup Executables
 */

/*!
 * Writes a new .dat file containing only messages with a specific source ID.
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns -1 on error, otherwise 0
 */
int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	char *outFileName = NULL;
	bool clobberOutput = false;
	uint8_t source = 0;
	bool type[255] = {0};
	bool raw = false;

	char *usage =
		"Usage: %1$s [-v] [-q] [-f] [-r] [-o outfile] -S source [-C channel [-C channel ...]] DATFILE\n"
		"\t-v\tIncrease verbosity\n"
		"\t-q\tDecrease verbosity\n"
		"\t-f\tOverwrite existing output files\n"
		"\t-r\tWrite raw data (No message formatting)\n"
		"\t-S\tSource number to extract\n"
		"\t-T\tMessage type(s) to extract\n"
		"\t-o\tWrite output to named file\n"
		"\nVersion: " GIT_VERSION_STRING "\n"
		"\nOutput file name will be generated based on input file name, unless set by -o option\n";

	opterr = 0; // Handle errors ourselves
	int go = 0;
	bool doUsage = false;
	uint8_t tmp = 0;
	uint8_t typeCount = 0;
	while ((go = getopt(argc, argv, "vqfro:S:C:")) != -1) {
		switch (go) {
			case 'v':
				state.verbose++;
				break;
			case 'q':
				state.verbose--;
				break;
			case 'f':
				clobberOutput = true;
				break;
			case 'r':
				raw = true;
				break;
			case 'S':
				source = strtol(optarg, NULL, 0);
				if (source < 2 || source > 128) {
					log_error(&state, "Invalid source requested (%s)", optarg);
					doUsage = true;
				}
				break;
			case 'C':
				tmp = strtol(optarg, NULL, 0);
				if (tmp < 1 || tmp >= 128) {
					log_error(&state, "Invalid message type requested (%s)",
					          optarg);
					doUsage = true;
				}
				type[tmp] = true;
				typeCount++;
				break;

			case 'o':
				if (outFileName) {
					log_error(
						&state,
						"Only a single output file name can be provided");
					doUsage = true;
				} else {
					outFileName = strdup(optarg);
				}
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
		free(outFileName);
		return -1;
	}

	char *inFileName = strdup(argv[optind]);
	FILE *inFile = fopen(inFileName, "rb");
	if (inFile == NULL) {
		log_error(&state, "Unable to open input file");
		free(inFileName);
		free(outFileName);
		destroy_program_state(&state);
		return -1;
	}

	if (raw && typeCount != 1) {
		log_warning(&state, "Raw mode requested without a message type filter");
		log_warning(&state,
		            "Different message types will not be distinguished in output file");
	}

	if (outFileName == NULL) {
		// Work out output file name
		// Split into base and dirnames so that we're don't accidentally split the
		// path on a .
		char *inF1 = strdup(inFileName);
		char *inF2 = strdup(inFileName);
		char *dn = dirname(inF1);
		char *bn = basename(inF2);

		// Find last . in file name, if any
		char *ep = strrchr(bn, '.');

		// If no ., use full basename length, else use length to .
		int bnl = 0;
		if (ep == NULL) {
			bnl = strlen(bn);
		} else {
			bnl = ep - bn;
		}
		// New basename is old basename up to . (or end, if absent)
		char *nbn = calloc(bnl + 1, sizeof(char));
		strncpy(nbn, bn, bnl);
		if (asprintf(&outFileName, "%s/%s.s%02x.dat", dn, nbn, source) <= 0) {
			outFileName = NULL;
		}
		free(nbn);
		free(inF1);
		free(inF2);
		if (outFileName == NULL) {
			log_error(&state, "Unable to generate output file name");
			free(inFileName);
			return -1;
		}
	}

	errno = 0;
	char fmode[4] = {'w', 'b', 0};

	if (!clobberOutput) {
		// wbx7: Create, binary, comp. level 7
		fmode[2] = 'x';
	}

	FILE *outFile = fopen(outFileName, fmode);
	if (outFile == NULL) {
		log_error(&state, "Unable to open output file");
		log_error(&state, "%s", strerror(errno));
		fclose(inFile);
		free(inFileName);
		free(outFileName);
		destroy_program_state(&state);
		return -1;
	}
	log_info(&state, 1, "Writing messages from source 0x%02x to %s", source, outFileName);
	log_info(&state, 1, "Raw mode %s", raw ? "enabled" : "disabled");
	if (typeCount > 0) {
		log_info(&state, 2, "Filtering for %d message types", typeCount);
		for (int i = 0; i < 128; i++) {
			if (type[i]) { log_info(&state, 3, "Message type 0x%02x enabled", i); }
		}
	}
	free(outFileName);
	outFileName = NULL;

	state.started = 1;
	int msgCount = 0;
	struct stat inStat = {0};
	if (fstat(fileno(inFile), &inStat) != 0) {
		log_error(&state, "Unable to get input file status: %s", strerror(errno));
		free(inFileName);
		fclose(inFile);
		fclose(outFile);
		destroy_program_state(&state);
		return -1;
	}
	long inSize = inStat.st_size;
	long inPos = 0;
	int progress = 0;
	if (inSize >= 1E9) {
		log_info(&state, 1, "Reading %.2fGB of data from %s", inSize / 1.0E9, inFileName);
	} else if (inSize >= 1E6) {
		log_info(&state, 1, "Reading %.2fMB of data from %s", inSize / 1.0E6, inFileName);
	} else if (inSize >= 1E3) {
		log_info(&state, 1, "Reading %.2fkB of data from %s", inSize / 1.0E3, inFileName);
	} else {
		log_info(&state, 1, "Reading %ld bytes of data from %s", inSize, inFileName);
	}

	free(inFileName);

	while (!(feof(inFile))) {
		// Read message from data file
		msg_t mtmp = {0};
		if (!mp_readMessage(fileno(inFile), &mtmp)) {
			if (mtmp.data.value == 0xAA || mtmp.data.value == 0xEE) {
				log_error(&state,
				          "Error reading messages from file (Code: 0x%52x)\n",
				          (uint8_t)mtmp.data.value);
			}
			if (mtmp.data.value == 0xFD) {
				// No more data, exit cleanly
				log_info(&state, 1, "End of file reached");
			}
			break;
		}
		if (mtmp.source == source) {
			bool writeMsg = true;
			if (typeCount > 0) { writeMsg = type[mtmp.type]; }

			if (writeMsg) {
				if (raw) {
					if (!mp_writeData(fileno(outFile), &mtmp)) {
						log_error(&state, "Unable to write output: %s",
						          strerror(errno));
						fclose(inFile);
						fclose(outFile);
						destroy_program_state(&state);
						return -1;
					}
				} else {
					if (!mp_writeMessage(fileno(outFile), &mtmp)) {
						log_error(&state, "Unable to write output: %s",
						          strerror(errno));
						fclose(inFile);
						fclose(outFile);
						destroy_program_state(&state);
						return -1;
					}
				}
				msgCount++;
			}
		}
		msg_destroy(&mtmp);
		inPos = ftell(inFile);
		if (((((1.0 * inPos) / inSize) * 100) - progress) >= 5) {
			progress = (((1.0 * inPos) / inSize) * 100);
			log_info(&state, 2, "Progress: %d%% (%ld / %ld)", progress, inPos, inSize);
		}
	}
	fclose(inFile);
	fclose(outFile);

	log_info(&state, 1, "%d messages processed", msgCount);
	destroy_program_state(&state);
	return 0;
}
