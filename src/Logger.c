#include "Logger.h"

/*!
 * @addtogroup Executables
 * @{
 * @file Logger.c
 *
 * @brief Generates main data logging executable
 *
 * Reads data from connected sensors, generates timestamps and writes the resulting messages to file for later conversion and analysis.
 * @}
 */

int main(int argc, char *argv[]) {
        char *monPrefix = NULL;
	char *stateName = NULL;

	program_state state = {0};
	state.verbose = 1;

	bool saveState = true;
	bool rotateMonitor = true;

	gps_params gpsParams = gps_getParams();
	mp_params mpParams = mp_getParams();
	nmea_params nmeaParams = nmea_getParams();
	i2c_params i2cParams = i2c_getParams();
	timer_params timerParams = timer_getParams();

        char *usage =  "Usage: %1$s [-v] [-q] [-f frequency] [-g port] [-n port] [-p port] [-i port] [-b initial baud] [-m file] [-R] [-s file | -S]\n"
                "\t-v\tIncrease verbosity\n"
		"\t-q\tDecrease verbosity\n"
		"\t-f\tMarker frequency\n"
                "\t-g port\tSpecify GPS port\n"
		"\t-n port\tNMEA source port\n"
		"\t-p port\tMP source port\n"
		"\t-b baud\tGPS initial baud rate\n"
		"\t-i port\tI2C Bus\n"
                "\t-m file\tMonitor file prefix\n"
                "\t-s file\tState file name\n"
		"\t-S\tDo not use state file\n"
		"\t-R\tDo not rotate monitor file\n"
                "\nVersion: " GIT_VERSION_STRING "\n"
		"Default options equivalent to:\n"
		"\t%1$s -f " DEFAULT_MARK_FREQ_STRING " -m " DEFAULT_MON_PREFIX " -s " DEFAULT_STATE_NAME "\n";

/*****************************************************************************
	Program Startup
*****************************************************************************/
	setvbuf(stdout, NULL, _IONBF, 0); // Unbuffered stdout = more accurate journalling/reporting

        opterr = 0; // Handle errors ourselves
        int go = 0;
	bool doUsage = false;
        while ((go = getopt (argc, argv, "vqb:f:g:i:m:n:p:s:SRL")) != -1) {
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
					log_error(&state, "Bad initial baud value ('%s')", optarg);
					doUsage = true;
				}
				break;
			case 'f':
				errno = 0;
				timerParams.frequency = strtol(optarg, NULL, 10);
				if (errno || timerParams.frequency < 1) {
					log_error(&state, "Bad marker frequency ('%s')", optarg);
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
				log_error(&state, "Unknown option `-%c'", optopt);
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
		ini_config conf = {0};
		if (!new_config(&conf)) {
			fprintf(stderr, "Failed to allocated new config\n");
		} else {
			int rv = ini_parse("config.ini", config_handler, &conf);
			fprintf(stderr, "=== INI TEST: %d\n\n", rv);
			print_config(&conf);
			destroy_config(&conf);
		}
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
	if (!monPrefix) {
		monPrefix = strdup(DEFAULT_MON_PREFIX);
	}

	// Default state file name is derived from the port name
	if (!stateName) {
		stateName = strdup(DEFAULT_STATE_NAME);
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

	FILE *monitorFile = NULL;
	char *monFileStem = NULL;

	errno = 0;
	log_info(&state, 1, "Using %s as output file prefix", monPrefix);
	monitorFile = openSerialNumberedFile(monPrefix, "dat", &monFileStem);

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
		return -1;
	}
	log_info(&state, 1, "Using data file %s.dat", monFileStem);

	errno = 0;
	{
		char *logFileName = NULL;
		if (asprintf(&logFileName, "%s.%s", monFileStem, "log") < 0) {
			log_error(&state, "Failed to allocate memory for log file name: %s", strerror(errno));
		}
		state.log = fopen(logFileName, "w+x");
		free(logFileName);
	}
	if (!state.log) {
		if (errno == EEXIST) {
			log_error(&state, "Unable to open log file. Log file and data file names out of sync?");
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
	state.logverbose = 3;
	log_info(&state, 2, "Using log file %s.log", monFileStem);

	errno = 0;
	FILE *varFile = NULL;
	{
		char *varFileName = NULL;
		if (asprintf(&varFileName, "%s.%s", monFileStem, "var") < 0) {
			log_error(&state, "Failed to allocate memory for variable file name: %s", strerror(errno));
		}
		varFile = fopen(varFileName, "w+x");
		free(varFileName);
	}
	if (!varFile) {
		if (errno == EEXIST) {
			log_error(&state, "Unable to open variable file. Variable file and data file names out of sync?");
		} else {
			log_error(&state, "Unable to open variable file: %s", strerror(errno));
		}
		free(gpsParams.portName);
		free(mpParams.portName);
		free(nmeaParams.portName);
		free(i2cParams.busName);
		free(monPrefix);
		free(stateName);
		return EXIT_FAILURE;
	}
	log_info(&state, 2, "Using variable file %s.var", monFileStem);

	log_info(&state, 1, "Version: " GIT_VERSION_STRING); // Preprocessor concatenation, not a format string!
	if (saveState) {
		log_info(&state, 1, "Using state file %s", stateName);
	}

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
	if (gpsParams.portName) { nThreads++;}
	if (mpParams.portName) { nThreads++;}
	if (nmeaParams.portName) { nThreads++;}
	if (i2cParams.busName) { nThreads++;}

	int tix = 0; // Thread Index
	log_thread_args_t *ltargs = calloc(nThreads, sizeof(log_thread_args_t));
	pthread_t *threads = calloc(nThreads, sizeof(pthread_t));

	ltargs[tix].tag = "Timer";
	ltargs[tix].logQ = &log_queue;
	ltargs[tix].pstate = &state;
	ltargs[tix].dParams = &timerParams;
	ltargs[tix].funcs = timer_getCallbacks();

	ltargs[tix].funcs.startup(&(ltargs[tix]));
	if (ltargs[tix].returnCode < 0) {
		log_error(&state, "Unable to set up Timers");
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

	if (gpsParams.portName) {
		ltargs[tix].tag = "GPS";
		ltargs[tix].logQ = &log_queue;
		ltargs[tix].pstate = &state;
		ltargs[tix].dParams = &gpsParams;
		ltargs[tix].funcs = gps_getCallbacks();

		ltargs[tix].funcs.startup(&(ltargs[tix]));
		if (ltargs[tix].returnCode < 0) {
			log_error(&state, "Unable to set up GPS connection on %s", gpsParams.portName);
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
	}

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

	if (!log_softwareVersion(&log_queue)) {
		log_error(&state, "Error pushing version message to queue");
		free(gpsParams.portName);
		free(mpParams.portName);
		free(nmeaParams.portName);
		free(i2cParams.busName);
		free(monPrefix);
		free(stateName);
		free(threads);
		return EXIT_FAILURE;
	}

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
			char *newMonFileStem = NULL;
			newMonitor  = openSerialNumberedFile(monPrefix, "dat", &newMonFileStem);

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
				free(monFileStem);
				monFileStem= newMonFileStem;
				log_info(&state, 2, "Using data file %s.dat", monFileStem);
			}

			// As above, but for the log file
			FILE *newLog = NULL;
			errno = 0;
			{
				char *logFileName = NULL;
				if (asprintf(&logFileName, "%s.%s", monFileStem, "log") < 0) {
					log_error(&state, "Failed to allocate memory for log file name: %s", strerror(errno));
				}
				newLog = fopen(logFileName, "w+x");
				free(logFileName);
			}
			if (newLog == NULL) {
				// As above, if the issue is log file names then we continue with the existing file
				if (errno == EEXIST) {
					log_error(&state, "Unable to open log file - mismatch between log files and data file names?");
				} else {
					log_error(&state, "Unable to open log file: %s", strerror(errno));
					return -1;
				}
			} else {
				FILE * oldLog = state.log;
				state.log = newLog;
				fclose(oldLog);
				log_info(&state, 2, "Using log file %s.log", monFileStem);
			}

			FILE *newVar = NULL;
			errno = 0;
			{
				char *varFileName = NULL;
				if (asprintf(&varFileName, "%s.%s", monFileStem, "var") < 0) {
					log_error(&state, "Failed to allocate memory for variable file name: %s", strerror(errno));
				}
				newVar = fopen(varFileName, "w+x");
				free(varFileName);
			}
			if (newVar == NULL) {
				// As above, if the issue is log file names then we continue with the existing file
				if (errno == EEXIST) {
					log_error(&state, "Unable to open variable file - mismatch between variable file and data file names?");
				} else {
					log_error(&state, "Unable to open variable file: %s", strerror(errno));
					return -1;
				}
			} else {
				// We're the only thread writing to the .var file, so fewer shenanigans required.
				fclose(varFile);
				log_info(&state, 2, "Using variable file %s.var", monFileStem);
				varFile = newVar;
			}

			// Re-request channel names for the new files
			for (int tix=0; tix < nThreads; tix++) {
				if (ltargs[tix].funcs.channels) {
					ltargs[tix].funcs.channels(&ltargs[tix]);
				}
			}

			if (!log_softwareVersion(&log_queue)) {
				log_error(&state, "Unable to push software version message to queue");
				return -1;
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
		if (res->type == SLCHAN_MAP || res->type == SLCHAN_NAME) {
			mp_writeMessage(fileno(varFile), res);
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
	free(monFileStem);
	log_info(&state, 2, "Monitor file closed");

	fclose(varFile);
	log_info(&state, 2, "Variable file closed");

	log_info(&state, 0, "%d messages read successfully\n\n", msgCount);
	fclose(state.log);
	free(stateName);

	/***
	 * While this isn't necessary for the program to run, it keeps Valgrind
	 * happy and makes it easier to spot real bugs and leaks
	 *
	 */
	free(gpsParams.portName);
	free(mpParams.portName);
	free(nmeaParams.portName);
	free(i2cParams.busName);
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

/*!
 * @brief Push software version into message queue
 * 
 * @param[in] q Log queue
 * @return True on success
 */

bool log_softwareVersion(msgqueue *q) {
	const char *version = "Logger version: " GIT_VERSION_STRING;
	msg_t *verMsg = msg_new_string(SLSOURCE_LOCAL, SLCHAN_LOG_INFO, strlen(version), version);
	if (!queue_push(q, verMsg)) {
		msg_destroy(verMsg);
		free(verMsg);
		return false;
	}
	return true;
}
