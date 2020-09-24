#include "Logger.h"

//! @file Logger.c Generates main data logging executable

int main(int argc, char *argv[]) {
	char *gpsPortName = NULL;
        char *monPrefix = NULL;
	char *stateName = NULL;

	int verbose = 0;
	bool saveMonitor = true;
	bool saveState = true;
	bool saveLog = true;
	bool rotateMonitor = true;

        char *usage =  "Usage: %1$s [-v] [-g port] [-m file [-R]| -M] [-s file | -S] [-L]\n"
                "\t-v\tIncrease verbosity\n"
                "\t-g port\tSpecify GPS port\n"
                "\t-m file\tMonitor file prefix\n"
                "\t-s file\tState file name. Default derived from GPS port name\n"
		"\t-M\tDo not use monitor file\n"
		"\t-S\tDo not use state file\n"
		"\t-L\tDo not store log messages\n"
		"\t-R\tDo not rotate monitor file\n"
                "\nVersion: " GIT_VERSION_STRING "\n"
		"Default options equivalent to:\n"
		"\t%1$s -g " DEFAULT_GPS_PORT " -m " DEFAULT_MON_PREFIX "\n";

/*****************************************************************************
	Program Startup
*****************************************************************************/
	setvbuf(stdout, NULL, _IONBF, 0); // Unbuffered stdout = more accurate journalling/reporting

        opterr = 0; // Handle errors ourselves
        int go = 0;
        while ((go = getopt (argc, argv, "vg:m:s:MSRL")) != -1) {
                switch(go) {
                        case 'v':
                                verbose++;
                                break;
			case 'g':
				gpsPortName = strdup(optarg);
                                break;
			case 'm':
				monPrefix = strdup(optarg);
				break;
			case 's':
				stateName = strdup(optarg);
				break;
			case 'S':
				saveState = false;
				break;
			case 'M':
				saveMonitor = false;
				break;
			case 'L':
				saveLog = false;
				break;
			case 'R':
				rotateMonitor = false;
				break;
			case '?':
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				fprintf(stderr, usage, argv[0]);
				return EXIT_FAILURE;
		}
        }

	// Should be no spare arguments
        if (argc - optind != 0) {
		fprintf(stderr, usage, argv[0]);
                return EXIT_FAILURE;
        }

	// Check for conflicting options
	if (stateName && !saveState) {
		fprintf(stderr, "-s and -S are mutually exclusive\n");
		fprintf(stderr, usage, argv[0]);
		return EXIT_FAILURE;
	}

	if ((monPrefix || !rotateMonitor) && !saveMonitor) {
		fprintf(stderr, "-M and -m/-R are mutually exclusive\n");
		fprintf(stderr, usage, argv[0]);
		return EXIT_FAILURE;
	}

	if (!saveMonitor && saveLog) {
		fprintf(stderr, "Will not save log if not saving monitor file\n");
		return EXIT_FAILURE;
	}

	// Set defaults if no argument provided
	// Defining the defaults as compiled in constants at the top of main() causes issues
	// with overwriting them later
	if (!gpsPortName) {
		gpsPortName = strdup(DEFAULT_GPS_PORT);
	}

	if (!monPrefix) {
		monPrefix = strdup(DEFAULT_MON_PREFIX);
	}

	// Default state file name is derived from the port name
	if (!stateName) {
		char *tmp = strdup(gpsPortName);
		if (asprintf(&stateName, "%s.state", basename(tmp)) < 0) {
			perror("asprintf");
			return -1;
		}
		free(tmp);
	}

	/*** Set up signal information for installation later ***/
	sigset_t blocking;
	sigemptyset(&blocking);
	sigaddset(&blocking, SIGINT);
	sigaddset(&blocking, SIGQUIT);
	sigaddset(&blocking, SIGUSR1);
	sigaddset(&blocking, SIGHUP);
	sigaddset(&blocking, SIGRTMIN + 1);
	sigaddset(&blocking, SIGRTMIN + 2);
	sigaddset(&blocking, SIGRTMIN + 3);
	sigaddset(&blocking, SIGRTMIN + 4);
	const struct sigaction saShutdown = {.sa_handler = signalShutdown, .sa_mask = blocking, .sa_flags = SA_RESTART};
	const struct sigaction saRotate = {.sa_handler = signalRotate, .sa_mask = blocking, .sa_flags = SA_RESTART};
	const struct sigaction saPause = {.sa_handler = signalPause, .sa_mask = blocking, .sa_flags = SA_RESTART};
	const struct sigaction saUnpause = {.sa_handler = signalUnpause, .sa_mask = blocking, .sa_flags = SA_RESTART};


	int gpsHandle = openConnection(gpsPortName);
	if (gpsHandle < 0) {
		fprintf(stderr, "Unable to open a connection on port %s\n", gpsPortName);
		perror("openConnection");
		return -1;
	}

	FILE *monitorFile = openSerialNumberedFile(monPrefix, "dat");

	if (monitorFile == NULL) {
		perror("monitorFile");
		return -1;
	}

	/***
	 * Once startup is complete, enable external signal processing
	 **/

	sigaction(SIGINT, &saShutdown, NULL);
	sigaction(SIGQUIT, &saShutdown, NULL);
	sigaction(SIGRTMIN + 1, &saShutdown, NULL);
	sigaction(SIGUSR1, &saRotate, NULL);
	sigaction(SIGHUP, &saRotate, NULL);
	sigaction(SIGRTMIN + 2, &saRotate, NULL);
	sigaction(SIGRTMIN + 3, &saPause, NULL);
	sigaction(SIGRTMIN + 4, &saUnpause, NULL);


	int count = 0;
	while (!shutdown) {
		ubx_message out;
		if (readMessage(gpsHandle, &out)) {
			count++;
			ubx_print_hex(&out);
			printf("\n");
			uint8_t *data = NULL;
			ssize_t len = ubx_flat_array(&out, &data);
			fwrite(data, len, 1, monitorFile);
		} else {
			if (!(out.sync1 == 0xFF || out.sync1 == 0xFD)) {
				fprintf(stderr, "Error signalled from readMessage\n");
				break;
			}
		}
	}
	closeConnection(gpsHandle);
	fclose(monitorFile);
	fprintf(stdout, "%d messages read successfully\n\n", count);
	return 0;
}

FILE *openSerialNumberedFile(const char *prefix, const char *extension) {
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	char *fileName = NULL;
	char date[9];
	strftime(date, 9, "%Y%m%d", tm);

	int i = 0;
	FILE *file = NULL;
	while (i <= 0xFF) {
		errno = 0;
		if (asprintf(&fileName, "%s%s%02x.%s", prefix, date, i, extension) == -1) {
			return NULL;
		}
		errno = 0;
		file = fopen(fileName, "w+x"); //w+x = rw + create. Fail if exists
		if (file) {
			fprintf(stdout, "Using: %s.\n", fileName);
			free(fileName);
			return file;
		}
		if (errno != EEXIST) {
			free(fileName);
			return NULL;
		}
		i++;
		free(fileName);
		fileName = NULL;
	}

	return NULL;
}

// Signal handlers documented in header file
void signalShutdown(int signnum __attribute__((unused))) {
	shutdown = true;
}

void signalRotate(int signnum __attribute__((unused))) {
	rotateNow = true;
}

void signalPause(int signnum __attribute__((unused))) {
	pauseLog = true;
}

void signalUnpause(int signnum __attribute__((unused))) {
	pauseLog = false;
}
