/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Logger.h"

#include "LoggerGPS.h"
#include "LoggerSignals.h"

/*!
 * Perform initial setup of a u-blox GPS device.
 *
 * The serial set up is carried out by ubx_openConnection().
 *
 * The module is configured for Galileo support, and to output required
 * navigation information. Satellite information is also requested, but at a
 * lower rate (Once per 100 navigation updates - approximately every 50
 * seconds).
 *
 * GPS module information is also requested at initial startup, but not on a
 * regular basis.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Error code set in ptargs->returnCode if required
 */
void *gps_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	gps_params *gpsInfo = (gps_params *)args->dParams;

	gpsInfo->handle = ubx_openConnection(gpsInfo->portName, gpsInfo->initialBaud);
	if (gpsInfo->handle < 0) {
		log_error(args->pstate, "[GPS:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}

	log_info(args->pstate, 1, "[GPS:%s] Configuring GPS...", args->tag);
	{
		bool msgSuccess = true;
		errno = 0;
		log_info(args->pstate, 2, "[GPS:%s] Enabling log messages", args->tag);
		msgSuccess &= ubx_enableLogMessages(gpsInfo->handle);
		// The usleep commands are probably not required, but mindful
		// that there's a limited RX buffer on the GPS module
		usleep(5E3);
		msgSuccess &= ubx_enableGalileo(gpsInfo->handle);
		log_info(args->pstate, 2, "[GPS:%s] Enabling Galileo (GNSS Reset)", args->tag);

		// This one *is* necessary, as enabling Galileo can require a GNSS reset
		sleep(3);
		// 500ms Update rate, new output each time
		msgSuccess &= ubx_setNavigationRate(gpsInfo->handle, 500, 1);
		usleep(5E3);
		msgSuccess &= ubx_setI2CAddress(gpsInfo->handle, 0x0a);
		usleep(5E3);
		log_info(args->pstate, 2, "[GPS:%s] Configuring message rates", args->tag);

		// NAV-PVT on every update
		msgSuccess &= ubx_setMessageRate(gpsInfo->handle, 0x01, 0x07, 1);
		usleep(5E3);

		// NAV-SAT on every 100th update
		msgSuccess &= ubx_setMessageRate(gpsInfo->handle, 0x01, 0x35, 120);
		usleep(5E3);

		// NAV-TIMEUTC on every update
		msgSuccess &= ubx_setMessageRate(gpsInfo->handle, 0x01, 0x21, 1);
		usleep(5E3);

		log_info(args->pstate, 2, "[GPS:%s] Polling for status information", args->tag);
		msgSuccess &= ubx_pollMessage(gpsInfo->handle, 0x0A, 0x04);
		usleep(5E3);

		// Poll for GNSS status (MON)
		msgSuccess &= ubx_pollMessage(gpsInfo->handle, 0x0A, 0x28);
		usleep(5E3);

		// Poll again for CFG
		msgSuccess &= ubx_pollMessage(gpsInfo->handle, 0x06, 0x3E);
		usleep(5E3);

		if (!msgSuccess) {
			log_error(args->pstate, "[GPS:%s] Unable to configure: %s", args->tag,
			          strerror(errno));
			ubx_closeConnection(gpsInfo->handle);
			args->returnCode = -2;
			return NULL;
		}
		log_info(args->pstate, 1, "[GPS:%s] Configuration completed", args->tag);
	}
	args->returnCode = 0;
	return NULL;
}

/*!
 * Takes a gps_params struct (passed via log_thread_args_t)
 *
 * Reads messages from a device configured with gps_setup() and pushes them to
 * the message queue.
 *
 * Exits on error or when shutdown is signalled.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Error code set in ptargs->returnCode if required
 */
void *gps_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	gps_params *gpsInfo = (gps_params *)args->dParams;

	log_info(args->pstate, 1, "[GPS:%s] Logging thread started", args->tag);

	uint8_t *buf = calloc(UBX_SERIAL_BUFF, sizeof(uint8_t));
	int ubx_index = 0;
	int ubx_hw = 0;
	while (!shutdownFlag) {
		ubx_message out = {0};
		if (ubx_readMessage_buf(gpsInfo->handle, &out, buf, &ubx_index, &ubx_hw)) {
			uint8_t *data = NULL;
			ssize_t len = ubx_flat_array(&out, &data);
			bool handled = false;
			if (out.msgClass == UBXNAV && out.msgID == 0x21) {
				// Extract GPS ToW
				uint32_t ts = out.data[0] + (out.data[1] << 8) +
				              (out.data[2] << 16) + (out.data[3] << 24);
				msg_t *utc =
					msg_new_timestamp(gpsInfo->sourceNum, SLCHAN_TSTAMP, ts);
				if (!queue_push(args->logQ, utc)) {
					log_error(args->pstate,
					          "[GPS:%s] Error pushing message to queue",
					          args->tag);
					msg_destroy(utc);
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
				handled = true;
			} else if (out.msgClass == UBXNAV && out.msgID == 0x07) {
				// NAV-PVT
				ubx_nav_pvt nav = {0};
				if (!ubx_decode_nav_pvt(&out, &nav)) {
					log_error(args->pstate,
					          "[GPS:%s] Unable to decode NAV-PVT message",
					          args->tag);
				} else {
					float posData[6] = {
						nav.longitude,       nav.latitude,
						nav.height * 1E-3,   nav.ASL * 1E-3,
						nav.horizAcc * 1E-3, nav.vertAcc * 1E-3};

					float velData[7] = {
						nav.northV * 1E-3, nav.eastV * 1E-3,
						nav.downV * 1E-3,  nav.groundSpeed * 1E-3,
						nav.heading,       nav.speedAcc * 1E-3,
						nav.headingAcc};

					float dt[8] = {nav.year,       nav.month,   nav.day,
					               nav.hour,       nav.minute,  nav.second,
					               nav.nanosecond, nav.accuracy};

					// clang-format off
					msg_t *mnav = msg_new_float_array(gpsInfo->sourceNum, 4, 6, posData);
					msg_t *mvel = msg_new_float_array(gpsInfo->sourceNum, 5, 7, velData);
					msg_t *mdt = msg_new_float_array(gpsInfo->sourceNum, 6, 8, dt);
					// clang-format on

					if (!queue_push(args->logQ, mnav) ||
					    !queue_push(args->logQ, mvel) ||
					    !queue_push(args->logQ, mdt)) {
						log_error(
							args->pstate,
							"[GPS:%s] Error pushing messages to queue",
							args->tag);
						msg_destroy(mnav);
						msg_destroy(mvel);
						msg_destroy(mdt);
						args->returnCode = -1;
						pthread_exit(&(args->returnCode));
					}
					handled = true;
				}
			}
			if (!handled || gpsInfo->dumpAll) {
				msg_t *sm = msg_new_bytes(gpsInfo->sourceNum, 3, len, data);
				if (!queue_push(args->logQ, sm)) {
					log_error(args->pstate,
					          "[GPS:%s] Error pushing message to queue",
					          args->tag);
					msg_destroy(sm);
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
			}
			if (data) {
				// Copied into message, so can safely free here
				free(data);
			}
			// Do not destroy or free sm (or other msg_t objects) here
			// After pushing it to the queue, it is the responsibility of the
			// consumer to dispose of it after use.
		} else {
			if (!(out.sync1 == 0xFF || out.sync1 == 0xFD || out.sync1 == 0xEE)) {
				// 0xFF, 0xFD and 0xEE are used to signal recoverable
				// states that resulted in no valid message.
				//
				// 0xFF and 0xFD indicate an out of data error, which is
				// not a problem for serial monitoring, but might indicate
				// EOF when reading from file
				//
				// 0xEE indicates an invalid message following valid sync bytes
				log_error(args->pstate,
				          "[GPS:%s] Error signalled from gps_readMessage_buf",
				          args->tag);
				args->returnCode = -2;
				free(buf);
				if (out.extdata) { free(out.extdata); }
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
	log_info(args->pstate, 1, "[GPS:%s] Logging thread exiting", args->tag);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Calls ubx_closeConnection(), which will do any cleanup required.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Error code set in ptargs->returnCode if required
 */
void *gps_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	gps_params *gpsInfo = (gps_params *)args->dParams;

	if (gpsInfo->handle >= 0) { // Admittedly 0 is unlikely
		ubx_closeConnection(gpsInfo->handle);
	}
	gpsInfo->handle = -1;
	if (gpsInfo->sourceName) {
		free(gpsInfo->sourceName);
		gpsInfo->sourceName = NULL;
	}
	if (gpsInfo->portName) {
		free(gpsInfo->portName);
		gpsInfo->portName = NULL;
	}
	return NULL;
}

/*!
 * Populate list of channels and push to queue as a map message
 *
 * Terminates thread on error
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Error code set in ptargs->returnCode if required
 */
void *gps_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	gps_params *gpsInfo = (gps_params *)args->dParams;

	msg_t *m_sn = msg_new_string(gpsInfo->sourceNum, SLCHAN_NAME, strlen(gpsInfo->sourceName),
	                             gpsInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[GPS:%s] Error pushing channel name to queue", args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(7);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 7, "Raw UBX");
	sa_create_entry(channels, 4, 8, "Position");
	sa_create_entry(channels, 5, 8, "Velocity");
	sa_create_entry(channels, 6, 8, "DateTime");

	msg_t *m_cmap = msg_new_string_array(gpsInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[GPS:%s] Error pushing channel map to queue", args->tag);
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
 * @returns device_callbacks for uBlox GPS devices
 */
device_callbacks gps_getCallbacks() {
	device_callbacks cb = {.startup = &gps_setup,
	                       .logging = &gps_logging,
	                       .shutdown = &gps_shutdown,
	                       .channels = &gps_channels};
	return cb;
}

/*!
 * @returns Default parameters for uBlox GPS devices
 */
gps_params gps_getParams() {
	gps_params gp = {.portName = NULL,
	                 .sourceNum = SLSOURCE_GPS,
	                 .initialBaud = 9600,
	                 .targetBaud = 115200,
	                 .handle = -1,
	                 .dumpAll = false};
	return gp;
}

/*!
 * @param[in] lta Pointer to log_thread_args_t
 * @param[in] s Pointer to config_section to be parsed
 * @returns True on success, false on error
 */
bool gps_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[GPS:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	gps_params *gp = calloc(1, sizeof(gps_params));
	if (!gp) {
		log_error(lta->pstate, "[GPS:%s] Unable to allocate memory for device parameters",
		          lta->tag);
		return false;
	}
	(*gp) = gps_getParams();

	config_kv *t = NULL;
	if ((t = config_get_key(s, "name"))) {
		gp->sourceName = config_qstrdup(t->value);
	} else {
		// Must set a name, so nick the tag value
		gp->sourceName = strdup(lta->tag);
	}
	t = NULL;

	if ((t = config_get_key(s, "port"))) { gp->portName = config_qstrdup(t->value); }
	t = NULL;

	if ((t = config_get_key(s, "dumpall"))) { gp->dumpAll = config_parse_bool(t->value); }
	t = NULL;

	if ((t = config_get_key(s, "baud"))) {
		errno = 0;
		gp->targetBaud = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[GPS:%s] Error parsing baud rate: %s", lta->tag,
			          strerror(errno));
			free(gp);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "initialbaud"))) {
		errno = 0;
		gp->initialBaud = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[GPS:%s] Error parsing initial baud rate: %s",
			          lta->tag, strerror(errno));
			free(gp);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "sourcenum"))) {
		errno = 0;
		int sn = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[GPS:%s] Error parsing source number: %s",
			          lta->tag, strerror(errno));
			free(gp);
			return false;
		}
		if (sn < 0) {
			log_error(lta->pstate, "[GPS:%s] Invalid source number (%s)", lta->tag,
			          t->value);
			free(gp);
			return false;
		}
		if (sn < 10) {
			gp->sourceNum += sn;
		} else {
			gp->sourceNum = sn;
			if (sn < SLSOURCE_GPS || sn > (SLSOURCE_GPS + 0x0F)) {
				log_warning(
					lta->pstate,
					"[GPS:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems",
					lta->tag, sn);
			}
		}
	}
	t = NULL;
	lta->dParams = gp;
	return true;
}
