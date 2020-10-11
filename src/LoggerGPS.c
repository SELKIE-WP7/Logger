#include "Logger.h"
#include "LoggerGPS.h"
#include "LoggerSignals.h"


/*!
 * Perform initial setup of a u-blox GPS device.
 *
 * It's not unusual for these devices to be configured for a lower baud rate on
 * startup, so connect at a lower rate and switch to something faster.
 *
 * Once the baud rate is configured, set up the messages we need and poll for
 * some status information for the log.
 */ 
void *gps_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	gps_params *gpsInfo = (gps_params *) args->dParams;

	gpsInfo->handle = ubx_openConnection(gpsInfo->portName, gpsInfo->initialBaud);
	if (gpsInfo->handle < 0) {
		log_error(args->pstate, "[GPS:%s] Unable to open a connection", gpsInfo->portName);
		args->returnCode = -1;
		return NULL;
	}

	log_info(args->pstate, 1, "[GPS:%s] Configuring GPS...", gpsInfo->portName);
	{
		bool msgSuccess = true;
		errno = 0;
		log_info(args->pstate, 2, "[GPS:%s] Enabling log messages", gpsInfo->portName);
		msgSuccess &= ubx_enableLogMessages(gpsInfo->handle);
		// The usleep commands are probably not required, but mindful
		// that there's a limited RX buffer on the GPS module
		usleep(5E3); 
		msgSuccess &= ubx_enableGalileo(gpsInfo->handle);
		log_info(args->pstate, 2, "[GPS:%s] Enabling Galileo (GNSS Reset)", gpsInfo->portName);
		sleep(3); // This one *is* necessary, as enabling Galileo can require a GNSS reset
		msgSuccess &= ubx_setNavigationRate(gpsInfo->handle, 500, 1); // 500ms Update rate, new output each time
		usleep(5E3);
		log_info(args->pstate, 2, "[GPS:%s] Configuring message rates", gpsInfo->portName);
		msgSuccess &= ubx_setMessageRate(gpsInfo->handle, 0x01, 0x07, 1); // NAV-PVT on every update
		usleep(5E3);
		msgSuccess &= ubx_setMessageRate(gpsInfo->handle, 0x01, 0x35, 1); // NAV-SAT on every update
		usleep(5E3);
		msgSuccess &= ubx_setMessageRate(gpsInfo->handle, 0x01, 0x21, 1); // NAV-TIMEUTC on every update
		usleep(5E3);
		log_info(args->pstate, 2, "[GPS:%s] Polling for status information", gpsInfo->portName);
		msgSuccess &= ubx_pollMessage(gpsInfo->handle, 0x0A, 0x04); // Poll software version
		usleep(5E3);
		msgSuccess &= ubx_pollMessage(gpsInfo->handle, 0x0A, 0x28); // Poll for GNSS status (MON)
		usleep(5E3);
		msgSuccess &= ubx_pollMessage(gpsInfo->handle, 0x06, 0x3E); // As above, but from CFG
		usleep(5E3);

		if (!msgSuccess) {
			log_error(args->pstate, "[GPS:%s] Unable to configure: %s", gpsInfo->portName, strerror(errno));
			ubx_closeConnection(gpsInfo->handle);
			args->returnCode = -2;
			return NULL;
		}
		log_info(args->pstate, 1, "[GPS:%s] Configuration completed", gpsInfo->portName);
	}
	args->returnCode = 0;
	return NULL;
}

/*!
 * Takes a gps_params struct (passed via log_thread_args_t) 
 * messages from a device configured with gps_setup() and pushes them to the
 * message queue.
 *
 * Exits on error or when shutdown is signalled.
 */
void *gps_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	gps_params *gpsInfo = (gps_params *) args->dParams;

	log_info(args->pstate, 1, "[GPS:%s] Logging thread started", gpsInfo->portName);

	uint8_t *buf = calloc(UBX_SERIAL_BUFF, sizeof(uint8_t));
	int ubx_index = 0;
	int ubx_hw = 0;
	while (!shutdownFlag) {
		ubx_message out = {0};
		if (ubx_readMessage_buf(gpsInfo->handle, &out, buf, &ubx_index, &ubx_hw)) {
			uint8_t *data = NULL;
			ssize_t len = ubx_flat_array(&out, &data);

			msg_t *sm = msg_new_bytes(gpsInfo->sourceNum, 3, len, data);
			if (!queue_push(args->logQ, sm)) {
				log_error(args->pstate, "[GPS:%s] Error pushing message to queue", gpsInfo->portName);
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
			if (!(out.sync1 == 0xFF || out.sync1 == 0xFD || out.sync1 == 0xEE)) {
				// 0xFF, 0xFD and 0xEE are used to signal recoverable states that resulted
				// in no valid message.
				// 
				// 0xFF and 0xFD indicate an out of data error, which is not a problem
				// for serial monitoring, but might indicate EOF when reading from file
				//
				// 0xEE indicates an invalid message following valid sync bytes
				log_error(args->pstate, "[GPS:%s] Error signalled from gps_readMessage_buf", gpsInfo->portName);
				args->returnCode = -2;
				free(buf);
				if (out.extdata) {
					free(out.extdata);
				}
				pthread_exit(&(args->returnCode));
			}
			// We've already exited (via pthread_exit) for error
			// cases, so at this point sleep briefly and wait for
			// more data
			usleep(SERIAL_SLEEP);
		}

		// Clean up before next iteration
		if (out.extdata) { // Similarly, we are finished with this copy
			free(out.extdata);
		}
	}
	free(buf);
	log_info(args->pstate, 1, "[GPS:%s] Logging thread exiting", gpsInfo->portName);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Calls ubx_closeConnection(), which will do any cleanup required.
 */
void *gps_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	gps_params *gpsInfo = (gps_params *) args->dParams;
	if (gpsInfo->handle >= 0) { // Admittedly 0 is unlikely
		ubx_closeConnection(gpsInfo->handle);
	}
	gpsInfo->handle = -1;
	return NULL;
}

device_callbacks gps_getCallbacks() {
	device_callbacks cb = {
		.startup = &gps_setup,
		.logging = &gps_logging,
		.shutdown = &gps_shutdown
	};
	return cb;
}

gps_params gps_getParams() {
	gps_params gp = {
		.portName = NULL,
		.sourceNum = SLSOURCE_GPS,
		.initialBaud = 9600,
		.targetBaud = 115200,
		.handle = -1
	};
	return gp;
}
