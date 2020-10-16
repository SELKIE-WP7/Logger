#include "Logger.h"
#include "LoggerNMEA.h"
#include "LoggerSignals.h"


/*!
 * Perform initial setup of a u-blox NMEA device.
 *
 * It's not unusual for these devices to be configured for a lower baud rate on
 * startup, so connect at a lower rate and switch to something faster.
 *
 * Once the baud rate is configured, set up the messages we need and poll for
 * some status information for the log.
 */ 
void *nmea_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	nmea_params *nmeaInfo = (nmea_params *) args->dParams;

	nmeaInfo->handle = nmea_openConnection(nmeaInfo->portName, nmeaInfo->baudRate);
	if (nmeaInfo->handle < 0) {
		log_error(args->pstate, "[NMEA:%s] Unable to open a connection", nmeaInfo->portName);
		args->returnCode = -1;
		return NULL;
	}

	log_info(args->pstate, 2, "[NMEA:%s] Connected", nmeaInfo->portName);
	args->returnCode = 0;
	return NULL;
}

/*!
 * Takes a nmea_params struct (passed via log_thread_args_t) 
 * messages from a device configured with nmea_setup() and pushes them to the
 * message queue.
 *
 * Exits on error or when shutdown is signalled.
 */
void *nmea_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	nmea_params *nmeaInfo = (nmea_params *) args->dParams;

	log_info(args->pstate, 1, "[NMEA:%s] Logging thread started", nmeaInfo->portName);

	uint8_t *buf = calloc(NMEA_SERIAL_BUFF, sizeof(uint8_t));
	int nmea_index = 0;
	int nmea_hw = 0;
	while (!shutdownFlag) {
		nmea_msg_t out = {0};
		if (nmea_readMessage_buf(nmeaInfo->handle, &out, buf, &nmea_index, &nmea_hw)) {
			char *data = NULL;
			ssize_t len = nmea_flat_array(&out, &data);

			msg_t *sm = msg_new_bytes(nmeaInfo->sourceNum, 3, len, (uint8_t *)data);
			if (!queue_push(args->logQ, sm)) {
				log_error(args->pstate, "[NMEA:%s] Error pushing message to queue", nmeaInfo->portName);
				msg_destroy(sm);
				args->returnCode = -1;
				pthread_exit(&(args->returnCode));
			}
			if (data) {
				free(data); // Copied into message, so can safely free here
			}
			// Do not destroy or free sm here
			// After pushing it to the queue, it is the responsibility of the consumer
			// to dispose of it after use.
		} else {
			if (!(out.raw[0] == 0xFF || out.raw[0] == 0xFD || out.raw[0] == 0xEE)) {
				// 0xFF, 0xFD and 0xEE are used to signal recoverable states that resulted
				// in no valid message.
				// 
				// 0xFF and 0xFD indicate an out of data error, which is not a problem
				// for serial monitoring, but might indicate EOF when reading from file
				//
				// 0xEE indicates an invalid message following valid sync bytes
				log_error(args->pstate, "[NMEA:%s] Error signalled from nmea_readMessage_buf", nmeaInfo->portName);
				args->returnCode = -2;
				free(buf);
				sa_destroy(&(out.fields));
				pthread_exit(&(args->returnCode));
			}
			// We've already exited (via pthread_exit) for error
			// cases, so at this point sleep briefly and wait for
			// more data
			usleep(SERIAL_SLEEP);
		}

		// Clean up before next iteration
		sa_destroy(&(out.fields));
	}
	free(buf);
	log_info(args->pstate, 1, "[NMEA:%s] Logging thread exiting", nmeaInfo->portName);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Calls nmea_closeConnection(), which will do any cleanup required.
 */
void *nmea_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	nmea_params *nmeaInfo = (nmea_params *) args->dParams;
	if (nmeaInfo->handle >= 0) { // Admittedly 0 is unlikely
		nmea_closeConnection(nmeaInfo->handle);
	}
	nmeaInfo->handle = -1;
	return NULL;
}

/*!
 * Populate list of channels and push to queue as a map message
 */
void *nmea_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	nmea_params *nmeaInfo = (nmea_params *) args->dParams;

	strarray *channels = sa_new(4);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 8, "Raw NMEA");

	msg_t *m_cmap = msg_new_string_array(nmeaInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[NMEA:%s] Error pushing channel map to queue", nmeaInfo->portName);
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

device_callbacks nmea_getCallbacks() {
	device_callbacks cb = {
		.startup = &nmea_setup,
		.logging = &nmea_logging,
		.shutdown = &nmea_shutdown,
		.channels = &nmea_channels
	};
	return cb;
}

nmea_params nmea_getParams() {
	nmea_params gp = {
		.portName = NULL,
		.sourceNum = SLSOURCE_NMEA,
		.baudRate = 115200,
		.handle = -1
	};
	return gp;
}
