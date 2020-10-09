#include "Logger.h"

//! @file Logger.c Generates main data logging executable

int main(int argc, char *argv[]) {
        char *monPrefix = NULL;
	char *stateName = NULL;

	program_state state = {0};
	state.verbose = 0;

	bool saveMonitor = true;
	bool saveState = true;
	bool rotateMonitor = true;

	gps_params gpsParams = gps_getParams();
	mp_params mpParams = mp_getParams();

        char *usage =  "Usage: %1$s [-v] [-q] [-g port] [-p port] [-b initial baud] [-m file [-R]| -M] [-s file | -S]\n"
                "\t-v\tIncrease verbosity\n"
		"\t-q\tDecrease verbosity\n"
                "\t-g port\tSpecify GPS port\n"
		"\t-p port\tMP source port\n"
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
        while ((go = getopt (argc, argv, "vqb:g:m:p:s:MSRL")) != -1) {
                switch(go) {
                        case 'v':
                                state.verbose++;
                                break;
			case 'q':
				state.verbose--;
				break;
			case 'b':
				errno = 0;
				gpsParams.initialBaud = strtol(optarg, NULL, 10);
				if (errno) {
					log_error(&state, "Bad initial baud value ('%s')\n", optarg);
					doUsage = true;
				}
				break;
			case 'g':
				if (gpsParams.portName) {
					log_error(&state, "Only a single GPS port can be specified");
					doUsage = true;
				} else {
					gpsParams.portName = strdup(optarg);
				}
                                break;
			case 'm':
				if (monPrefix) {
					log_error(&state, "Only a single monitor file prefix can be specified");
					doUsage = true;
				} else {
					monPrefix = strdup(optarg);
				}
				break;
			case 'p':
				if (mpParams.portName) {
					log_error(&state, "Only a single MP port can be specified");
					doUsage = true;
				} else {
					mpParams.portName = strdup(optarg);
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
		if (gpsParams.portName) {free(gpsParams.portName);}
		return EXIT_FAILURE;
	}

	// Set defaults if no argument provided
	// Defining the defaults as compiled in constants at the top of main() causes issues
	// with overwriting them later
	if (!gpsParams.portName) {
		gpsParams.portName = strdup(DEFAULT_GPS_PORT);
	}

	if (!monPrefix) {
		monPrefix = strdup(DEFAULT_MON_PREFIX);
	}

	// Default state file name is derived from the port name
	if (!stateName) {
		char *tmp = strdup(gpsParams.portName);
		if (asprintf(&stateName, "%s.state", basename(tmp)) < 0) {
			perror("asprintf");
			free(tmp);
			free(gpsParams.portName);
			free(monPrefix);
			return -1;
		}
		free(tmp);
	}

	int mon_yday = -1; //!< Log rotation markers: Day for currently opened files
	int mon_nextyday = -2; //!< Log rotation markers: Day for next files

	{
		/*
		 * Store current yday so that we can rotate files when the day changes
		 * If we happen to hit midnight between now and the loop then we might
		 * get an empty/near empty file before rotating, but that's an unlikely
		 * enough event that ignoring it feels reasonable
		 */
		time_t timeNow = time(NULL);
		struct tm *now = localtime(&timeNow);
		mon_yday = now->tm_yday;
		// If first rotation is triggered by signal and current != next
		// we get two rotations, so set them to the same value here.
		mon_nextyday = mon_yday;
	}

	errno = 0;
	state.log = openSerialNumberedFile(monPrefix, "log");
	state.logverbose = 3;
	if (!state.log) {
		if (errno == EEXIST) {
			log_error(&state, "Unable to open log file - too many files created with this prefix today?");
		} else {
			log_error(&state, "Unable to open log file: %s", strerror(errno));
		}
		free(gpsParams.portName);
		free(monPrefix);
		free(stateName);
		return EXIT_FAILURE;
	}

	log_info(&state, 1, "Using GPS port %s", gpsParams.portName);
	log_info(&state, 1, "Using state file %s", stateName);
	log_info(&state, 1, "Using %s as monitor and log file prefix", monPrefix);

	// Block signal handling until we're up and running
	signalHandlersBlock();

	msgqueue log_queue = {0};
	if (!queue_init(&log_queue)) {
		log_error(&state, "Unable to initialise message queue");
		return EXIT_FAILURE;
	}

	int nThreads = 1;
	if (mpParams.portName) { nThreads++;}

	log_thread_args_t *ltargs = calloc(nThreads, sizeof(log_thread_args_t));
	pthread_t *threads = calloc(nThreads, sizeof(pthread_t));

	device_callbacks gpsDev = gps_getCallbacks();
	ltargs[0].logQ = &log_queue;
	ltargs[0].pstate = &state;
	ltargs[0].dParams = &gpsParams;

	gpsDev.startup(&(ltargs[0]));
	if (ltargs[0].returnCode < 0) {
		log_error(&state, "Unable to set up GPS connection");
		return EXIT_FAILURE;
	}


	device_callbacks mpDev = mp_getCallbacks();
	if (mpParams.portName) {
		ltargs[1].logQ = &log_queue;
		ltargs[1].pstate = &state;
		ltargs[1].dParams = &mpParams;
		mpDev.startup(&(ltargs[1]));
		if (ltargs[1].returnCode < 0) {
			log_error(&state, "Unable to set up MP connection on %s", mpParams.portName);
			return EXIT_FAILURE;
		}
	}

	FILE *monitorFile = NULL;

	if (saveMonitor) {
		errno = 0;
		monitorFile = openSerialNumberedFile(monPrefix, "dat");

		if (monitorFile == NULL) {
			if (errno == EEXIST) {
				log_error(&state, "Unable to open data file - too many files created with this prefix today?");
			} else {
				log_error(&state, "Unable to open data file: %s", strerror(errno));
			}
			return -1;
		}
	}

	log_info(&state, 1, "Initialisation complete, starting log threads");

	if (pthread_create(&(threads[0]), NULL, gpsDev.logging, &(ltargs[0])) != 0){
		log_error(&state, "Unable to launch GPS thread");
		return EXIT_FAILURE;
	}

	if (mpParams.portName) {
		if (pthread_create(&(threads[1]), NULL, mpDev.logging, &(ltargs[1])) != 0){
			log_error(&state, "Unable to launch MP thread");
			return EXIT_FAILURE;
		}
	}

	state.started = true;
	log_info(&state, 1, "Startup complete");

	/***
	 * Once startup is complete, enable external signal processing
	 **/
	signalHandlersInstall();
	signalHandlersUnblock();

	//! Number of successfully handled messages
	int msgCount = 0; 
	//! Loop count. Used to avoid checking e.g. date on every iteration
	unsigned int loopCount = 0;
	while (!shutdownFlag) {
		/*
		 * Main application loop
		 *
		 * This loop deals with popping messages off the stack and writing them to file (if required),
		 * as well as responding to events - either from signals or from e.g. the time of day.
		 *
		 * The different subsections of this loop interact with each other, which makes the order in
		 * which they appear significant!
		 *
		 */

		// Increment on each iteration - used below to run tasks periodically
		loopCount++;

		// Check if any of the monitoring threads have exited with an error
		for (int it=0; it < nThreads; it++) {
			if (ltargs[it].returnCode != 0) {
				log_error(&state, "Thread %d has signalled an error: %d", it, ltargs[it].returnCode);
				// Begin app shutdown
				// We allow this loop (over threads) to continue in order to report on remaining threads
				// After that, we allow the main while loop to continue (easier) and it will terminate afterwards
				shutdownFlag = true;
			}
		}

		// If we're not shutting down, Check if we need to pause
		if (pauseLog && !shutdownFlag) {
			log_info(&state, 0, "Logging paused");
			// Flush outputs, we could be here for a while.
			fflush(NULL);
			// Loop until either a) We're unpaused, b) We need to shutdown
			while (pauseLog && !shutdownFlag) {
				sleep(1);
			}
			log_info(&state, 0, "Logging resumed");
			continue;
		}

		// Periodic jobs that don't need checking/testing every iteration
		if ((loopCount % 200 == 0)) {
			if (saveMonitor && rotateMonitor) {
				/*
				 * During testing of software on the previous project, the time/localtime combination could
				 * take up a surprising amount of CPU time, so we avoid calling it if we can.
				 *
				 * This is also why mon_nextyday is used - it saves us a second call during file rotation.
				 * Note that this does mean that this check needs to appear after the pauseLog block, but
				 * before the rotateNow block.
				 */
				time_t timeNow = time(NULL);
				struct tm *now = localtime(&timeNow);
				if (now->tm_yday != mon_yday) {
					rotateNow = true;
					mon_nextyday = now->tm_yday;
				}
			}

			if ((loopCount % 1000) == 0) {
				/*
				 * Our longest interval check (for now), so we can reset the counter here.
				 * Note that the interval here (1000) must be a multiple of the outer interval (200),
				 * otherwise this check needs to be moved out a level
				 */
				fflush(NULL);
				loopCount = 0;
			}
		}

		if (rotateNow) {
			// Rotate all log files, then reset the flag

			/*
			 * Note that we don't check the rotateMonitor flag here
			 *
			 * If automatic rotation is disabled at the command line then the rotateNow flag will only be
			 * set if triggered by a signal, so this allows rotating logs on an external trigger even if
			 * automatic rotation is disabled.
			 */

			log_info(&state, 1, "Rotating log files");

			// "Monitor" / main data file rotation
			if (saveMonitor) {
				errno = 0;

				FILE *newMonitor = NULL;
				newMonitor  = openSerialNumberedFile(monPrefix, "dat");

				if (newMonitor == NULL) {
					// If the error is likely to be too many data files, continue with the old handle.
					// For all other errors, we exit.
					if (errno == EEXIST) {
						log_error(&state, "Unable to open data file - too many files created with this prefix today?");
					} else {
						log_error(&state, "Unable to open data file: %s", strerror(errno));
						return -1;
					}
				} else {
					fclose(monitorFile);
					monitorFile = newMonitor;
				}

			}

			// As above, but for the log file
			FILE *newLog = NULL;
			errno = 0;
			newLog = openSerialNumberedFile(monPrefix, "log");
			if (newLog == NULL) {
				// As above, if the issue is log file names then we continue with the existing file
				if (errno == EEXIST) {
					log_error(&state, "Unable to open log file - too many files created with this prefix today?");
				} else {
					log_error(&state, "Unable to open log file: %s", strerror(errno));
					return -1;
				}
			} else {
				FILE * oldLog = state.log;
				state.log = newLog;
				fclose(oldLog);
			}

			mon_yday = mon_nextyday;
			rotateNow = false;
		}

		// Check for waiting messages to be logged
		msg_t *res = queue_pop(&log_queue);
		if (res == NULL) {
			// No data waiting, so sleep for a little bit and go back around
			usleep(1000);
			continue;
		}
		msgCount++;
		if (saveMonitor) {
			mp_writeMessage(fileno(monitorFile), res);
		}
		msg_destroy(res);
		free(res);
	}
	state.shutdown = true;
	shutdownFlag = true; // Ensure threads aware
	log_info(&state, 1, "Shutting down");
	for (int it=0; it < nThreads; it++) {
		pthread_join(threads[it], NULL);
		if (ltargs[it].returnCode != 0) {
			log_error(&state, "Thread %d has signalled an error: %d", it, ltargs[it].returnCode);
		}
	}

	gpsDev.shutdown(&gpsParams);
	if (mpParams.portName) {
		mpDev.shutdown(&mpParams);
	}
	free(ltargs);
	free(threads);

	if (queue_count(&log_queue) > 0) {
		log_info(&state, 2, "Processing remaining queued messages");
		while (queue_count(&log_queue) > 0) {
			msg_t *res = queue_pop(&log_queue);
			msgCount++;
			if (res->dtype == MSG_BYTES) {
				if (saveMonitor) {
					fwrite(res->data.bytes, res->length, 1, monitorFile);
				}
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
	free(monPrefix);
	free(stateName);
	log_info(&state, 2, "Monitor file closed");
	log_info(&state, 0, "%d messages read successfully\n\n", msgCount);
	fclose(state.log);
	return 0;
}
