#include "Logger.h"
#include "LoggerMP.h"
#include "LoggerSignals.h"

/*!
 * Uses mp_openConnection to connect to a device providing messge pack formatted data.
 * It is assumed that no other setup is required for these devices.
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *mp_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	mp_params *mpInfo = (mp_params *) args->dParams;

	mpInfo->handle = mp_openConnection(mpInfo->portName, mpInfo->baudRate);
	if (mpInfo->handle < 0) {
		log_error(args->pstate, "[MP:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}

	log_info(args->pstate, 2, "[MP:%s] Connected", args->tag);
	args->returnCode = 0;
	return NULL;
}

/*!
 * Reads messages from the serial connection established by mp_setup(), and pushes them to the queue.
 * As messages are already in the right format, no further processing is done here.
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *mp_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	mp_params *mpInfo = (mp_params *) args->dParams;

	log_info(args->pstate, 1, "[MP:%s] Logging thread started", args->tag);

	uint8_t *buf = calloc(MP_SERIAL_BUFF, sizeof(uint8_t));
	int mp_index = 0;
	int mp_hw = 0;
	while (!shutdownFlag) {
		// Needs to be on the heap as we'll be queuing it
		msg_t *out = calloc(1, sizeof(msg_t));
		if (mp_readMessage_buf(mpInfo->handle, out, buf, &mp_index, &mp_hw)) {

			if (!queue_push(args->logQ, out)) {
				log_error(args->pstate, "[MP:%s] Error pushing message to queue", args->tag);
				msg_destroy(out);
				args->returnCode = -1;
				pthread_exit(&(args->returnCode));
			}
			// Do not destroy or free out here
			// After pushing it to the queue, it is the responsibility of the consumer
			// to dispose of it after use.
		} else {
			if (out->dtype == MSG_ERROR &&
				!(out->data.value == 0xFF || out->data.value == 0xFD || out->data.value == 0xEE )) {

				// 0xFF, 0xFD and 0xEE are used to signal recoverable states that resulted
				// in no valid message.
				// 
				// 0xFF and 0xFD indicate an out of data error, which is not a problem
				// for serial monitoring, but might indicate EOF when reading from file
				//
				// 0xEE indicates an invalid message following valid sync bytes
				log_error(args->pstate, "[MP:%s] Error signalled from mp_readMessage_buf", args->tag);
				free(buf);
				free(out);
				args->returnCode = -2;
				pthread_exit(&(args->returnCode));
			}
			// out was allocated but not pushed to the queue, so free it here.
			msg_destroy(out);
			free(out);

			// We've already exited (via pthread_exit) for error
			// cases, so at this point sleep briefly and wait for
			// more data
			usleep(SERIAL_SLEEP);
		}
	}
	free(buf);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Simple wrapper around mp_closeConnection(), which will do any cleanup required.
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *mp_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	mp_params *mpInfo = (mp_params *) args->dParams;
	if (mpInfo->handle >= 0) { // Admittedly 0 is unlikely
		mp_closeConnection(mpInfo->handle);
	}
	mpInfo->handle = -1;
	if (mpInfo->portName) {
		free(mpInfo->portName);
		mpInfo->portName = NULL;
	}
	return NULL;
}

device_callbacks mp_getCallbacks() {
	device_callbacks cb = {
		.startup = &mp_setup,
		.logging = &mp_logging,
		.shutdown = &mp_shutdown,
		.channels = NULL // No channel map - will be supplied by connected device(s)
	};
	return cb;
}

mp_params mp_getParams() {
	mp_params mp = {
		.portName = NULL,
		.baudRate = 115200,
		.handle = -1
	};
	return mp;
}


bool mp_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[MP:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	mp_params *mp = calloc(1, sizeof(mp_params));
	if (!mp) {
		log_error(lta->pstate, "[MP:%s] Unable to allocate memory for device parameters", lta->tag);
		return false;
	}
	(*mp) = mp_getParams();

	config_kv *t = NULL;
	if ((t = config_get_key(s, "port"))) {
		mp->portName = config_qstrdup(t->value);
	}
	t = NULL;

	if ((t = config_get_key(s, "baud"))) {
		errno = 0;
		mp->baudRate = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[MP:%s] Error parsing baud rate: %s", lta->tag, strerror(errno));
			return false;
		}
	}
	t = NULL;

	lta->dParams = mp;
	return true;
}
