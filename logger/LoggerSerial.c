#include "Logger.h"

#include "LoggerSerial.h"
#include "LoggerSignals.h"

/*!
 * Opens a serial connection and the required baud rate.
 *
 * It is assumed that no other setup is required for these devices.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Exit code in ptargs->returnCode if required
 */
void *rx_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	rx_params *rxInfo = (rx_params *)args->dParams;

	rxInfo->handle = openSerialConnection(rxInfo->portName, rxInfo->baudRate);
	if (rxInfo->handle < 0) {
		log_error(args->pstate, "[Serial:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}

	log_info(args->pstate, 2, "[Serial:%s] Connected", args->tag);
	args->returnCode = 0;
	return NULL;
}

/*!
 * Reads messages from the connection established by rx_setup(), and pushes them to the
 * queue. Data is not interpreted, just pushed into the queue with suitable headers.
 *
 * Message size is variable, based on min/max limits and the amount of data
 * available to read from the source.
 *
 * Terminates thread in case of error.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Exit code in ptargs->returnCode if required
 */
void *rx_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	rx_params *rxInfo = (rx_params *)args->dParams;

	log_info(args->pstate, 1, "[Serial:%s] Logging thread started", args->tag);

	uint8_t *buf = calloc(rxInfo->maxBytes, sizeof(uint8_t));
	int rx_hw = 0;
	while (!shutdownFlag) {
		// Need to implement a message handling loop here
		int ti = 0;
		if (rx_hw < rxInfo->maxBytes - 1) {
			errno = 0;
			ti = read(rxInfo->handle, &(buf[rx_hw]), rxInfo->maxBytes - rx_hw);
			if (ti >= 0) {
				rx_hw += ti;
			} else {
				if (errno != EAGAIN) {
					log_error(
						args->pstate,
						"[Serial:%s] Unexpected error while reading from serial port (%s)",
						args->tag, strerror(errno));
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
			}
		}

		if (rx_hw < rxInfo->minBytes) {
			// Sleep briefly, then loop until we have more than the minimum
			// number of bytes available
			usleep(1E6 / rxInfo->pollFreq);
			continue;
		}

		msg_t *sm = msg_new_bytes(rxInfo->sourceNum, 3, rx_hw, buf);
		if (!queue_push(args->logQ, sm)) {
			log_error(args->pstate, "[Serial:%s] Error pushing message to queue",
			          args->tag);
			msg_destroy(sm);
			args->returnCode = -1;
			pthread_exit(&(args->returnCode));
		}
		rx_hw = 0;
		memset(buf, 0, rxInfo->maxBytes);
	}
	free(buf);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Simple wrapper around rx_closeConnection(), which will do any cleanup required.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Exit code in ptargs->returnCode if required
 */
void *rx_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	rx_params *rxInfo = (rx_params *)args->dParams;

	if (rxInfo->handle >= 0) { // Admittedly 0 is unlikely
		close(rxInfo->handle);
	}
	rxInfo->handle = -1;
	if (rxInfo->portName) {
		free(rxInfo->portName);
		rxInfo->portName = NULL;
	}
	return NULL;
}

/*!
 * @returns device_callbacks for serial sources
 */
device_callbacks rx_getCallbacks() {
	device_callbacks cb = {.startup = &rx_setup,
	                       .logging = &rx_logging,
	                       .shutdown = &rx_shutdown,
	                       .channels = &rx_channels};
	return cb;
}

/*!
 * @returns Default parameters for serial sources
 */
rx_params rx_getParams() {
	rx_params mp = {.portName = NULL,
	                .baudRate = 115200,
	                .handle = -1,
	                .minBytes = 10,
	                .maxBytes = 1024,
	                .pollFreq = 10};
	return mp;
}

/*!
 * Populate list of channels and push to queue as a map message
 *
 * Exits thread in case of error.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Exit code in ptargs->returnCode if required
 */
void *rx_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	rx_params *rxInfo = (rx_params *)args->dParams;

	msg_t *m_sn = msg_new_string(rxInfo->sourceNum, SLCHAN_NAME, strlen(rxInfo->sourceName),
	                             rxInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[Serial:%s] Error pushing channel name to queue",
		          args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(4);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 8, "Raw Data");

	msg_t *m_cmap = msg_new_string_array(rxInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[Serial:%s] Error pushing channel map to queue",
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

/*!
 * @param[in] lta Pointer to log_thread_args_t
 * @param[in] s Pointer to config_section to be parsed
 * @returns True on success, false on error
 */
bool rx_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[Serial:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	rx_params *rx = calloc(1, sizeof(rx_params));
	if (!rx) {
		log_error(lta->pstate,
		          "[Serial:%s] Unable to allocate memory for device parameters", lta->tag);
		return false;
	}
	(*rx) = rx_getParams();

	config_kv *t = NULL;
	if ((t = config_get_key(s, "port"))) { rx->portName = config_qstrdup(t->value); }
	t = NULL;

	if ((t = config_get_key(s, "name"))) {
		rx->sourceName = config_qstrdup(t->value);
	} else {
		// Must set a name, so nick the tag value
		rx->sourceName = strdup(lta->tag);
	}
	t = NULL;

	if ((t = config_get_key(s, "sourcenum"))) {
		errno = 0;
		int sn = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[Serial:%s] Error parsing source number: %s",
			          lta->tag, strerror(errno));
			free(rx);
			return false;
		}
		if (sn < 0) {
			log_error(lta->pstate, "[Serial:%s] Invalid source number (%s)", lta->tag,
			          t->value);
			free(rx);
			return false;
		}
		if (sn < 10) {
			rx->sourceNum += sn;
		} else {
			rx->sourceNum = sn;
			if (sn < SLSOURCE_EXT || sn > (SLSOURCE_EXT + 0x0F)) {
				log_warning(
					lta->pstate,
					"[Serial:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems",
					lta->tag, sn);
			}
		}
	} else {
		rx->sourceNum = SLSOURCE_EXT + 0x0E;
		log_warning(lta->pstate,
		            "[Serial:%s] Source number not provided - 0x%02x assigned", lta->tag,
		            rx->sourceNum);
	}

	if ((t = config_get_key(s, "baud"))) {
		errno = 0;
		rx->baudRate = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[Serial:%s] Error parsing baud rate: %s", lta->tag,
			          strerror(errno));
			free(rx);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "minbytes"))) {
		errno = 0;
		rx->minBytes = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate,
			          "[Serial:%s] Error parsing minimum message size: %s", lta->tag,
			          strerror(errno));
			free(rx);
			return false;
		}
		if (rx->minBytes <= 0) {
			log_error(
				lta->pstate,
				"[Serial:%s] Invalid minimum packet size specified (%d is not greater than zero)",
				lta->tag, rx->minBytes);
			free(rx);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "maxbytes"))) {
		errno = 0;
		rx->maxBytes = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate,
			          "[Serial:%s] Error parsing maximum message size: %s", lta->tag,
			          strerror(errno));
			free(rx);
			return false;
		}

		if (rx->maxBytes <= 0) {
			log_error(
				lta->pstate,
				"[Serial:%s] Invalid maximum packet size specified (%d is not greater than zero)",
				lta->tag, rx->maxBytes);
			free(rx);
			return false;
		}

		if (rx->maxBytes < rx->minBytes) {
			log_error(
				lta->pstate,
				"[Serial:%s] Invalid maximum packet size specified (%d is smaller than specified minimum packet size)",
				lta->tag, rx->maxBytes);
			free(rx);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "frequency"))) {
		errno = 0;
		rx->pollFreq = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate,
			          "[Serial:%s] Error parsing minimum poll frequency: %s", lta->tag,
			          strerror(errno));
			free(rx);
			return false;
		}

		if (rx->pollFreq <= 0) {
			log_error(
				lta->pstate,
				"[Serial:%s] Invalid minimum poll frequency (%d is not greater than zero)",
				lta->tag, rx->pollFreq);
			free(rx);
			return false;
		}
	}
	t = NULL;
	lta->dParams = rx;
	return true;
}
