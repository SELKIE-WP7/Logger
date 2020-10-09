#include "Logger.h"
#include "LoggerMP.h"
#include "LoggerSignals.h"

int mp_setup(program_state *pstate, const char* mpPortName, const int baudRate) {
	int mpHandle = mp_openConnection(mpPortName, baudRate);
	if (mpHandle < 0) {
		log_error(pstate, "Unable to open a connection on port %s", mpPortName);
		return -1;
	}

	log_info(pstate, 1, "Connected MP Source on port %s", mpPortName);
	return mpHandle;
}

void *mp_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	log_info(args->pstate, 1, "MP Logging thread started");

	uint8_t *buf = calloc(MP_SERIAL_BUFF, sizeof(uint8_t));
	int mp_index = 0;
	int mp_hw = 0;
	while (!shutdownFlag) {
		// Needs to be on the heap as we'll be queuing it
		msg_t *out = calloc(1, sizeof(msg_t));
		if (mp_readMessage_buf(args->streamHandle, out, buf, &mp_index, &mp_hw)) {

			if (!queue_push(args->logQ, out)) {
				log_error(args->pstate, "Error pushing message to queue from MP source");
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
				log_error(args->pstate, "Error signalled from mp_readMessage");
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
			usleep(5E2);
		}
	}
	free(buf);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}
