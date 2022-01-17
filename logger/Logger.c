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
	struct global_opts go = {0};

	program_state state = {0};
	state.verbose = 1;

	go.saveState = true;
	go.rotateMonitor = true;

	int verbosityModifier = 0;

        char *usage =  "Usage: %1$s [-v] [-q] <config file>\n"
                "\t-v\tIncrease verbosity\n"
		"\t-q\tDecrease verbosity\n"
		"\nSee documentation for configuration file format details\n"
                "\nVersion: " GIT_VERSION_STRING "\n";

/*****************************************************************************
	Program Startup
*****************************************************************************/
	setvbuf(stdout, NULL, _IONBF, 0); // Unbuffered stdout = more accurate journalling/reporting

        opterr = 0; // Handle errors ourselves
        int gov = 0;
	bool doUsage = false;
        while ((gov = getopt (argc, argv, "vq")) != -1) {
                switch(gov) {
			case 'v':
				verbosityModifier++;
				break;
			case 'q':
				verbosityModifier--;
				break;
			// TODO: Migrate all below to config file
			/*
			case 'b':
				errno = 0;
				gpsParams.initialBaud = strtol(optarg, NULL, 10);
				if (errno) {
					log_error(&state, "Bad initial baud value ('%s')", optarg);
					doUsage = true;
				}
				break;
			case 'c':
				if (go.configFileName) {
					log_error(&state, "Only a single configuration file can be specified");
				} else {
					go.configFileName = strdup(optarg);
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
				if (go.dataPrefix) {
					log_error(&state, "Only a single monitor file prefix can be specified");
					doUsage = true;
				} else {
					go.dataPrefix = strdup(optarg);
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
				if (go.stateName) {
					log_error(&state, "Only a single state file name can be specified");
					doUsage = true;
				} else {
					go.stateName = strdup(optarg);
				}
				break;
			case 'S':
				go.saveState = false;
				break;
			case 'R':
				go.rotateMonitor = false;
				break;
			*/
			case '?':
				log_error(&state, "Unknown option `-%c'", optopt);
				doUsage = true;
		}
        }

	// Should be 1 spare arguments - the configuration file name
        if (argc - optind != 1) {
		log_error(&state, "Invalid arguments");
		doUsage = true;
        }

	if (doUsage) {
		fprintf(stderr, usage, argv[0]);
		destroy_global_opts(&go);
		return EXIT_FAILURE;
	}

	go.configFileName = strdup(argv[optind]);

/***************************
	Extract global config options from "" section
****************************/

	ini_config conf = {0};
	if (!new_config(&conf)) {
		log_error(&state, "Failed to allocated new config");
		destroy_global_opts(&go);
		return EXIT_FAILURE;
	}

	log_info(&state, 1, "Reading configuration from file \"%s\"", go.configFileName);
	if (ini_parse(go.configFileName, config_handler, &conf)) {
		log_error(&state, "Unable to read configuration file");
		destroy_global_opts(&go);
		return EXIT_FAILURE;
	}

	config_section *def = config_get_section(&conf, "");
	if (def == NULL) {
		log_warning(&state, "No global configuration section found in file. Using defaults");
	} else {
		config_kv *kv = NULL;
		if ((kv = config_get_key(def, "verbose"))) {
			errno = 0;
			state.verbose = strtol(kv->value, NULL, 0);
			if (errno) {
				log_error(&state, "Error parsing verbosity: %s", strerror(errno));
				doUsage = true;
			}
		}

		kv = NULL;
		if ((kv = config_get_key(def, "frequency"))) {
			errno = 0;
			go.coreFreq = strtol(kv->value, NULL, 0);
			if (errno) {
				log_error(&state, "Error parsing core sample frequency: %s", strerror(errno));
				doUsage = true;
			}
		}

		kv = NULL;
		if ((kv = config_get_key(def, "prefix"))) {
			go.dataPrefix = strdup(kv->value);
		}

		kv = NULL;
		if ((kv = config_get_key(def, "statefile"))) {
			go.stateName = strdup(kv->value);
		}

		kv = NULL;
		if ((kv = config_get_key(def, "savestate"))) {
			int st = config_parse_bool(kv->value);
			if (st < 0) {
				log_error(&state, "Error parsing option savestate: %s", strerror(errno));
				doUsage = true;
			}
			go.saveState = st;
		}

		kv = NULL;
		if ((kv = config_get_key(def, "rotate"))) {
			int rm = config_parse_bool(kv->value);
			if (rm < 0) {
				log_error(&state, "Error parsing option rotate: %s", strerror(errno));
				doUsage = true;
			}
			go.rotateMonitor = rm;
		}
	}

	state.verbose += verbosityModifier;

	log_info(&state, 3, "**** Parsed configuration file follows: ");
	if (state.verbose >= 3) { print_config(&conf); }
	log_info(&state, 3, "**** Parsed configuration file ends ");

	// Check for conflicting options
	// Downgraded to a warning as part of the move to configuration files
	if (go.stateName && !go.saveState) {
		log_warning(&state, "State file name configured, but state file use disabled by configuration");
	}

	if (doUsage) {
		fprintf(stderr, usage, argv[0]);
		destroy_global_opts(&go);
		destroy_config(&conf);
		return EXIT_FAILURE;
	}

	// Set defaults if no argument provided
	// Defining the defaults as compiled in constants at the top of main() causes issues
	// with overwriting them later
	if (!go.dataPrefix) {
		go.dataPrefix = strdup(DEFAULT_MON_PREFIX);
	}

	// Default state file name is derived from the port name
	if (!go.stateName) {
		go.stateName = strdup(DEFAULT_STATE_NAME);
	}

	// Set default frequency if not already set
	if (!go.coreFreq) {
		go.coreFreq = DEFAULT_MARK_FREQUENCY;
	}

	// Per thread / individual source configuration happens after this global setup section
	log_info(&state, 3, "Core configuration completed");

/**********************************************************************************************
 * Set up various log files and mechanisms
 *********************************************************************************************/

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
	log_info(&state, 1, "Using %s as output file prefix", go.dataPrefix);
	go.monitorFile = openSerialNumberedFile(go.dataPrefix, "dat", &go.monFileStem);

	if (go.monitorFile == NULL) {
		if (errno == EEXIST) {
			log_error(&state, "Unable to open data file - too many files created with this prefix today?");
		} else {
			log_error(&state, "Unable to open data file: %s", strerror(errno));
		}
		destroy_global_opts(&go);
		return EXIT_FAILURE;
	}
	log_info(&state, 1, "Using data file %s.dat", go.monFileStem);

	errno = 0;
	{
		char *logFileName = NULL;
		if (asprintf(&logFileName, "%s.%s", go.monFileStem, "log") < 0) {
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
		destroy_config(&conf);
		destroy_global_opts(&go);
		destroy_program_state(&state);
		return EXIT_FAILURE;
	}
	state.logverbose = 3;
	log_info(&state, 2, "Using log file %s.log", go.monFileStem);

	errno = 0;
	{
		char *varFileName = NULL;
		if (asprintf(&varFileName, "%s.%s", go.monFileStem, "var") < 0) {
			log_error(&state, "Failed to allocate memory for variable file name: %s", strerror(errno));
		}
		go.varFile = fopen(varFileName, "w+x");
		free(varFileName);
	}
	if (!go.varFile) {
		if (errno == EEXIST) {
			log_error(&state, "Unable to open variable file. Variable file and data file names out of sync?");
		} else {
			log_error(&state, "Unable to open variable file: %s", strerror(errno));
		}
		destroy_config(&conf);
		destroy_global_opts(&go);
		destroy_program_state(&state);
		return EXIT_FAILURE;
	}
	log_info(&state, 2, "Using variable file %s.var", go.monFileStem);

	log_info(&state, 1, "Version: " GIT_VERSION_STRING); // Preprocessor concatenation, not a format string!
	if (go.saveState) {
		log_info(&state, 1, "Using state file %s", go.stateName);
	}

	// Block signal handling until we're up and running
	signalHandlersBlock();

	msgqueue log_queue = {0};
	if (!queue_init(&log_queue)) {
		log_error(&state, "Unable to initialise message queue");
		destroy_config(&conf);
		destroy_global_opts(&go);
		destroy_program_state(&state);
		return EXIT_FAILURE;
	}

/********************************************************************************************
 * Configure individual data sources, based on the configuration file sections
 * 	For each section:
 *		Allocate log_thread_args_t structure (lta)
 *		Set lta->tag to section name
 *		Set lta->logQ to queue
 *		Set lta->pstate to &state
 *		Identify device type
 *		Get callback functions and set lta->funcs
 *		If set, call lta->funcs->parse() with config section to populate lta->dparams
 *			parse() must allocate memory for dparams
********************************************************************************************/

	log_info(&state, 2, "Configuring data sources");

	// Set true for graceful exit at next convenient point during configuration
	bool nextExit = false;

	log_thread_args_t *ltargs = calloc(10, sizeof(log_thread_args_t));
	size_t ltaSize = 10;
	size_t nThreads = 0;

	if (!ltargs) {
		log_error(&state, "Unable to allocate ltargs");
		destroy_global_opts(&go);
		destroy_config(&conf);
		if (go.varFile) { fclose(go.varFile); }
		destroy_program_state(&state);
		return EXIT_FAILURE;
	}

	ltargs[nThreads].tag = strdup("Timer"); // Must be free-able
	ltargs[nThreads].logQ = &log_queue;
	ltargs[nThreads].pstate = &state;
	{
		timer_params *tp = calloc(1, sizeof(timer_params));
		if (!tp) {
			log_error(&state, "Unable to allocate timer parameters");
			nextExit = true;
		} else {
			(*tp) = timer_getParams();
			tp->frequency = go.coreFreq;
			ltargs[nThreads].dParams = tp;
		}
	}
	ltargs[nThreads].funcs = timer_getCallbacks();

	nThreads++;

	for (int i = 0; i < conf.numsects; ++i) {
		if (strcmp(conf.sects[i].name, "") == 0) {
			// The global/unlabelled section gets handled above
			continue;
		}
		log_info(&state, 3, "Found section: %s", conf.sects[i].name);
		ltargs[nThreads].tag = strdup(conf.sects[i].name);
		ltargs[nThreads].logQ = &log_queue;
		ltargs[nThreads].pstate = &state;
		// TODO: Device specific init
		config_kv *type = config_get_key(&(conf.sects[i]), "type");
		if (type == NULL) {
			log_error(&state, "Configuration - data source type not defined for \"%s\"", conf.sects[i].name);
			free(ltargs[nThreads].tag);
			ltargs[nThreads].tag = NULL;
			nextExit = true;
			continue;
		}
		ltargs[nThreads].type = strdup(type->value);
		ltargs[nThreads].funcs = dmap_getCallbacks(type->value);
		dc_parser dcp = dmap_getParser(type->value);
		if (dcp == NULL) {
			log_error(&state, "Configuration - no parser available for \"%s\" (%s)", conf.sects[i].name, type->value);
			free(ltargs[nThreads].tag);
			free(ltargs[nThreads].type);
			ltargs[nThreads].tag = NULL;
			ltargs[nThreads].type = NULL;
			nextExit = true;
			continue;
		}
		if (!dcp(&(ltargs[nThreads]), &(conf.sects[i]))) {
			log_error(&state, "Configuration - parser failed for \"%s\" (%s)", conf.sects[i].name, type->value);
			free(ltargs[nThreads].tag);
			free(ltargs[nThreads].type);
			free(ltargs[nThreads].dParams);
			ltargs[nThreads].tag = NULL;
			ltargs[nThreads].type = NULL;
			ltargs[nThreads].dParams = NULL;
			nextExit = true;
			continue;
		}
		nThreads++;
		if (nThreads >= ltaSize) {
			log_thread_args_t *ltt = realloc(ltargs, (ltaSize+10)*sizeof(log_thread_args_t));
			if (!ltt) {
				log_error(&state, "Unable to reallocate ltargs structure: %s", strerror(errno));
				return EXIT_FAILURE;
			}
			ltaSize += 10;
			ltargs = ltt;
		}
	}

	destroy_config(&conf);

	if (nextExit) {
		for (int i = 0; i < nThreads; i++) {
			if (ltargs[i].tag) {free(ltargs[i].tag);}
			if (ltargs[i].type) {free(ltargs[i].type);}
			if (ltargs[i].dParams) {free(ltargs[i].dParams);}
		}
		free(ltargs);
		state.shutdown = true;
		log_error(&state, "Failed to complete configuration successfully - exiting.");
		destroy_global_opts(&go);
		destroy_program_state(&state);
		return EXIT_FAILURE;
	}
	log_info(&state, 2, "Data source configuration complete");
	log_info(&state, 2, "Initialising threads");

	pthread_t *threads = calloc(nThreads, sizeof(pthread_t));
	if (!threads) {
		log_error(&state, "Unable to allocate threads: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	for (int tix=0; tix < nThreads; tix++) {
		ltargs[tix].funcs.startup(&(ltargs[tix]));
		if (ltargs[tix].returnCode < 0) {
			log_error(&state, "Unable to set up \"%s\"", ltargs[tix].tag);
			nextExit = true;
			break;
		}
	}

	if (nextExit) {
		for (int i = 0; i < nThreads; i++) {
			if (ltargs[i].tag) {free(ltargs[i].tag);}
			if (ltargs[i].type) {free(ltargs[i].type);}
			if (ltargs[i].dParams) {free(ltargs[i].dParams);}
		}
		free(ltargs);
		state.shutdown = true;
		log_error(&state, "Failed to initialise all data sources successfully - exiting.");
		destroy_global_opts(&go);
		destroy_program_state(&state);
		free(threads);
		return EXIT_FAILURE;
	}

	log_info(&state, 1, "Initialisation complete, starting log threads");

	for (int tix=0; tix < nThreads; tix++) {
		if (!ltargs[tix].funcs.logging) {
			log_error(&state, "Unable to launch thread %s - no logging function provided", ltargs[tix].tag);
			nextExit = true;
			shutdownFlag = true; // Ensure threads aware
			for (int it = tix - 1; it >= 0; --it) {
				if (it < 0) { break;}
				pthread_join(threads[it], NULL);
				if (ltargs[it].returnCode != 0) {
					log_error(&state, "Thread %d has signalled an error: %d", it, ltargs[it].returnCode);
				}
			}
			break;
		}
		if (pthread_create(&(threads[tix]), NULL, ltargs[tix].funcs.logging, &(ltargs[tix])) != 0){
			log_error(&state, "Unable to launch %s thread", ltargs[tix].tag);
			nextExit = true;
			shutdownFlag = true; // Ensure threads aware
			for (int it = tix - 1; it >= 0; --it) {
				if (it < 0) { break;}
				pthread_join(threads[it], NULL);
				if (ltargs[it].returnCode != 0) {
					log_error(&state, "Thread %d has signalled an error: %d", it, ltargs[it].returnCode);
				}
			}
			break;
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

	if (nextExit) {
		for (int i = 0; i < nThreads; i++) {
			if (ltargs[i].tag) {free(ltargs[i].tag);}
			if (ltargs[i].type) {free(ltargs[i].type);}
			if (ltargs[i].dParams) {free(ltargs[i].dParams);}
		}
		free(ltargs);
		state.shutdown = true;
		log_error(&state, "Failed to start all data sources - exiting.");
		destroy_global_opts(&go);
		destroy_program_state(&state);
		free(threads);
		return EXIT_FAILURE;
	}

	state.started = true;
	fflush(stdout);
	log_info(&state, 1, "Startup complete");

	if (!log_softwareVersion(&log_queue)) {
		log_error(&state, "Error pushing version message to queue");
		for (int i = 0; i < nThreads; i++) {
			if (ltargs[i].tag) {free(ltargs[i].tag);}
			if (ltargs[i].type) {free(ltargs[i].type);}
			if (ltargs[i].dParams) {free(ltargs[i].dParams);}
		}
		free(ltargs);
		destroy_global_opts(&go);
		destroy_program_state(&state);
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
			if (go.rotateMonitor) {
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
			 * Note that we don't check the go.rotateMonitor flag here
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
			newMonitor  = openSerialNumberedFile(go.dataPrefix, "dat", &newMonFileStem);

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
				fclose(go.monitorFile);
				go.monitorFile = newMonitor;
				free(go.monFileStem);
				go.monFileStem= newMonFileStem;
				log_info(&state, 2, "Using data file %s.dat", go.monFileStem);
			}

			// As above, but for the log file
			FILE *newLog = NULL;
			errno = 0;
			{
				char *logFileName = NULL;
				if (asprintf(&logFileName, "%s.%s", go.monFileStem, "log") < 0) {
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
				log_info(&state, 2, "Using log file %s.log", go.monFileStem);
			}

			FILE *newVar = NULL;
			errno = 0;
			{
				char *varFileName = NULL;
				if (asprintf(&varFileName, "%s.%s", go.monFileStem, "var") < 0) {
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
				fclose(go.varFile);
				log_info(&state, 2, "Using variable file %s.var", go.monFileStem);
				go.varFile = newVar;
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
		if (!mp_writeMessage(fileno(go.monitorFile), res)) {
			log_error(&state, "Unable to write out data to log file: %s", strerror(errno));
			return -1;
		}
		if (res->type == SLCHAN_MAP || res->type == SLCHAN_NAME) {
			mp_writeMessage(fileno(go.varFile), res);
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
			log_error(&state, "Thread %d (%s) has signalled an error: %d", it, ltargs[it].tag, ltargs[it].returnCode);
		}
	}


	for (int tix=0; tix < nThreads; tix++) {
		ltargs[tix].funcs.shutdown(&(ltargs[tix]));
	}

	for (int i = 0; i < nThreads; i++) {
		if (ltargs[i].tag) {free(ltargs[i].tag);}
		if (ltargs[i].type) {free(ltargs[i].type);}
		if (ltargs[i].dParams) {free(ltargs[i].dParams);}
	}
	free(ltargs);
	free(threads);

	if (queue_count(&log_queue) > 0) {
		log_info(&state, 2, "Processing remaining queued messages");
		while (queue_count(&log_queue) > 0) {
			msg_t *res = queue_pop(&log_queue);
			msgCount++;
			msgCount++;
			mp_writeMessage(fileno(go.monitorFile), res);
			msg_destroy(res);
			free(res);
		}
		log_info(&state, 2, "Queue emptied");
	}
	queue_destroy(&log_queue);
	log_info(&state, 2, "Message queue destroyed");

	fclose(go.monitorFile);
	free(go.monFileStem);
	go.monitorFile = NULL;
	go.monFileStem = NULL;
	log_info(&state, 2, "Monitor file closed");

	fclose(go.varFile);
	go.varFile = NULL;
	log_info(&state, 2, "Variable file closed");

	log_info(&state, 0, "%d messages read successfully\n\n", msgCount);
	fclose(state.log);
	state.log = NULL;

	/***
	 * While this isn't necessary for the program to run, it keeps Valgrind
	 * happy and makes it easier to spot real bugs and leaks
	 *
	 */
	destroy_program_state(&state);
	destroy_global_opts(&go);
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

void destroy_global_opts(struct global_opts *go) {
	if (go->configFileName) { free(go->configFileName); }
	if (go->dataPrefix) { free(go->dataPrefix); }
	if (go->stateName) { free(go->stateName); }
	if (go->monFileStem) { free(go->monFileStem); }

	go->configFileName = NULL;
	go->dataPrefix = NULL;
	go->stateName = NULL;
	go->monFileStem = NULL;

	if (go->monitorFile) { fclose(go->monitorFile); }
	if (go->varFile) { fclose(go->varFile); }

	go->monitorFile = NULL;
	go->varFile = NULL;
}
