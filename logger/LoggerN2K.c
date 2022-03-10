#include "Logger.h"

#include "LoggerN2K.h"
#include "LoggerSignals.h"

/*!
 * Set up connection to an N2K serial gateway device.
 *
 * No other steps required.
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *n2k_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	n2k_params *n2kInfo = (n2k_params *)args->dParams;

	n2kInfo->handle = n2k_openConnection(n2kInfo->portName, n2kInfo->baudRate);
	if (n2kInfo->handle < 0) {
		log_error(args->pstate, "[N2K:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}

	log_info(args->pstate, 2, "[N2K:%s] Connected", args->tag);
	args->returnCode = 0;
	return NULL;
}

/*!
 * Takes a n2k_params struct (passed via log_thread_args_t)
 * messages from a device configured with n2k_setup() and pushes them to the
 * message queue.
 *
 * Exits on error or when shutdown is signalled.
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *n2k_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	n2k_params *n2kInfo = (n2k_params *)args->dParams;

	log_info(args->pstate, 1, "[N2K:%s] Logging thread started", args->tag);

	uint8_t *buf = calloc(N2K_BUFF, sizeof(uint8_t));
	size_t n2k_index = 0;
	size_t n2k_hw = 0;
	while (!shutdownFlag) {
		n2k_act_message out = {0};
		if (n2k_act_readMessage_buf(n2kInfo->handle, &out, buf, &n2k_index, &n2k_hw)) {
			bool handled = false;

			if (!handled) {
				size_t mlen = 0;
				uint8_t *rd = NULL;
				if (!n2k_act_to_bytes(&out, &rd, &mlen)) {
					log_warning(
						args->pstate,
						"[NMEA:%s] Unable to serialise message (PGN %d, Source %d)",
						args->tag, out.PGN, out.src);
					if (rd) { free(rd); }
					continue;
				}
				msg_t *rm = NULL;
				rm = msg_new_bytes(n2kInfo->sourceNum, 3, mlen, rd);
				if (!queue_push(args->logQ, rm)) {
					log_error(args->pstate,
					          "[NMEA:%s] Error pushing message to queue",
					          args->tag);
					msg_destroy(rm);
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
			}
			// Do not destroy or free message here
			// After pushing it to the queue, it is the responsibility of the
			// consumer to dispose of it after use.
		} else {
			if (!(out.priority == 0xFF || out.priority == 0xFD ||
			      out.priority == 0xEE)) {
				// 0xFF, 0xFD and 0xEE are used to signal recoverable
				// states that resulted in no valid message.
				//
				// 0xFF and 0xFD indicate an out of data error, which is
				// not a problem for serial monitoring, but might indicate
				// EOF when reading from file
				//
				// 0xEE indicates an invalid message following valid sync
				// bytes
				log_error(args->pstate,
				          "[N2K:%s] Error signalled from n2k_readMessage_buf",
				          args->tag);
				args->returnCode = -2;
				free(buf);
				pthread_exit(&(args->returnCode));
			}
			// We've already exited (via pthread_exit) for error
			// cases, so at this point sleep briefly and wait for
			// more data
			usleep(SERIAL_SLEEP);
		}
	}
	free(buf);
	log_info(args->pstate, 1, "[N2K:%s] Logging thread exiting", args->tag);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Calls n2k_closeConnection(), which will do any cleanup required.
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *n2k_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	n2k_params *n2kInfo = (n2k_params *)args->dParams;

	if (n2kInfo->handle >= 0) { // Admittedly 0 is unlikely
		n2k_closeConnection(n2kInfo->handle);
	}
	n2kInfo->handle = -1;
	if (n2kInfo->sourceName) {
		free(n2kInfo->sourceName);
		n2kInfo->sourceName = NULL;
	}
	if (n2kInfo->portName) {
		free(n2kInfo->portName);
		n2kInfo->portName = NULL;
	}
	return NULL;
}

/*!
 * Populate list of channels and push to queue as a map message
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *n2k_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	n2k_params *n2kInfo = (n2k_params *)args->dParams;

	msg_t *m_sn = msg_new_string(n2kInfo->sourceNum, SLCHAN_NAME, strlen(n2kInfo->sourceName),
	                             n2kInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[N2K:%s] Error pushing channel name to queue", args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(5);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 8, "Raw N2K");
	sa_create_entry(channels, 4, 5, "");

	msg_t *m_cmap = msg_new_string_array(n2kInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[N2K:%s] Error pushing channel map to queue", args->tag);
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

device_callbacks n2k_getCallbacks() {
	device_callbacks cb = {.startup = &n2k_setup,
	                       .logging = &n2k_logging,
	                       .shutdown = &n2k_shutdown,
	                       .channels = &n2k_channels};
	return cb;
}

n2k_params n2k_getParams() {
	n2k_params gp = {
		.portName = NULL, .sourceNum = SLSOURCE_N2K, .baudRate = 115200, .handle = -1};
	return gp;
}

bool n2k_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[N2K:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	n2k_params *nmp = calloc(1, sizeof(n2k_params));
	if (!nmp) {
		log_error(lta->pstate, "[N2K:%s] Unable to allocate memory for device parameters",
		          lta->tag);
		return false;
	}
	(*nmp) = n2k_getParams();

	config_kv *t = NULL;
	if ((t = config_get_key(s, "port"))) { nmp->portName = config_qstrdup(t->value); }
	t = NULL;

	if ((t = config_get_key(s, "baud"))) {
		errno = 0;
		nmp->baudRate = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[N2K:%s] Error parsing baud rate: %s", lta->tag,
			          strerror(errno));
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "sourcenum"))) {
		errno = 0;
		int sn = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[N2K:%s] Error parsing source number: %s",
			          lta->tag, strerror(errno));
			return false;
		}
		if (sn < 0) {
			log_error(lta->pstate, "[N2K:%s] Invalid source number (%s)", lta->tag,
			          t->value);
			return false;
		}
		if (sn < 10) {
			nmp->sourceNum += sn;
		} else {
			nmp->sourceNum = sn;
			if (sn < SLSOURCE_N2K || sn > (SLSOURCE_N2K + 0x0F)) {
				log_warning(
					lta->pstate,
					"[N2K:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems",
					lta->tag, sn);
			}
		}
	}
	t = NULL;
	lta->dParams = nmp;
	return true;
}
