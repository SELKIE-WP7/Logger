#include "Logger.h"

//! @file Logger.c Generates main data logging executable

int main(int argc, char *argv[]) {
        char *monPrefix = NULL;
	char *stateName = NULL;

	program_state state = {0};
	state.verbose = 0;

	bool saveState = true;
	bool rotateMonitor = true;

	gps_params gpsParams = gps_getParams();
	mp_params mpParams = mp_getParams();
	nmea_params nmeaParams = nmea_getParams();
	i2c_params i2cParams = i2c_getParams();

        char *usage =  "Usage: %1$s [-v] [-q] [-g port] [-n port] [-p port] [-i port] [-b initial baud] [-m file] [-R] [-s file | -S]\n"
                "\t-v\tIncrease verbosity\n"
		"\t-q\tDecrease verbosity\n"
                "\t-g port\tSpecify GPS port\n"
		"\t-n port\tNMEA source port\n"
		"\t-p port\tMP source port\n"
		"\t-b baud\tGPS initial baud rate\n"
		"\t-i port\tI2C Bus\n"
                "\t-m file\tMonitor file prefix\n"
                "\t-s file\tState file name. Default derived from GPS port name\n"
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
        while ((go = getopt (argc, argv, "vqb:g:i:m:n:p:s:SRL")) != -1) {
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
			case 'i':
				if (i2cParams.busName) {
					log_error(&state, "Only a single I2C bus is supported");
					doUsage = true;
				} else {
					i2cParams.busName = strdup(optarg);
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
			case 'n':
				if (nmeaParams.portName) {
					log_error(&state, "Only a single NMEA port can be specified");
					doUsage = true;
				} else {
					nmeaParams.portName = strdup(optarg);
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

	if (doUsage) {
		fprintf(stderr, usage, argv[0]);
		free(gpsParams.portName);
		free(mpParams.portName);
		free(nmeaParams.portName);
		free(i2cParams.busName);
		free(monPrefix);
		free(stateName);
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
			free(mpParams.portName);
			free(nmeaParams.portName);
			free(i2cParams.busName);
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
		free(mpParams.portName);
		free(nmeaParams.portName);
		free(i2cParams.busName);
		free(monPrefix);
		free(stateName);
		return EXIT_FAILURE;
	}

	log_info(&state, 1, "Version: " GIT_VERSION_STRING); // Preprocessor concatenation, not a format string!
	log_info(&state, 1, "Using GPS port %s", gpsParams.portName);
	log_info(&state, 1, "Using state file %s", stateName);
	log_info(&state, 1, "Using %s as monitor and log file prefix", monPrefix);

	// Block signal handling until we're up and running
	signalHandlersBlock();

	msgqueue log_queue = {0};
	if (!queue_init(&log_queue)) {
		log_error(&state, "Unable to initialise message queue");
		free(gpsParams.portName);
		free(mpParams.portName);
		free(nmeaParams.portName);
		free(i2cParams.busName);
		free(monPrefix);
		free(stateName);
		return EXIT_FAILURE;
	}

	int nThreads = 1;
	if (mpParams.portName) { nThreads++;}
	if (nmeaParams.portName) { nThreads++;}
	if (i2cParams.busName) { nThreads++;}

	int tix = 0; // Thread Index
	log_thread_args_t *ltargs = calloc(nThreads, sizeof(log_thread_args_t));
	pthread_t *threads = calloc(nThreads, sizeof(pthread_t));

	ltargs[tix].tag = "GPS";
	ltargs[tix].logQ = &log_queue;
	ltargs[tix].pstate = &state;
	ltargs[tix].dParams = &gpsParams;
	ltargs[tix].funcs = gps_getCallbacks();

	ltargs[tix].funcs.startup(&(ltargs[tix]));
	if (ltargs[tix].returnCode < 0) {
		log_error(&state, "Unable to set up GPS connection");
		free(gpsParams.portName);
		free(mpParams.portName);
		free(nmeaParams.portName);
		free(i2cParams.busName);
		free(monPrefix);
		free(stateName);
		free(threads);
		return EXIT_FAILURE;
	}
	// All done, let next thread be configured
	tix++;

	if (mpParams.portName) {
		ltargs[tix].tag = "MP";
		ltargs[tix].logQ = &log_queue;
		ltargs[tix].pstate = &state;
		ltargs[tix].dParams = &mpParams;
		ltargs[tix].funcs = mp_getCallbacks();
		ltargs[tix].funcs.startup(&(ltargs[tix]));
		if (ltargs[tix].returnCode < 0) {
			log_error(&state, "Unable to set up MP connection on %s", mpParams.portName);
			free(gpsParams.portName);
			free(mpParams.portName);
			free(nmeaParams.portName);
			free(i2cParams.busName);
			free(monPrefix);
			free(stateName);
			free(threads);
			return EXIT_FAILURE;
		}
		tix++;
	}

	if (nmeaParams.portName) {
		ltargs[tix].tag = "NMEA";
		ltargs[tix].logQ = &log_queue;
		ltargs[tix].pstate = &state;
		ltargs[tix].dParams = &nmeaParams;
		ltargs[tix].funcs = nmea_getCallbacks();
		ltargs[tix].funcs.startup(&(ltargs[tix]));
		if (ltargs[tix].returnCode < 0) {
			log_error(&state, "Unable to set up NMEA connection on %s", nmeaParams.portName);
			free(gpsParams.portName);
			free(mpParams.portName);
			free(nmeaParams.portName);
			free(i2cParams.busName);
			free(monPrefix);
			free(stateName);
			free(threads);
			return EXIT_FAILURE;
		}
		tix++;
	}

	if (i2cParams.busName) {
		i2c_chanmap_add_ina219(&i2cParams, 0x40,  4);
		i2c_chanmap_add_ina219(&i2cParams, 0x41,  7);
		i2c_chanmap_add_ina219(&i2cParams, 0x42, 10);
		i2c_chanmap_add_ina219(&i2cParams, 0x43, 13);
		ltargs[tix].tag = "I2C";
		ltargs[tix].logQ = &log_queue;
		ltargs[tix].pstate = &state;
		ltargs[tix].dParams = &i2cParams;
		ltargs[tix].funcs = i2c_getCallbacks();
		ltargs[tix].funcs.startup(&ltargs[tix]);
		if (ltargs[tix].returnCode < 0) {
			log_error(&state, "Unable to set up I2C connection on %s", i2cParams.busName);
			free(gpsParams.portName);
			free(mpParams.portName);
			free(nmeaParams.portName);
			free(i2cParams.busName);
			free(monPrefix);
			free(stateName);
			free(threads);
			return EXIT_FAILURE;
		}
		tix++;

	}

	FILE *monitorFile = NULL;

	errno = 0;
	monitorFile = openSerialNumberedFile(monPrefix, "dat");

	if (monitorFile == NULL) {
		if (errno == EEXIST) {
			log_error(&state, "Unable to open data file - too many files created with this prefix today?");
		} else {
			log_error(&state, "Unable to open data file: %s", strerror(errno));
		}
		free(gpsParams.portName);
		free(mpParams.portName);
		free(nmeaParams.portName);
		free(i2cParams.busName);
		free(monPrefix);
		free(stateName);
		free(threads);
		return -1;
	}

	log_info(&state, 1, "Initialisation complete, starting log threads");

	for (int tix=0; tix < nThreads; tix++) {
		if (pthread_create(&(threads[tix]), NULL, ltargs[tix].funcs.logging, &(ltargs[tix])) != 0){
			log_error(&state, "Unable to launch %s thread", ltargs[tix].tag);
			free(gpsParams.portName);
			free(mpParams.portName);
			free(nmeaParams.portName);
			free(i2cParams.busName);
			free(monPrefix);
			free(stateName);
			free(threads);
			return EXIT_FAILURE;
		}

#ifdef _GNU_SOURCE
		char threadname[16] = {0};
		snprintf(threadname, 16, "Logger: %s", ltargs[tix].tag);
		pthread_setname_np(threads[tix], threadname);
#endif
		if (ltargs[tix].funcs.channels) {
			ltargs[tix].funcs.channels(&ltargs[tix]);
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
			if (rotateMonitor) {
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

			log_info(&state, 0, "Rotating log files");

			// "Monitor" / main data file rotation
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

			// Re-request channel names for the new files
			for (int tix=0; tix < nThreads; tix++) {
				if (ltargs[tix].funcs.channels) {
					ltargs[tix].funcs.channels(&ltargs[tix]);
				}
			}
			log_info(&state, 0, "%d messages read successfully - resetting count", msgCount);
			msgCount = 0;
			mon_yday = mon_nextyday;
			rotateNow = false;
		}

		// Check for waiting messages to be logged
		msg_t *res = queue_pop(&log_queue);
		if (res == NULL) {
			// No data waiting, so sleep for a little bit and go back around
			usleep(5 * SERIAL_SLEEP);
			continue;
		}
		msgCount++;
		mp_writeMessage(fileno(monitorFile), res);
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


	for (int tix=0; tix < nThreads; tix++) {
		ltargs[tix].funcs.shutdown(&(ltargs[tix]));
	}

	free(ltargs);
	free(threads);

	if (queue_count(&log_queue) > 0) {
		log_info(&state, 2, "Processing remaining queued messages");
		while (queue_count(&log_queue) > 0) {
			msg_t *res = queue_pop(&log_queue);
			msgCount++;
			msgCount++;
			mp_writeMessage(fileno(monitorFile), res);
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

/*!
 * Timespec subtraction, modified from the timeval example in the glibc manual
 *
 * @param[out] result X-Y
 * @param[in] x Input X
 * @param[in] y Input Y
 * @return true if difference is negative
 */
bool timespec_subtract(struct timespec *result, struct timespec *x, struct timespec *y) {
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_nsec < y->tv_nsec) {
		int fsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
		y->tv_nsec -= 1000000000 * fsec;
		y->tv_sec += fsec;
	}
	if ((x->tv_nsec - y->tv_nsec) > 1000000000) {
		int fsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
		y->tv_nsec += 1000000000 * fsec;
		y->tv_sec -= fsec;
	}

	/* Compute the time remaining to wait.
	tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_nsec = x->tv_nsec - y->tv_nsec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}
