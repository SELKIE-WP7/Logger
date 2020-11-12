#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

#include "version.h"

int sort_uint(const void *a, const void *b);

char * csv_all_timestamp_headers(const uint8_t source, const uint8_t type, const char *sourceName, const char *channelName);
char * csv_all_timestamp_data(const msg_t *msg);
char * csv_gps_position_headers(const uint8_t source, const uint8_t type, const char *sourcename, const char *channelName);
char * csv_gps_position_data(const msg_t *msg);
char * csv_gps_velocity_headers(const uint8_t source, const uint8_t type, const char *sourcename, const char *channelName);
char * csv_gps_velocity_data(const msg_t *msg);
char * csv_gps_datetime_headers(const uint8_t source, const uint8_t type, const char *sourcename, const char *channelName);
char * csv_gps_datetime_data(const msg_t *msg);

typedef char *(*csv_header_fn)(const uint8_t, const uint8_t, const char *, const char *);
typedef char *(*csv_data_fn)(const msg_t *);

typedef struct {
	uint8_t source;
	uint8_t type;
	csv_header_fn header;
	csv_data_fn data;
} csv_msg_handler;

int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	char *varfileName = NULL;
	char *outfileName = NULL;
	bool doGZ = true;
	bool clobberOutput = false;
	uint8_t primaryClock = 0x02;

        char *usage =  "Usage: %1$s [-v] [-q] [-f] [-c varfile] [-z|-Z] [-T source] [-o outfile] datfile\n"
                "\t-v\tIncrease verbosity\n"
		"\t-q\tDecrease verbosity\n"
		"\t-f\tOverwrite existing output files\n"
		"\t-c\tRead source and channel names from specified file\n"
		"\t-z\tEnable gzipped output\n"
		"\t-Z\tDisable gzipped output\n"
		"\t-T\tUse specified source as primary clock\n"
		"\t-o\tWrite output to named file\n"
                "\nVersion: " GIT_VERSION_STRING "\n"
		"Default options equivalent to:\n"
		"\t%1$s -z -T 0x02 datafile\n"
		"Output file name will be generated based on input file name and compression flags, unless set by -o option\n"
		"If no var file is provided using the -c option, the data file will be parsed twice in order to discover active channels\n";

        opterr = 0; // Handle errors ourselves
        int go = 0;
	bool doUsage = false;
        while ((go = getopt(argc, argv, "vqfzZc:o:T:")) != -1) {
                switch(go) {
                        case 'v':
                                state.verbose++;
                                break;
			case 'q':
				state.verbose--;
				break;
			case 'f':
				clobberOutput = true;
				break;
			case 'z':
				doGZ = true;
				break;
			case 'Z':
				doGZ = false;
				break;
			case 'c':
				varfileName = strdup(optarg);
				break;
			case 'o':
				outfileName = strdup(optarg);
				break;
			case 'T':
				errno = 0;
				primaryClock = strtol(optarg, NULL, 0);
				if (errno || primaryClock == 0 || primaryClock >= 128) {
					log_error(&state, "Bad clock source value ('%s')", optarg);
					doUsage = true;
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
		return -1;
	}

	char *infileName = strdup(argv[optind]);
	FILE *inFile = fopen(infileName, "rb");
	if (inFile == NULL) {
		log_error(&state, "Unable to open input file");
		return -1;
	}


	char *sourceNames[128] = {0};
	char *channelNames[128][128] = {0};
	for (int i=0; i < 128; i++) {
		sourceNames[i] = NULL;
		for (int j = 0; j < 128; j++) {
			channelNames[i][j] = NULL;
		}
	}
	sourceNames[0] = strdup("Logger");
	sourceNames[1] = strdup("dat2csv");

	int nSources = 0;
	uint8_t usedSources[128] = {0};

	if (varfileName == NULL) {
		varfileName = strdup(infileName);
		if (varfileName == NULL) {
			log_error(&state, "Error processing variable file name: %s", strerror(errno));
			return -1;
		}
	}

	// No longer run conditionally, but keeping variables in own scope
	{
		log_info(&state, 1, "Reading channel and source names from %s", varfileName);
		FILE *varFile = fopen(varfileName, "rb");
		if (varFile == NULL) {
			log_error(&state, "Unable to open variable file");
			return -1;
		}
		bool exitLoop = false;
		while (!(feof(varFile) || exitLoop)) {
			msg_t tmp = {0};
			if (!mp_readMessage(fileno(varFile), &tmp)) {
				if (tmp.data.value == 0xFF) {
					continue;
				} else if (tmp.data.value == 0xFD) {
					// No more data, exit cleanly
					log_info(&state, 2, "End of variable file reached");
					exitLoop = true;
					continue;
				} else {
					log_error(&state, "Error reading messages from variable file (Code: 0x%52x)\n", (uint8_t) tmp.data.value);
					log_error(&state, "%s", strerror(errno));
					return -1;
				}
			}
			if (tmp.type == SLCHAN_NAME) {
				if (tmp.data.string.length == 0) {
					continue;
				}
				if (sourceNames[tmp.source]) {
					// Most recent name wins, so discard any previous entries
					free(sourceNames[tmp.source]);
					sourceNames[tmp.source] = NULL;
					nSources--;
				}
				sourceNames[tmp.source] = strndup(tmp.data.string.data, tmp.data.string.length);
				usedSources[nSources++] = tmp.source;
			} else if (tmp.type == SLCHAN_MAP) {
				for (int i = 0; i < tmp.data.names.entries && i < 128; i++) {
					if (tmp.data.names.strings[i].length == 0) {
						continue;
					}
					if (channelNames[tmp.source][i]) {
						// Most recent name wins, so discard any previous entries
						free(channelNames[tmp.source][i]);
						channelNames[tmp.source][i] = NULL;
					}
					channelNames[tmp.source][i] = strndup(tmp.data.names.strings[i].data, tmp.data.names.strings[i].length);
				}
			} // And ignore any other message types
			msg_destroy(&tmp);
		}
		fclose(varFile);
		for (int i=0; i < 128; i++) {
			if (sourceNames[i]) {
				log_info(&state, 2, "[0x%02x]\t%s", i, sourceNames[i]);
			}
			for (int j = 0; j < 128; j++) {
				if (channelNames[i][j]) {
					log_info(&state, 2, "      \t[0x%02x]\t%s", j, channelNames[i][j]);
				}
			}
		}
		free(varfileName);
		varfileName = NULL;
		qsort(&usedSources, nSources, sizeof(uint8_t), &sort_uint);
	}

	int nHandlers = 0;
	int maxHandlers = 100;
	csv_msg_handler *handlers = calloc(maxHandlers, sizeof(csv_msg_handler));
	if (handlers == NULL) {
		log_error(&state, "Unable to allocate handler map");
	}

	// Set up per source timestamps (master clock handle separately);
	for (int i = 0; i < nSources; i++) {
		handlers[nHandlers++] = (csv_msg_handler) {usedSources[i], SLCHAN_TSTAMP, &csv_all_timestamp_headers, &csv_all_timestamp_data};

		if (nHandlers >= maxHandlers) {
			handlers = reallocarray(handlers, 50+maxHandlers, sizeof(csv_msg_handler));
			if (handlers == NULL) {
				log_error(&state, "Unable to expand handler map: %s", strerror(errno));
				return -1;
			}
		}
	}
	// GPS sources
	for (int i = 0; i < nSources; i++) {
		if (usedSources[i] >= SLSOURCE_GPS && usedSources[i] < (SLSOURCE_GPS + 0x10)) {
			// Source ID in correct range, assume this is GPS device
			handlers[nHandlers++] = (csv_msg_handler) {usedSources[i], 4, &csv_gps_position_headers, &csv_gps_position_data};
			handlers[nHandlers++] = (csv_msg_handler) {usedSources[i], 5, &csv_gps_velocity_headers, &csv_gps_velocity_data};
			handlers[nHandlers++] = (csv_msg_handler) {usedSources[i], 6, &csv_gps_datetime_headers, &csv_gps_datetime_data};
		}
	}

	if (outfileName == NULL) {
		// Work out output file name
		// Split into base and dirnames so that we're don't accidentally split the path on a .
		char *inF1 = strdup(infileName);
		char *inF2 = strdup(infileName);
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
		char *nbn = calloc(bnl+1, sizeof(char));
		strncpy(nbn, bn, bnl);
		if (doGZ) {
			if (asprintf(&outfileName, "%s/%s.csv.gz", dn, nbn) <= 0) {
				return -1;
			}
		} else {
			if (asprintf(&outfileName, "%s/%s.csv", dn, nbn) <= 0) {
				return -1;
			}
		}
		free(nbn);
		free(inF1);
		free(inF2);
	}

	errno = 0;
	char fmode[5] = {'w','b', 0, 0};

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
	gzFile outFile = gzopen(outfileName, fmode);
	if (outFile == NULL) {
		log_error(&state, "Unable to open output file");
		log_error(&state, "%s", strerror(errno));
		return -1;
	}

	state.started = 1;
	int msgCount = 0;
	struct stat inStat = {0};
	if (fstat(fileno(inFile), &inStat) != 0) {
		log_error(&state, "Unable to get input file status: %s", strerror(errno));
		return -1;
	}
	long inSize = inStat.st_size;
	long inPos = 0;
	int progress = 0;
	if (inSize >= 1E9) {
		log_info(&state, 1, "Reading %.2fGB from %s", inSize / 1.0E9, infileName);
	} else if (inSize >= 1E6) {
		log_info(&state, 1, "Reading %.2fMB from %s", inSize / 1.0E6, infileName);
	} else if (inSize >= 1E3) {
		log_info(&state, 1, "Reading %.2fkB from %s", inSize / 1.0E3, infileName);
	} else {
		log_info(&state, 1, "Reading %ld bytes from %s", inSize, infileName);
	}


	uint32_t timestep = 0;
	uint32_t nextstep = 0;

	size_t hlen = 0;
	size_t hsize = 512;
	char *header = calloc(hsize, 1);

	// The first handler is the master clock timestamp, so special case that header
	hlen += snprintf(header+hlen, hsize-hlen, "Timestamp");
	// For all remaining headers:
	for (int i = 0; i < nHandlers; i++) {
		char *fieldTitle = handlers[i].header(handlers[i].source, handlers[i].type, sourceNames[handlers[i].source], channelNames[handlers[i].source][handlers[i].type]);
		if (fieldTitle == NULL) {
			log_error(&state, "Unable to generate field name string: %s", strerror(errno));
			return -1;
		}
		if (hlen > (hsize - strlen(fieldTitle))) {
			header = realloc(header, hsize + 512);
			hsize += 512;
		}
		hlen += snprintf(header+hlen,hsize-hlen,",%s", fieldTitle);
		free(fieldTitle);
	}

	msg_t currentTimestep[1000];
	int currMsg = 0;
	log_info(&state, 2, "%s", header);
	gzprintf(outFile, "%s\n", header);
	while (!(feof(inFile))) {
		// Read message from data file
		msg_t *tmp = &(currentTimestep[currMsg++]);
		if (!mp_readMessage(fileno(inFile), tmp)) {
			if (tmp->data.value == 0xAA || tmp->data.value == 0xEE) {
				log_error(&state, "Error reading messages from file (Code: 0x%52x)\n", (uint8_t) tmp->data.value);
			}
			if (tmp->data.value == 0xFD) {
				// No more data, exit cleanly
				log_info(&state, 1, "End of file reached");
			}
			break;
		}
		// Increment total message count
		msgCount++;

		// Check whether we're updating the current timestep
		if (tmp->source == primaryClock && tmp->type == SLCHAN_TSTAMP) {
			nextstep = tmp->data.timestamp;
		}


		// Time for a new record? Write it out
		if (nextstep != timestep) {
			// Special case: Timestep from master clock
			gzprintf(outFile, "%d", timestep);
			// Call all message handlers, in order
			for (int i = 0; i < nHandlers; i++) {
				bool handled = false;
				for (int m = 0; m < currMsg; m++) {
					msg_t *msg = &(currentTimestep[m]);
					if (msg->type == handlers[i].type && msg->source == handlers[i].source) {
						char *out = handlers[i].data(msg);
						if (out == NULL) {
							log_error(&state, "Error converting message to output format: %s", strerror(errno));
							return -1;
						}
						gzprintf(outFile, ",%s", out);
						free(out);
						handled = true;
						break; // First instance of each message wins
					}
				}
				if (!handled) {
					char *out = handlers[i].data(NULL); // Generate empty fields
					if (out == NULL) {
						log_error(&state, "Error generating empty field: %s", strerror(errno));
						return -1;
					}
					gzprintf(outFile, ",%s", out);
				}
			}
			gzprintf(outFile, "\n");
			// Empty current message list
			for (int m=0; m < currMsg; m++) {
				msg_destroy(&(currentTimestep[m]));
			}
			currMsg = 0;
			timestep = nextstep;
		}

		inPos = ftell(inFile);
		if (((((1.0 * inPos) / inSize) * 100) - progress) >= 5) {
			progress = (((1.0 * inPos) / inSize) * 100);
			log_info(&state, 2, "Progress: %d%% (%ld / %ld)", progress, inPos, inSize);
		}
	}
	fclose(inFile);
	gzclose(outFile);

	for (int i = 0; i < 128; i++) {
		if (sourceNames[i] != NULL) {
			free(sourceNames[i]);
		}
	}
	for (int i = 0; i < 128; i++) {
		for (int j = 0; j < 128; j++) {
			if (channelNames[i][j] != NULL) {
				free(channelNames[i][j]);
				channelNames[i][j] = NULL;
			}
		}
	}
	log_info(&state, 1, "%d messages processed", msgCount);
	free(outfileName);
	free(infileName);
	return 0;
}

int sort_uint(const void *a, const void *b) {
	const uint8_t *ai = a;
	const uint8_t *bi = b;
	if (*ai > *bi) {
		return 1;
	} else if (*ai < *bi) {
		return -1;
	}
	return 0;
}

char * csv_all_timestamp_headers(const uint8_t source, const uint8_t type, const char *sourceName, const char *channelName) {
	char *fields = NULL;
	if (asprintf(&fields, "Timestamp:%02X", source) <= 0) {
		return NULL;
	}
	return fields;
}

char * csv_all_timestamp_data(const msg_t *msg) {
	char *out = NULL;
	if (msg == NULL) {
		return strdup("");
	}

	if (asprintf(&out, "%d", msg->data.timestamp) <= 0) {
		return NULL;
	}
	return out;
}

char * csv_gps_position_headers(const uint8_t source, const uint8_t type, const char *sourcename, const char *channelName) {
	char *fields = NULL;
	if (asprintf(&fields, "Longitude:%1$02X,Latitude:%1$02X,Height:%1$02X,HAcc:%1$02X,VAcc:%1$02X", source) <= 0) {
		return NULL;
	}
	return fields;
}

char * csv_gps_position_data(const msg_t *msg) {
	char *out = NULL;
	if (msg == NULL) {
		return strdup(",,,,");
	}
	const float *d = msg->data.farray;
	if (asprintf(&out, "%.5f,%.5f,%.3f,%.3f,%.3f", d[0], d[1], d[2], d[4], d[5]) <= 0) {
		return NULL;
	}
	return out;
}

char * csv_gps_velocity_headers(const uint8_t source, const uint8_t type, const char *sourcename, const char *channelName) {
	char *fields = NULL;
	if (asprintf(&fields, "Velocity_N:%1$02X,Velocity_E:%1$02X,Velocity_D:%1$02X,SpeedAcc:%1$02X,Heading:%1$02X,HeadAcc:%1$02X", source) <= 0) {
		return NULL;
	}
	return fields;
}

char * csv_gps_velocity_data(const msg_t *msg) {
	char *out = NULL;
	if (msg == NULL) {
		return strdup(",,,,,");
	}
	const float *d = msg->data.farray;
	if (asprintf(&out, "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f", d[0], d[1], d[2], d[5], d[4], d[6]) <= 0) {
		return NULL;
	}
	return out;
}

char * csv_gps_datetime_headers(const uint8_t source, const uint8_t type, const char *sourcename, const char *channelName) {
	char *fields = NULL;
	if (asprintf(&fields, "Date:%1$02X,Time:%1$02X,DTAcc:%1$02X", source) <= 0) {
		return NULL;
	}
	return fields;
}

char * csv_gps_datetime_data(const msg_t *msg) {
	char *out = NULL;
	if (msg == NULL) {
		return strdup(",,");
	}
	const float *d = msg->data.farray;
	//if (asprintf(&out, "\"%04.0f-%02.0f-%02.0f\",\"%02.0f:%02.0f:%02.0f.%06.0f\",%09.0f", d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7]) <= 0) {
	if (asprintf(&out, "%04.0f-%02.0f-%02.0f,%02.0f:%02.0f:%02.0f.%06.0f,%09.0f", d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7]) <= 0) {
		return NULL;
	}
	return out;
}
