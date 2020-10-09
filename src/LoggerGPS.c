#include "Logger.h"
#include "LoggerGPS.h"
#include "LoggerSignals.h"

int gps_setup(program_state *pstate, const char* gpsPortName, const int initialBaud) {
	int gpsHandle = ubx_openConnection(gpsPortName, initialBaud);
	if (gpsHandle < 0) {
		log_error(pstate, "Unable to open a connection on port %s", gpsPortName);
		return -1;
	}

	log_info(pstate, 1, "Configuring GPS...");
	{
		bool msgSuccess = true;
		errno = 0;
		log_info(pstate, 2, "Enabling GPS log messages");
		msgSuccess &= ubx_enableLogMessages(gpsHandle);
		// The usleep commands are probably not required, but mindful
		// that there's a limited RX buffer on the GPS module
		usleep(5E3); 
		msgSuccess &= ubx_enableGalileo(gpsHandle);
		log_info(pstate, 2, "Enabling Galileo (GNSS Reset)");
		sleep(3); // This one *is* necessary, as enabling Galileo can require a GNSS reset
		msgSuccess &= ubx_setNavigationRate(gpsHandle, 500, 1); // 500ms Update rate, new output each time
		usleep(5E3);
		log_info(pstate, 2, "Configuring GPS message rates");
		msgSuccess &= ubx_setMessageRate(gpsHandle, 0x01, 0x07, 1); // NAV-PVT on every update
		usleep(5E3);
		msgSuccess &= ubx_setMessageRate(gpsHandle, 0x01, 0x35, 1); // NAV-SAT on every update
		usleep(5E3);
		msgSuccess &= ubx_setMessageRate(gpsHandle, 0x01, 0x21, 1); // NAV-TIMEUTC on every update
		usleep(5E3);
		log_info(pstate, 2, "Requesting GPS status information");
		msgSuccess &= ubx_pollMessage(gpsHandle, 0x0A, 0x04); // Poll software version
		usleep(5E3);
		msgSuccess &= ubx_pollMessage(gpsHandle, 0x0A, 0x28); // Poll for GNSS status (MON)
		usleep(5E3);
		msgSuccess &= ubx_pollMessage(gpsHandle, 0x06, 0x3E); // As above, but from CFG
		usleep(5E3);

		if (!msgSuccess) {
			log_error(pstate, "Unable to complete GPS configuration: %s", strerror(errno));
			ubx_closeConnection(gpsHandle);
			return EXIT_FAILURE;
		}
		log_info(pstate, 2, "GPS configuration completed.");
	}
	return gpsHandle;
}

void *gps_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	log_info(args->pstate, 1, "GPS Logging thread started");

	uint8_t *buf = calloc(UBX_SERIAL_BUFF, sizeof(uint8_t));
	int ubx_index = 0;
	int ubx_hw = 0;
	while (!shutdownFlag) {
		ubx_message out = {0};
		if (ubx_readMessage_buf(args->streamHandle, &out, buf, &ubx_index, &ubx_hw)) {
			uint8_t *data = NULL;
			ssize_t len = ubx_flat_array(&out, &data);

			msg_t *sm = msg_new_bytes(SLSOURCE_GPS, 3, len, data);
			if (!queue_push(args->logQ, sm)) {
				log_error(args->pstate, "Error pushing message to queue from GPS");
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
				log_error(args->pstate, "Error signalled from readMessage");
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
			usleep(5E2);
		}

		// Clean up before next iteration
		if (out.extdata) { // Similarly, we are finished with this copy
			free(out.extdata);
		}
	}
	free(buf);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}
