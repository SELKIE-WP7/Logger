#include "Logger.h"
#include "LoggerI2C.h"
#include "LoggerSignals.h"

void *i2c_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	i2c_params *i2cInfo = (i2c_params *) args->dParams;

	i2cInfo->handle = i2c_openConnection(i2cInfo->busName);
	if (i2cInfo->handle < 0) {
		log_error(args->pstate, "[I2C:%s] Unable to open a connection", i2cInfo->busName);
		args->returnCode = -1;
		return NULL;
	}

	log_info(args->pstate, 2, "[I2C:%s] Connected", i2cInfo->busName);
	args->returnCode = 0;
	return NULL;
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

void *i2c_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	i2c_params *i2cInfo = (i2c_params *) args->dParams;

	log_info(args->pstate, 1, "[I2C:%s] Logging thread started", i2cInfo->busName);

	struct timespec lastIter = {0};
	clock_gettime(CLOCK_MONOTONIC, &lastIter);
	while (!shutdownFlag) {
		for (int mm = 0; mm < i2cInfo->en_count; mm++) {
			i2c_msg_map *map = &(i2cInfo->chanmap[mm]);
			msg_t *msg = msg_new_float(i2cInfo->sourceNum, map->messageID, map->func(i2cInfo->handle, map->deviceAddr));
			if (!queue_push(args->logQ, msg)) {
				log_error(args->pstate, "[I2C:%s] Error pushing message to queue", i2cInfo->busName);
				msg_destroy(msg);
				args->returnCode = -1;
				pthread_exit(&(args->returnCode));
			}
		}

		lastIter.tv_nsec += 1E9 / i2cInfo->frequency;
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

/*!
 * Simple wrapper around i2c_closeConnection(), which will do any cleanup required.
 */
void *i2c_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	i2c_params *i2cInfo = (i2c_params *) args->dParams;
	if (i2cInfo->handle >= 0) { // Admittedly 0 is unlikely
		i2c_closeConnection(i2cInfo->handle);
	}
	i2cInfo->handle = -1;
	return NULL;
}

device_callbacks i2c_getCallbacks() {
	device_callbacks cb = {
		.startup = &i2c_setup,
		.logging = &i2c_logging,
		.shutdown = &i2c_shutdown
	};
	return cb;
}

i2c_params i2c_getParams() {
	i2c_params i2c = {
		.busName = NULL,
		.sourceNum = SLSOURCE_I2C,
		.handle = -1,
		.frequency = 10,
		.en_count = 0,
		.chanmap = NULL
	};
	return i2c;
}
