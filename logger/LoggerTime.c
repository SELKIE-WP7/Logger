#include "LoggerTime.h"
#include "Logger.h"
#include "LoggerSignals.h"

void *timer_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	timer_params *timerInfo = (timer_params *)args->dParams;

	if (timerInfo->sourceName == NULL) {
		if (timerInfo->sourceNum == SLSOURCE_TIMER) {
			// Special case the default clock
			timerInfo->sourceName = strdup("Internal");
		} else {
			log_error(args->pstate, "[Timer:Unknown] No source name specified!");
		}
	}
	return NULL;
}

void *timer_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	timer_params *timerInfo = (timer_params *)args->dParams;

	log_info(args->pstate, 1, "[Timer:%s] Logging thread started", args->tag);

	const int incr_nsec = (1E9 / timerInfo->frequency);
	time_t lstamp = 0;
	while (!shutdownFlag) {
		struct timespec now = {0};
		clock_gettime(CLOCK_MONOTONIC, &now);
		// Millisecond precision timestamp, but arbitrary reference point
		msg_t *msg = msg_new_timestamp(timerInfo->sourceNum, SLCHAN_TSTAMP,
		                               (1000 * now.tv_sec + now.tv_nsec / 1000000));
		if (!queue_push(args->logQ, msg)) {
			log_error(args->pstate, "[Timer:%s] Error pushing message to queue",
			          args->tag);
			msg_destroy(msg);
			args->returnCode = -1;
			pthread_exit(&(args->returnCode));
		}

		time_t nstamp = time(NULL);
		if (nstamp != lstamp) {
			// Unix Epoch referenced timestamp
			msg_t *epoch_msg = msg_new_timestamp(timerInfo->sourceNum, 4, nstamp);
			if (!queue_push(args->logQ, epoch_msg)) {
				log_error(args->pstate,
				          "[Timer:%s] Error pushing message to queue", args->tag);
				msg_destroy(epoch_msg);
				args->returnCode = -1;
				pthread_exit(&(args->returnCode));
			}
			lstamp = nstamp;
		}

		struct timespec nextIter = now;
		nextIter.tv_nsec += incr_nsec;
		if (nextIter.tv_nsec >= 1E9) {
			int nsec = nextIter.tv_nsec / 1000000000;
			nextIter.tv_nsec = nextIter.tv_nsec % 1000000000;
			nextIter.tv_sec += nsec;
		}
		nextIter.tv_nsec -= (nextIter.tv_nsec % incr_nsec);

		struct timespec target = {0};
		clock_gettime(CLOCK_MONOTONIC, &now);
		if (timespec_subtract(&target, &nextIter, &now)) {
			// Target has passed! Update ASAP and continue
			log_warning(args->pstate, "[Timer:%s] Deadline missed", args->tag);
			continue;
		} else {
			nanosleep(&target, NULL);
		}
	}
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

void *timer_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	timer_params *timerInfo = (timer_params *)args->dParams;

	if (timerInfo->sourceName) {
		free(timerInfo->sourceName);
		timerInfo->sourceName = NULL;
	}
	return NULL;
}

/*!
 * Populate list of channels and push to queue as a map message
 */
void *timer_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	timer_params *timerInfo = (timer_params *)args->dParams;

	msg_t *m_sn = msg_new_string(timerInfo->sourceNum, SLCHAN_NAME,
	                             strlen(timerInfo->sourceName), timerInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[Timer:%s] Error pushing channel name to queue",
		          args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(5); // 0,1,2 and 4
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, 4, 5, "Epoch");

	msg_t *m_cmap = msg_new_string_array(timerInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[Timer:%s] Error pushing channel map to queue",
		          args->tag);
		msg_destroy(m_cmap);
		sa_destroy(channels);
		free(channels);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	sa_destroy(channels);
	free(channels);
	return NULL;
}

device_callbacks timer_getCallbacks() {
	device_callbacks cb = {.startup = &timer_setup,
	                       .logging = &timer_logging,
	                       .shutdown = &timer_shutdown,
	                       .channels = &timer_channels};
	return cb;
}

timer_params timer_getParams() {
	timer_params timer = {
		.sourceNum = SLSOURCE_TIMER,
		.sourceName = NULL,
		.frequency = DEFAULT_MARK_FREQUENCY,
	};
	return timer;
}

bool timer_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[Timer:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	timer_params *tp = calloc(1, sizeof(timer_params));
	if (!tp) {
		log_error(lta->pstate,
		          "[Timer:%s] Unable to allocate memory for device parameters", lta->tag);
		return false;
	}
	(*tp) = timer_getParams();

	config_kv *t = NULL;
	if ((t = config_get_key(s, "name"))) {
		tp->sourceName = config_qstrdup(t->value);
	} else {
		// Must set a name, so nick the tag value
		tp->sourceName = strdup(lta->tag);
	}
	t = NULL;

	if ((t = config_get_key(s, "frequency"))) {
		errno = 0;
		tp->frequency = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[Timer:%s] Error parsing frequency: %s", lta->tag,
			          strerror(errno));
			return false;
		}
		if (tp->frequency <= 0) {
			log_error(
				lta->pstate,
				"[Timer:%s] Invalid frequency requested (%d) - must be positive and non-zero",
				lta->tag, tp->frequency);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "sourcenum"))) {
		errno = 0;
		int sn = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[Timer:%s] Error parsing source number: %s",
			          lta->tag, strerror(errno));
			return false;
		}
		if (sn < 0) {
			log_error(lta->pstate, "[Timer:%s] Invalid source number (%s)", lta->tag,
			          t->value);
			return false;
		}
		if (sn < 10) {
			tp->sourceNum += sn;
		} else {
			tp->sourceNum = sn;
			if (sn <= SLSOURCE_TIMER || sn > (SLSOURCE_TIMER + 0x0F)) {
				log_warning(
					lta->pstate,
					"[Timer:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems",
					lta->tag, sn);
			}
		}
	}
	t = NULL;
	lta->dParams = tp;
	return true;
}
