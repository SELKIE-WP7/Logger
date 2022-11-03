#include "Logger.h"

#include "LoggerLPMS.h"
#include "LoggerSignals.h"

#define CHAN_ACC_RAW_X 4
#define CHAN_ACC_RAW_Y 5
#define CHAN_ACC_RAW_Z 6
#define CHAN_ACC_CAL_X 7
#define CHAN_ACC_CAL_Y 8
#define CHAN_ACC_CAL_Z 9
#define CHAN_G_RAW_X   10
#define CHAN_G_RAW_Y   11
#define CHAN_G_RAW_Z   12
#define CHAN_G_CAL_X   13
#define CHAN_G_CAL_Y   14
#define CHAN_G_CAL_Z   15
#define CHAN_G_ALIGN_X 16
#define CHAN_G_ALIGN_Y 17
#define CHAN_G_ALIGN_Z 18
#define CHAN_OMEGA_X   19
#define CHAN_OMEGA_Y   20
#define CHAN_OMEGA_Z   21
#define CHAN_ROLL      22
#define CHAN_PITCH     23
#define CHAN_YAW       24
#define CHAN_ACC_LIN_X 25
#define CHAN_ACC_LIN_Y 26
#define CHAN_ACC_LIN_Z 27
#define CHAN_ALTITUDE  28

/*!
 * Opens a serial connection and the required baud rate.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Exit code in ptargs->returnCode if required
 */
void *lpms_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	lpms_params *lpmsInfo = (lpms_params *)args->dParams;

	lpmsInfo->handle = lpms_openConnection(lpmsInfo->portName, lpmsInfo->baudRate);
	if (lpmsInfo->handle < 0) {
		log_error(args->pstate, "[LPMS:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}

	args->returnCode = 0;

	lpms_send_command_mode(lpmsInfo->handle);
	usleep(250000);
	lpms_message getModel = {.id = lpmsInfo->unitID, .command = LPMS_MSG_GET_SENSORMODEL};
	lpms_send_command(lpmsInfo->handle, &getModel);
	usleep(10000);

	lpms_message getFW = {.id = lpmsInfo->unitID, .command = LPMS_MSG_GET_FIRMWAREVER};
	lpms_send_command(lpmsInfo->handle, &getFW);
	usleep(10000);

	lpms_message getSerial = {.id = lpmsInfo->unitID, .command = LPMS_MSG_GET_SERIALNUM};
	lpms_send_command(lpmsInfo->handle, &getSerial);
	usleep(10000);

	uint8_t rateData[4] = {
		(lpmsInfo->pollFreq & 0xFF),
		(lpmsInfo->pollFreq & 0xFF00) >> 8,
		(lpmsInfo->pollFreq & 0xFF0000) >> 16,
		(lpmsInfo->pollFreq & 0xFF000000) >> 24,
	};
	lpms_message setRate = {.id = lpmsInfo->unitID,
	                        .command = LPMS_MSG_SET_FREQ,
	                        .length = sizeof(uint32_t),
	                        .data = rateData};
	lpms_send_command(lpmsInfo->handle, &setRate);
	usleep(125000);
	lpms_message getRate = {.id = lpmsInfo->unitID, .command = LPMS_MSG_GET_FREQ};
	lpms_send_command(lpmsInfo->handle, &getRate);
	usleep(10000);

	const uint32_t outputs =
		((1U << LPMS_IMU_ACCEL_RAW) + (1U << LPMS_IMU_ACCEL_CAL) +
	         (1U << LPMS_IMU_GYRO_RAW) + (1U << LPMS_IMU_GYRO_CAL) +
	         (1U << LPMS_IMU_GYRO_ALIGN) + (1U << LPMS_IMU_OMEGA) + (1U << LPMS_IMU_EULER) +
	         (1U << LPMS_IMU_ACCEL_LINEAR) + (1U << LPMS_IMU_ALTITUDE));
	uint8_t outputData[4] = {(outputs & 0xFF), (outputs & 0xFF00) >> 8,
	                         (outputs & 0xFF0000) >> 16, (outputs & 0xFF000000) >> 24};

	lpms_message setTransmitted = {.id = lpmsInfo->unitID,
	                               .command = LPMS_MSG_SET_OUTPUTS,
	                               .length = sizeof(uint32_t),
	                               .data = outputData};
	lpms_send_command(lpmsInfo->handle, &setTransmitted);
	usleep(125000);

	lpms_message getTransmitted = {.id = lpmsInfo->unitID, .command = LPMS_MSG_GET_OUTPUTS};
	lpms_send_command(lpmsInfo->handle, &getTransmitted);
	usleep(10000);
	log_info(args->pstate, 1, "[LPMS:%s] Initial setup commands sent", args->tag);
	return NULL;
}

/*!
 * Create a floating point value message for the specified source and channel
 * numbers, and push it to the queue (Q). If unable to create or push the
 * message, tidy up and return false.
 *
 * Reduces code duplication in lpms_logging()
 *
 * @param[in] Q Pointer to message queue
 * @param[in] src Message source number
 * @param[in] chan Message type/channel number
 * @param[in] val Message value
 * @returns True on success, False on error
 */
inline bool lpms_queue_message(msgqueue *Q, const uint8_t src, const uint8_t chan,
                               const float val) {
	msg_t *m = msg_new_float(src, chan, val);
	if (m == NULL) { return false; }
	if (!queue_push(Q, m)) {
		msg_destroy(m);
		return false;
	}
	return true;
}

/*!
 * Reads messages from the connection established by lpms_setup(), and pushes them to the
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
void *lpms_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	lpms_params *lpmsInfo = (lpms_params *)args->dParams;

	log_info(args->pstate, 1, "[LPMS:%s] Logging thread started", args->tag);
	lpms_send_stream_mode(lpmsInfo->handle);

	uint8_t *buf = calloc(LPMS_BUFF, sizeof(uint8_t));
	size_t lpms_hw = 0;
	size_t lpms_end = 0;
	uint32_t outputs = 0;
	bool unitMismatch = false;
	unsigned int pendingCount = 0;
	unsigned int missingCount = 0;
	while (!shutdownFlag) {
		lpms_data d = {.present = outputs};
		lpms_message *m = calloc(1, sizeof(lpms_message));
		if (!m) {
			perror("lpms_message calloc");
			args->returnCode = -1;
			pthread_exit(&(args->returnCode));
			return NULL;
		}
		bool r = lpms_readMessage_buf(lpmsInfo->handle, m, buf, &lpms_end, &lpms_hw);
		if (r) {
			uint16_t cs = 0;
			if (!(lpms_checksum(m, &cs) && cs == m->checksum)) {
				free(m->data);
				free(m);
				continue;
			}
			if ((m->id != lpmsInfo->unitID) && !unitMismatch) {
				log_warning(
					args->pstate,
					"[LPMS:%s] Unexpected unit ID (got 0x%02x, expected 0x%02x)",
					args->tag, m->id, lpmsInfo->unitID);
				unitMismatch = true;
			}
			if (m->command == LPMS_MSG_GET_OUTPUTS) {
				d.present = (uint32_t)m->data[0] + ((uint32_t)m->data[1] << 8) +
				            ((uint32_t)m->data[2] << 16) +
				            ((uint32_t)m->data[3] << 24);
				outputs = d.present;
				log_info(args->pstate, 1,
				         "[LPMS:%s] Output configuration received for unit 0x%02x",
				         args->tag, m->id);
			} else if (m->command == LPMS_MSG_GET_SENSORMODEL) {
				log_info(args->pstate, 1,
				         "[LPMS:%s] Unit 0x%02x: Sensor model: %-24s", args->tag,
				         m->id, (char *)m->data);
				char *lm = NULL;
				int sl = asprintf(&lm, "LPMS Unit 0x%02x: Sensor model: %-24s",
				                  m->id, (char *)m->data);
				msg_t *sm = msg_new_string(lpmsInfo->sourceNum, SLCHAN_LOG_INFO,
				                           sl, lm);
				free(lm);
				lm = NULL;
				if (!queue_push(args->logQ, sm)) {
					log_error(args->pstate,
					          "[LPMS:%s] Error pushing message to queue",
					          args->tag);
					msg_destroy(sm);
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
			} else if (m->command == LPMS_MSG_GET_SERIALNUM) {
				log_info(args->pstate, 1,
				         "[LPMS:%s] Unit 0x%02x: Serial number: %-24s", args->tag,
				         m->id, (char *)m->data);
				char *lm = NULL;
				int sl = asprintf(&lm, "LPMS Unit 0x%02x: Serial number: %-24s",
				                  m->id, (char *)m->data);
				msg_t *sm = msg_new_string(lpmsInfo->sourceNum, SLCHAN_LOG_INFO,
				                           sl, lm);
				free(lm);
				lm = NULL;
				if (!queue_push(args->logQ, sm)) {
					log_error(args->pstate,
					          "[LPMS:%s] Error pushing message to queue",
					          args->tag);
					msg_destroy(sm);
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
			} else if (m->command == LPMS_MSG_GET_FIRMWAREVER) {
				log_info(args->pstate, 1,
				         "[LPMS:%s] Unit 0x%02x: Firmware version: %-24s",
				         args->tag, m->id, (char *)m->data);
				char *lm = NULL;
				int sl = asprintf(&lm, "LPMS Unit 0x%02x: Firmware version: %-24s",
				                  m->id, (char *)m->data);
				msg_t *sm = msg_new_string(lpmsInfo->sourceNum, SLCHAN_LOG_INFO,
				                           sl, lm);
				free(lm);
				lm = NULL;
				if (!queue_push(args->logQ, sm)) {
					log_error(args->pstate,
					          "[LPMS:%s] Error pushing message to queue",
					          args->tag);
					msg_destroy(sm);
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
			} else if (m->command == LPMS_MSG_GET_FREQ) {
				uint32_t rate = m->data[0] +
					((uint32_t) m->data[1] << 8) +
					((uint32_t) m->data[2] << 16) +
					((uint32_t) m->data[3] << 24);
				log_info(args->pstate, 1,
					"[LPMS:%s] Unit 0x%02x: Message rate %dHz", args->tag, m->id, rate);
			} else if (m->command == LPMS_MSG_GET_IMUDATA) {
				if (!d.present) {
					// Can't extract data until configuration has appeared
					if ((pendingCount++ % 100) == 0) {
						log_warning(
							args->pstate,
							"[LPMS:%s] No output configuration received - skipping messages (%d skipped so far)",
							args->tag, pendingCount);
						log_info(args->pstate, 3,
						         "[LPMS:%s] Repeating GET_OUTPUTS request",
						         args->tag);
						lpms_message getTransmitted = {
							.id = lpmsInfo->unitID,
							.command = LPMS_MSG_GET_OUTPUTS};
						lpms_send_command(lpmsInfo->handle,
						                  &getTransmitted);
					}
					free(m->data);
					free(m);
					continue;
				}
				if (lpms_imu_set_timestamp(m, &d)) {
					msg_t *ts = msg_new_timestamp(lpmsInfo->sourceNum,
					                              SLCHAN_TSTAMP, d.timestamp);
					if (!queue_push(args->logQ, ts)) {
						log_error(
							args->pstate,
							"[LPMS:%s] Error pushing message to queue",
							args->tag);
						msg_destroy(ts);
						free(m->data);
						free(m);
						args->returnCode = -1;
						pthread_exit(&(args->returnCode));
					}
				} else {
					log_warning(args->pstate,
					            "[LPMS:%s] Unit 0x%02x: Timestamp invalid",
					            args->tag, m->id);
				}
				bool dataSet = true;
				dataSet &= lpms_imu_set_accel_raw(m, &d);
				dataSet &= lpms_imu_set_accel_cal(m, &d);
				dataSet &= lpms_imu_set_gyro_raw(m, &d);
				dataSet &= lpms_imu_set_gyro_cal(m, &d);
				dataSet &= lpms_imu_set_gyro_aligned(m, &d);
				dataSet &= lpms_imu_set_omega(m, &d);
				dataSet &= lpms_imu_set_euler_angles(m, &d);
				dataSet &= lpms_imu_set_accel_linear(m, &d);
				dataSet &= lpms_imu_set_altitude(m, &d);
				/*
				Not currently included in outputs
				dataSet |= lpms_imu_set_mag_raw(m, &d);
				dataSet |= lpms_imu_set_mag_cal(m, &d);
				dataSet |= lpms_imu_set_pressure(m, &d);
				dataSet |= lpms_imu_set_temperature(m, &d);
				*/
				if (!dataSet) {
					if (missingCount == 0) {
						log_warning(
							args->pstate,
							"[LPMS:%s] Unit 0x%02x: Some data missing in update",
							args->tag, m->id);
					}
					missingCount++;
				} else {
					missingCount = 0;
				}
				// Create output messages and push to queue
				bool rs = true;
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ACC_RAW_X, d.accel_raw[0]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ACC_RAW_Y, d.accel_raw[1]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ACC_RAW_Z, d.accel_raw[2]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ACC_CAL_X, d.accel_cal[0]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ACC_CAL_Y, d.accel_cal[1]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ACC_CAL_Z, d.accel_cal[2]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_G_RAW_X, d.gyro_raw[0]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_G_RAW_Y, d.gyro_raw[1]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_G_RAW_Z, d.gyro_raw[2]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_G_CAL_X, d.gyro_cal[0]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_G_CAL_Y, d.gyro_cal[1]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_G_CAL_Z, d.gyro_cal[2]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_G_ALIGN_X, d.gyro_aligned[0]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_G_ALIGN_Y, d.gyro_aligned[1]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_G_ALIGN_Z, d.gyro_aligned[2]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_OMEGA_X, d.omega[0]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_OMEGA_Y, d.omega[1]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_OMEGA_Z, d.omega[2]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ROLL, d.euler_angles[0]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_PITCH, d.euler_angles[1]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum, CHAN_YAW,
				                         d.euler_angles[2]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ACC_LIN_X, d.accel_linear[0]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ACC_LIN_Y, d.accel_linear[1]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ACC_LIN_Z, d.accel_linear[2]);
				rs &= lpms_queue_message(args->logQ, lpmsInfo->sourceNum,
				                         CHAN_ALTITUDE, d.altitude);
				if (!rs) {
					log_error(
						args->pstate,
						"[LPMS:%s] Unable to allocate and/or queue all messages (%d)",
						args->tag, errno);
					free(m->data);
					free(m);
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
			} else {
				log_info(args->pstate, 2, "[LPSM:%s] Unhandled message type: 0x%02x [%02d bytes]", args->tag, m->command, m->length);
			}
			free(m->data);
			free(m);
		} else {
			// No message available, so sleep 
			usleep(1E5);
		}
	}
	free(buf);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Simple wrapper around lpms_closeConnection(), which will do any cleanup required.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Exit code in ptargs->returnCode if required
 */
void *lpms_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	lpms_params *lpmsInfo = (lpms_params *)args->dParams;

	if (lpmsInfo->handle >= 0) { // Admittedly 0 is unlikely
		close(lpmsInfo->handle);
	}
	lpmsInfo->handle = -1;
	if (lpmsInfo->portName) {
		free(lpmsInfo->portName);
		lpmsInfo->portName = NULL;
	}
	return NULL;
}

/*!
 * @returns device_callbacks for serial sources
 */
device_callbacks lpms_getCallbacks() {
	device_callbacks cb = {.startup = &lpms_setup,
	                       .logging = &lpms_logging,
	                       .shutdown = &lpms_shutdown,
	                       .channels = &lpms_channels};
	return cb;
}

/*!
 * @returns Default parameters for serial sources
 */
lpms_params lpms_getParams() {
	lpms_params mp = {
		.portName = NULL, .baudRate = 921600, .handle = -1, .unitID = 1, .pollFreq = 10};
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
void *lpms_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	lpms_params *lpmsInfo = (lpms_params *)args->dParams;

	msg_t *m_sn = msg_new_string(lpmsInfo->sourceNum, SLCHAN_NAME,
	                             strlen(lpmsInfo->sourceName), lpmsInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[LPMS:%s] Error pushing channel name to queue",
		          args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(29);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 8, "Raw Data");
	sa_create_entry(channels, CHAN_ACC_RAW_X, 17, "AccelerationRaw_X");
	sa_create_entry(channels, CHAN_ACC_RAW_Y, 17, "AccelerationRaw_Y");
	sa_create_entry(channels, CHAN_ACC_RAW_Z, 17, "AccelerationRaw_Z");
	sa_create_entry(channels, CHAN_ACC_CAL_X, 17, "AccelerationCal_X");
	sa_create_entry(channels, CHAN_ACC_CAL_Y, 17, "AccelerationCal_Y");
	sa_create_entry(channels, CHAN_ACC_CAL_Z, 17, "AccelerationCal_Z");
	sa_create_entry(channels, CHAN_G_RAW_X, 9, "GyroRaw_X");
	sa_create_entry(channels, CHAN_G_RAW_Y, 9, "GyroRaw_Y");
	sa_create_entry(channels, CHAN_G_RAW_Z, 9, "GyroRaw_Z");
	sa_create_entry(channels, CHAN_G_CAL_X, 9, "GyroCal_X");
	sa_create_entry(channels, CHAN_G_CAL_Y, 9, "GyroCal_Y");
	sa_create_entry(channels, CHAN_G_CAL_Z, 9, "GyroCal_Z");
	sa_create_entry(channels, CHAN_G_ALIGN_X, 11, "GyroAlign_X");
	sa_create_entry(channels, CHAN_G_ALIGN_Y, 11, "GyroAlign_Y");
	sa_create_entry(channels, CHAN_G_ALIGN_Z, 11, "GyroAlign_Z");
	sa_create_entry(channels, CHAN_OMEGA_X, 12, "AngularVel X");
	sa_create_entry(channels, CHAN_OMEGA_Y, 12, "AngularVel Y");
	sa_create_entry(channels, CHAN_OMEGA_Z, 12, "AngularVel Z");
	sa_create_entry(channels, CHAN_ROLL, 4, "Roll");
	sa_create_entry(channels, CHAN_PITCH, 5, "Pitch");
	sa_create_entry(channels, CHAN_YAW, 3, "Yaw");
	sa_create_entry(channels, CHAN_ACC_LIN_X, 17, "AccelerationLin_X");
	sa_create_entry(channels, CHAN_ACC_LIN_Y, 17, "AccelerationLin_Y");
	sa_create_entry(channels, CHAN_ACC_LIN_Z, 17, "AccelerationLin_Z");
	sa_create_entry(channels, CHAN_ALTITUDE, 8, "Altitude");
	msg_t *m_cmap = msg_new_string_array(lpmsInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[LPMS:%s] Error pushing channel map to queue", args->tag);
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
bool lpms_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[LPMS:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	lpms_params *lpmsInfo = calloc(1, sizeof(lpms_params));
	if (!lpmsInfo) {
		log_error(lta->pstate, "[LPMS:%s] Unable to allocate memory for device parameters",
		          lta->tag);
		return false;
	}
	(*lpmsInfo) = lpms_getParams();

	config_kv *t = NULL;
	if ((t = config_get_key(s, "port"))) { lpmsInfo->portName = config_qstrdup(t->value); }
	t = NULL;

	if ((t = config_get_key(s, "name"))) {
		lpmsInfo->sourceName = config_qstrdup(t->value);
	} else {
		// Must set a name, so nick the tag value
		lpmsInfo->sourceName = strdup(lta->tag);
	}
	t = NULL;

	if ((t = config_get_key(s, "sourcenum"))) {
		errno = 0;
		int sn = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[LPMS:%s] Error parsing source number: %s",
			          lta->tag, strerror(errno));
			return false;
		}
		if (sn < 0) {
			log_error(lta->pstate, "[LPMS:%s] Invalid source number (%s)", lta->tag,
			          t->value);
			return false;
		}
		if (sn < 10) {
			lpmsInfo->sourceNum += sn;
		} else {
			lpmsInfo->sourceNum = sn;
			if (sn < SLSOURCE_ADC || sn > (SLSOURCE_ADC + 0x0F)) {
				log_warning(
					lta->pstate,
					"[LPMS:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems",
					lta->tag, sn);
			}
		}
	} else {
		lpmsInfo->sourceNum = SLSOURCE_ADC + 0x0E;
		log_warning(lta->pstate, "[LPMS:%s] Source number not provided - 0x%02x assigned",
		            lta->tag, lpmsInfo->sourceNum);
	}

	if ((t = config_get_key(s, "baud"))) {
		errno = 0;
		lpmsInfo->baudRate = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[LPMS:%s] Error parsing baud rate: %s", lta->tag,
			          strerror(errno));
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "unit"))) {
		errno = 0;
		lpmsInfo->unitID = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[LPMS:%s] Error parsing unit ID: %s", lta->tag,
			          strerror(errno));
			return false;
		}
		if ((lpmsInfo->unitID <= 0) || (lpmsInfo->unitID > 255)) {
			log_error(lta->pstate,
			          "[LPMS:%s] Unit ID specified (%d not in range 1-255)", lta->tag,
			          lpmsInfo->unitID);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "frequency"))) {
		errno = 0;
		lpmsInfo->pollFreq = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate,
			          "[LPMS:%s] Error parsing minimum poll frequency: %s", lta->tag,
			          strerror(errno));
			return false;
		}

		switch (lpmsInfo->pollFreq) {
			case 5:
			case 10:
			case 50:
			case 100:
			case 250:
			case 500:
				break;
			default:
				log_error(
					lta->pstate,
					"[LPMS:%s] Invalid minimum poll frequency (%d is not greater than zero)",
					lta->tag, lpmsInfo->pollFreq);
				return false;
		}
	}
	t = NULL;
	lta->dParams = lpmsInfo;
	return true;
}
