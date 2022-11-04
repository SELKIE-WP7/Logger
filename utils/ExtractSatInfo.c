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
 * @brief Extract satellite information from .dat file
 * @ingroup Executables
 */

/*!
 * Parses raw UBX data included for NAV-SAT messages and reports basic
 * information on each satellite detected.
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns -1 on error, otherwise 0
 */
int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	char *outFileName = NULL;
	bool doGZ = true;
	bool clobberOutput = false;

	char *usage =
		"Usage: %1$s [-v] [-q] [-f] [-Z]  [-o outfile] DATFILE\n"
		"\t-v\tIncrease verbosity\n"
		"\t-q\tDecrease verbosity\n"
		"\t-f\tOverwrite existing output files\n"
		"\t-Z\tDisable gzipped output\n"
		"\t-o\tWrite output to named file\n"
		"\nVersion: " GIT_VERSION_STRING "\n"
		"Output file name will be generated based on input file name and compression flags, unless set by -o option\n";

	opterr = 0; // Handle errors ourselves
	int go = 0;
	bool doUsage = false;
	while ((go = getopt(argc, argv, "vqfZo:")) != -1) {
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
			case 'Z':
				doGZ = false;
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
		free(outFileName);
		destroy_program_state(&state);
		return -1;
	}

	char *inFileName = strdup(argv[optind]);
	FILE *inFile = fopen(inFileName, "rb");
	if (inFile == NULL) {
		log_error(&state, "Unable to open input file");
		free(outFileName);
		free(inFileName);
		destroy_program_state(&state);
		return -1;
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
		if (doGZ) {
			if (asprintf(&outFileName, "%s/%s.satinfo.csv.gz", dn, nbn) <= 0) {
				free(nbn);
				free(inF1);
				free(inF2);
				fclose(inFile);
				free(outFileName);
				free(inFileName);
				destroy_program_state(&state);
				return -1;
			}
		} else {
			if (asprintf(&outFileName, "%s/%s.satinfo.csv", dn, nbn) <= 0) {
				free(nbn);
				free(inF1);
				free(inF2);
				fclose(inFile);
				free(outFileName);
				free(inFileName);
				destroy_program_state(&state);
				return -1;
			}
		}
		free(nbn);
		free(inF1);
		free(inF2);
	}

	errno = 0;
	char fmode[5] = {'w', 'b', 0, 0};

	if (doGZ) {
		if (clobberOutput) {
			// wb7: Write, binary, comp. level 7
			fmode[2] = 7;
		} else {
			// wbx7: Create, binary, comp. level 7
			fmode[2] = 'x';
			fmode[3] = 7;
		}
	} else {
		if (clobberOutput) {
			// wbT: Write, binary, do not gzip
			fmode[2] = 'T';
		} else {
			// wbxT: Create, binary, do not gzip
			fmode[2] = 'x';
			fmode[3] = 'T';
		}
	}

	// Use zib functions, but in transparent mode if not compressing
	// Avoids wrapping every write function with doGZ checks
	gzFile outFile = gzopen(outFileName, fmode);
	if (outFile == NULL) {
		log_error(&state, "Unable to open output file");
		log_error(&state, "%s  ", strerror(errno));
		fclose(inFile);
		free(outFileName);
		free(inFileName);
		destroy_program_state(&state);
		return -1;
	}
	log_info(&state, 1, "Writing %s output to %s", doGZ ? "compressed" : "uncompressed",
	         outFileName);
	free(outFileName);
	outFileName = NULL;

	state.started = 1;
	int msgCount = 0;
	struct stat inStat = {0};
	if (fstat(fileno(inFile), &inStat) != 0) {
		log_error(&state, "Unable to get input file status: %s", strerror(errno));
		free(inFileName);
		fclose(inFile);
		gzclose(outFile);
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
	inFileName = NULL;

	char *GNSS[] = {"GPS", "SBAS", "Galileo", "BeiDou", "IMES", "QZSS", "GLONASS"};
	gzprintf(outFile,
	         "TOW,Source,GNSS,SatID,SNR,Elevation,Azimuth,Residual,Quality,SatUsed\n");
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
		if (tmp.source >= SLSOURCE_GPS && tmp.source < (SLSOURCE_GPS + 0x10)) {
			const uint8_t *data = tmp.data.bytes;
			// Only interested in raw messages from GPS
			// Only looking for UBX 01:35 messages
			if (tmp.type == 0x03 && (data[0] == 0xb5 && data[1] == 0x62 &&
			                         data[2] == 0x01 && data[3] == 0x35)) {
				const uint32_t tow = data[6] + (data[7] << 8) + (data[8] << 16) +
				                     (data[9] << 24);
				const uint8_t numSats = data[11];

				for (int i = 0; i < numSats; i++) {
					const uint8_t gnssID = data[14 + 12 * i];
					const uint8_t satID = data[15 + 12 * i];
					const uint8_t SNR = data[16 + 12 * i];
					const int8_t elev = data[17 + 12 * i];
					const int16_t azi =
						data[18 + 12 * i] + (data[19 + 12 * i] << 8);
					const int16_t res =
						(data[20 + 12 * i] + (data[21 + 12 * i] << 8)) /
						10;
					const uint8_t qual = data[22 + 12 * i] & 0x07;
					const uint8_t inUse = (data[22 + 12 * i] & 0x08) / 0x08;
					gzprintf(outFile, "%u,%02X,%s,%u,%u,%d,%d,%d,%u,%u\n", tow,
					         tmp.source, GNSS[gnssID], satID, SNR, elev, azi,
					         res, qual, inUse);
				}
			}
			msgCount++;
		}
		msg_destroy(&tmp);
		inPos = ftell(inFile);
		if (((((1.0 * inPos) / inSize) * 100) - progress) >= 5) {
			progress = (((1.0 * inPos) / inSize) * 100);
			log_info(&state, 2, "Progress: %d%% (%ld / %ld)", progress, inPos, inSize);
		}
	}
	fclose(inFile);
	gzclose(outFile);

	log_info(&state, 1, "%d messages processed", msgCount);
	destroy_program_state(&state);
	return 0;
}
