#include "Logger.h"

//! @file Logger.c Generates main data logging executable

int main(int argc, char *argv[]) {
	char *gpsPortName = NULL;
        char *monPrefix = NULL;
	char *stateName = NULL;

	program_state state = {0};
	state.verbose = 0;

	int initialBaud = 9600;
	bool saveMonitor = true;
	bool saveState = true;
	bool rotateMonitor = true;
	bool logToFile = false;

        char *usage =  "Usage: %1$s [-v] [-q] [-l] [-g port] [-b initial baud] [-m file [-R]| -M] [-s file | -S]\n"
                "\t-v\tIncrease verbosity\n"
		"\t-q\tDecrease verbosity\n"
		"\t-l\tLog information messages to file\n"
                "\t-g port\tSpecify GPS port\n"
		"\t-b baud\tGPS initial baud rate\n"
                "\t-m file\tMonitor file prefix\n"
                "\t-s file\tState file name. Default derived from GPS port name\n"
		"\t-M\tDo not use monitor file\n"
		"\t-S\tDo not use state file\n"
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
	bool doUsage = false;
        while ((go = getopt (argc, argv, "vqb:g:m:s:MSRL")) != -1) {
                switch(go) {
                        case 'v':
                                state.verbose++;
                                break;
			case 'q':
				state.verbose--;
				break;
			case 'b':
				errno = 0;
				initialBaud = strtol(optarg, NULL, 10);
				if (errno) {
					log_error(&state, "Bad initial baud value ('%s')\n", optarg);
					doUsage = true;
				}
				break;
			case 'g':
				if (gpsPortName) {
					log_error(&state, "Only a single GPS port can be specified");
					doUsage = true;
				} else {
					gpsPortName = strdup(optarg);
				}
                                break;
			case 'l':
				logToFile = true;
				break;
			case 'm':
				if (monPrefix) {
					log_error(&state, "Only a single monitor file prefix can be specified");
					doUsage = true;
				} else {
					monPrefix = strdup(optarg);
				}
				break;
			case 's':
				if (stateName) {
					log_error(&state, "Only a single state file name can be specified");
					doUsage = true;
				} else {
					stateName = strdup(optarg);
				}
				break;
			case 'S':
				saveState = false;
				break;
			case 'M':
				saveMonitor = false;
				break;
			case 'R':
				rotateMonitor = false;
				break;
			case '?':
				log_error(&state, "Unknown option `-%c'.\n", optopt);
				doUsage = true;
		}
        }

	// Should be no spare arguments
        if (argc - optind != 0) {
		log_error(&state, "Invalid arguments");
		doUsage = true;
        }

	// Check for conflicting options
	if (stateName && !saveState) {
		log_error(&state, "-s and -S are mutually exclusive");
		doUsage = true;
	}

	if ((monPrefix || !rotateMonitor) && !saveMonitor) {
		log_error(&state, "-M and -m/-R are mutually exclusive");
		doUsage = true;
	}

	if (doUsage) {
		fprintf(stderr, usage, argv[0]);
		if (monPrefix) {free(monPrefix);}
		if (stateName) {free(stateName);}
		if (gpsPortName) {free(gpsPortName);}
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
			free(tmp);
			free(gpsPortName);
			free(monPrefix);
			return -1;
		}
		free(tmp);
	}

	if (logToFile) {
		state.log = openSerialNumberedFile(monPrefix, "log");
		if (!state.log) {
			log_error(&state, "Unable to open log file: %s", strerror(errno));
			free(gpsPortName);
			free(monPrefix);
			free(stateName);
			return EXIT_FAILURE;
		}

		// When logging to file, cap printed messages to basic information (0)
		// and use the verbosity level to dictate what is logged to file instead
		if (state.verbose > 0) {
			state.logverbose = state.verbose;
			state.verbose = 0;
		}
	}


	log_info(&state, 1, "Using GPS port %s", gpsPortName);
	log_info(&state, 1, "Using state file %s", stateName);
	log_info(&state, 1, "Using %s as monitor and log file prefix", monPrefix);

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


	int gpsHandle = gps_setup(&state, gpsPortName, initialBaud);
	free(gpsPortName);
	gpsPortName = NULL;

	if (gpsHandle < 0) {
		log_error(&state, "Unable to set up GPS connection");
		return EXIT_FAILURE;
	}

	FILE *monitorFile = openSerialNumberedFile(monPrefix, "dat");

	if (monitorFile == NULL) {
		perror("monitorFile");
		return -1;
	}


	msgqueue log_queue = {0};
	if (!queue_init(&log_queue)) {
		log_error(&state, "Unable to initialise message queue");
		return EXIT_FAILURE;
	}

	log_info(&state, 1, "Initialisation complete, starting log threads");

	log_thread_args_t ltargs = {&log_queue, &state, gpsHandle};
	pthread_t threads[1];
	if (pthread_create(&(threads[0]), NULL, &gps_logging, &ltargs) != 0){
		log_error(&state, "Unable to launch GPS thread");
		return EXIT_FAILURE;
	}

	state.started = true;
	log_info(&state, 1, "Startup complete");
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
		if (pauseLog) {
			log_info(&state, 0, "Logging paused");
			while (pauseLog && !shutdown) {
				sleep(1);
			}
			log_info(&state, 0, "Logging resumed");
			continue;
		}
		msg_t *res = queue_pop(&log_queue);
		if (res == NULL) {
			usleep(1000);
			continue;
		}
		count++;
		if (res->dtype == MSG_BYTES) {
			fwrite(res->data.bytes, res->length, 1, monitorFile);
		} else {
			log_warning(&state, "Unexpected message type (%d)", res->dtype);
		}
		msg_destroy(res);
		free(res);
	}
	state.shutdown = true;
	shutdown = true; // Ensure threads aware
	log_info(&state, 1, "Shutting down");
	pthread_join(threads[0], NULL);
	log_info(&state, 2, "GPS thread returned %d", ltargs.returnCode);
	ubx_closeConnection(gpsHandle);
	log_info(&state, 2, "GPS device closed");

	if (queue_count(&log_queue) > 0) {
		log_info(&state, 2, "Processing remaining queued messages");
		while (queue_count(&log_queue) > 0) {
			msg_t *res = queue_pop(&log_queue);
			count++;
			if (res->dtype == MSG_BYTES) {
				fwrite(res->data.bytes, res->length, 1, monitorFile);
			} else {
				log_warning(&state, "Unexpected message type (%d)", res->dtype);
			}
			msg_destroy(res);
			free(res);
		}
		log_info(&state, 2, "Queue emptied");
	}
	queue_destroy(&log_queue);
	log_info(&state, 2, "Message queue destroyed");
	fclose(monitorFile);
	log_info(&state, 2, "Monitor file closed");
	log_info(&state, 0, "%d messages read successfully\n\n", count);
	return 0;
}
