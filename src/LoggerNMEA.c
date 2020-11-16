#include "Logger.h"
#include "LoggerNMEA.h"
#include "LoggerSignals.h"


/*!
 * Set up connection to an NMEA serial gateway device.
 *
 * No other steps required.
 *
 * @param ptargs Pointer to log_thread_args_t
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
 *
 * @param ptargs Pointer to log_thread_args_t
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
			bool handled = false;

			if ((strncmp(out.talker, "II", 2) == 0) && (strncmp(out.message, "ZDA", 3) == 0)) {
				struct tm *t = nmea_parse_zda(&out);
				if (t != NULL) {
					time_t epoch = mktime(t) - t->tm_gmtoff;
					if (epoch != (time_t)(-1)) {
						msg_t *tm = msg_new_timestamp(nmeaInfo->sourceNum, 4, epoch);
						if (!queue_push(args->logQ, tm)) {
							log_error(args->pstate, "[NMEA:%s] Error pushing message to queue", nmeaInfo->portName);
							msg_destroy(tm);
							free(t);
							args->returnCode = -1;
							pthread_exit(&(args->returnCode));
						}
						handled = true; // Suppress ZDA messages
					}

				}
			}
			if (!handled) {
				msg_t *sm = msg_new_bytes(nmeaInfo->sourceNum, 3, len, (uint8_t *)data);
				if (!queue_push(args->logQ, sm)) {
					log_error(args->pstate, "[NMEA:%s] Error pushing message to queue", nmeaInfo->portName);
					msg_destroy(sm);
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
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
 *
 * @param ptargs Pointer to log_thread_args_t
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
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *nmea_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	nmea_params *nmeaInfo = (nmea_params *) args->dParams;

	msg_t *m_sn = msg_new_string(nmeaInfo->sourceNum, SLCHAN_NAME, strlen(args->tag), args->tag);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[NMEA:%s] Error pushing channel name to queue", nmeaInfo->portName);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(5);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 8, "Raw NMEA");
	sa_create_entry(channels, 4, 5, "Epoch");

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
