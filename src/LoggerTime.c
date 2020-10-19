#include "Logger.h"
#include "LoggerTime.h"
#include "LoggerSignals.h"

void *timer_setup(void *ptargs) {
	return NULL;
}

void *timer_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	timer_params *timerInfo = (timer_params *) args->dParams;

	log_info(args->pstate, 1, "[Timer] Logging thread started");

	int lc = 0;
	struct timespec lastIter = {0};
	clock_gettime(CLOCK_MONOTONIC, &lastIter);
	while (!shutdownFlag) {
		// Millisecond precision timestamp, but arbitrary reference point
		msg_t *msg = msg_new_timestamp(timerInfo->sourceNum, SLCHAN_TSTAMP, (1000 * lastIter.tv_sec + lastIter.tv_nsec * 1000000));
		if (!queue_push(args->logQ, msg)) {
			log_error(args->pstate, "[Timer] Error pushing message to queue");
			msg_destroy(msg);
			args->returnCode = -1;
			pthread_exit(&(args->returnCode));
		}

		if ((lc % 10) == 0) {
			// Unix Epoch referenced timestamp
			msg_t *epoch_msg = msg_new_timestamp(timerInfo->sourceNum, 4, time(NULL));
			if (!queue_push(args->logQ, epoch_msg)) {
				log_error(args->pstate, "[Timer] Error pushing message to queue");
				msg_destroy(epoch_msg);
				args->returnCode = -1;
				pthread_exit(&(args->returnCode));
			}
			lc = 0;
		}
		lc++;

		lastIter.tv_nsec += 1E9 / timerInfo->frequency;
		if (lastIter.tv_nsec > 1E9) {
			int nsec = lastIter.tv_nsec / 1000000000;
			lastIter.tv_nsec = lastIter.tv_nsec % 1000000000;
			lastIter.tv_sec += nsec;
		}
		struct timespec now = {0};
		clock_gettime(CLOCK_MONOTONIC, &now);
		struct timespec target = {0};
		if (timespec_subtract(&target, &lastIter, &now)) {
			// Target has passed!
			clock_gettime(CLOCK_MONOTONIC, &lastIter);
		} else {
			clock_gettime(CLOCK_MONOTONIC, &lastIter);
			// If we're interrupted, just carry on. We might need to handle
			// the shutdown flag, and worst case is we log a bit of extra data
			nanosleep(&target, NULL);
		}
	}
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

void *timer_shutdown(void *ptargs) {
	return NULL;
}

/*!
 * Populate list of channels and push to queue as a map message
 */
void *timer_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	timer_params *timerInfo = (timer_params *) args->dParams;

	msg_t *m_sn = msg_new_string(timerInfo->sourceNum, SLCHAN_NAME, 6, "Timers");

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[Timers] Error pushing channel name to queue");
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(4);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, 4, 5, "Epoch");

	msg_t *m_cmap = msg_new_string_array(timerInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[Timers] Error pushing channel map to queue");
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
	device_callbacks cb = {
		.startup = &timer_setup,
		.logging = &timer_logging,
		.shutdown = &timer_shutdown,
		.channels = &timer_channels
	};
	return cb;
}

timer_params timer_getParams() {
	timer_params timer = {
		.sourceNum = SLSOURCE_LOCAL,
		.frequency = 10,
	};
	return timer;
}
