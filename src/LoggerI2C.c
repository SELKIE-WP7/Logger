#include "Logger.h"
#include "LoggerI2C.h"
#include "LoggerSignals.h"

void *i2c_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	i2c_params *i2cInfo = (i2c_params *) args->dParams;

	if (!i2c_validate_chanmap(i2cInfo)) {
		log_error(args->pstate, "[I2C:%s] Invalid channel map", i2cInfo->busName);
		args->returnCode = -2;
		return NULL;
	} 

	i2cInfo->handle = i2c_openConnection(i2cInfo->busName);
	if (i2cInfo->handle < 0) {
		log_error(args->pstate, "[I2C:%s] Unable to open a connection", i2cInfo->busName);
		args->returnCode = -1;
		return NULL;
	}
	log_info(args->pstate, 2, "[I2C:%s] Connected", i2cInfo->busName);
	args->returnCode = 0;
	return NULL;
}

void *i2c_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	i2c_params *i2cInfo = (i2c_params *) args->dParams;

	log_info(args->pstate, 1, "[I2C:%s] Logging thread started", i2cInfo->busName);

	struct timespec lastIter = {0};
	clock_gettime(CLOCK_MONOTONIC, &lastIter);
	while (!shutdownFlag) {
		for (int mm = 0; mm < i2cInfo->en_count; mm++) {
			i2c_msg_map *map = &(i2cInfo->chanmap[mm]);
			msg_t *msg = msg_new_float(i2cInfo->sourceNum, map->messageID, map->func(i2cInfo->handle, map->deviceAddr));
			if (!queue_push(args->logQ, msg)) {
				log_error(args->pstate, "[I2C:%s] Error pushing message to queue", i2cInfo->busName);
				msg_destroy(msg);
				args->returnCode = -1;
				pthread_exit(&(args->returnCode));
			}
		}

		lastIter.tv_nsec += 1E9 / i2cInfo->frequency;
		if (lastIter.tv_nsec > 1E9) {
			int nsec = lastIter.tv_nsec / 1000000000;
			lastIter.tv_nsec = lastIter.tv_nsec % 1000000000;
			lastIter.tv_sec += nsec;
		}
		struct timespec now = {0};
		clock_gettime(CLOCK_MONOTONIC, &now);
		struct timespec target = {0};
		if (timespec_subtract(&target, &lastIter, &now)) {
			// Target has passed!
			clock_gettime(CLOCK_MONOTONIC, &lastIter);
		} else {
			clock_gettime(CLOCK_MONOTONIC, &lastIter);
			// If we're interrupted, just carry on. We might need to handle
			// the shutdown flag, and worst case is we log a bit of extra data
			nanosleep(&target, NULL);
		}
	}
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Simple wrapper around i2c_closeConnection(), which will do any cleanup required.
 */
void *i2c_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	i2c_params *i2cInfo = (i2c_params *) args->dParams;
	if (i2cInfo->handle >= 0) { // Admittedly 0 is unlikely
		i2c_closeConnection(i2cInfo->handle);
	}
	i2cInfo->handle = -1;
	return NULL;
}

/*!
 * Populate list of channels and push to queue as a map message
 */
void *i2c_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	i2c_params *i2cInfo = (i2c_params *) args->dParams;

	msg_t *m_sn = msg_new_string(i2cInfo->sourceNum, SLCHAN_NAME, strlen(args->tag), args->tag);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[I2C:%s] Error pushing channel name to queue", i2cInfo->busName);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	uint8_t maxID = 3;
	for (int i=0; i < i2cInfo->en_count; i++) {
		if (i2cInfo->chanmap[i].messageID > maxID) {
			maxID = i2cInfo->chanmap[i].messageID;
		}
	}
	strarray *channels = sa_new(maxID+1);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");

	for (int i=0; i < i2cInfo->en_count; i++) {
		sa_set_entry(channels, i2cInfo->chanmap[i].messageID, &(i2cInfo->chanmap[i].message_name));
	}

	msg_t *m_cmap = msg_new_string_array(i2cInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[I2C:%s] Error pushing channel map to queue", i2cInfo->busName);
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

device_callbacks i2c_getCallbacks() {
	device_callbacks cb = {
		.startup = &i2c_setup,
		.logging = &i2c_logging,
		.shutdown = &i2c_shutdown,
		.channels = &i2c_channels
	};
	return cb;
}

i2c_params i2c_getParams() {
	i2c_params i2c = {
		.busName = NULL,
		.sourceNum = SLSOURCE_I2C,
		.handle = -1,
		.frequency = 10,
		.en_count = 0,
		.chanmap = NULL
	};
	return i2c;
}

/*!
 * Ensures that the only one message is set for each channel, that no reserved
 * channels are used and that device addresses and read functions are set.
 */
bool i2c_validate_chanmap(i2c_params *ip) {
	i2c_msg_map *cmap = ip->chanmap;
	bool seen[128] = {0};
	if (!cmap) {
		return false;
	}
	// Reserved channels
	seen[SLCHAN_NAME] = true;
	seen[SLCHAN_MAP] = true;
	seen[SLCHAN_TSTAMP] = true;
	seen[SLCHAN_RAW] = true;
	seen[SLCHAN_LOG_INFO] = true;
	seen[SLCHAN_LOG_WARN] = true;
	seen[SLCHAN_LOG_ERR] = true;

	for (int i=0; i < ip->en_count; i++) {
		if (seen[ip->chanmap[i].messageID]) {
			return false;
		}
		seen[ip->chanmap[i].messageID] = true;
		if (!ip->chanmap[i].func) {
			return false;
		}
		if (!ip->chanmap[i].deviceAddr) {
			return false;
		}
	}
	return true;
}

/*!
 * Adds three entries to the channel map for a specified INA219 device
 *
 * - `baseID`: Shunt Voltage [mV]
 * - `baseID+1`: Bus Voltage [V]
 * - `baseID+2`: Bus Current [A]
 *
 * @param[in] ip i2c_params structure to modify
 * @param[in] devAddr INA219 Device Address
 * @param[in] baseID Message ID for first message type (Shunt Voltage)
 * @return True on success, false otherwise
 */
bool i2c_chanmap_add_ina219(i2c_params *ip, const uint8_t devAddr, const uint8_t baseID) {
	if (!ip) {
		return false;
	}
	if (baseID < 4 || baseID > 121) {
		return false;
	}
	i2c_msg_map *tmp = realloc(ip->chanmap, sizeof(i2c_msg_map) * (ip->en_count + 3));
	if (!tmp) {
		return false;
	}
	ip->chanmap = tmp;

	char tmpS[50] = {0};

	snprintf(tmpS, 18, "0x%02x:ShuntVoltage", devAddr);
	ip->chanmap[ip->en_count].messageID = baseID;
	// Memory was allocated with malloc, so make sure we initialise these values
	ip->chanmap[ip->en_count].message_name.length = 0; 
	ip->chanmap[ip->en_count].message_name.data = NULL;
	str_update(&(ip->chanmap[ip->en_count].message_name), 18, tmpS);
	ip->chanmap[ip->en_count].deviceAddr = devAddr;
	ip->chanmap[ip->en_count].func = &i2c_ina219_read_shuntVoltage;
	ip->en_count++;

	snprintf(tmpS, 16, "0x%02x:BusVoltage", devAddr);
	ip->chanmap[ip->en_count].messageID = baseID+1;
	ip->chanmap[ip->en_count].message_name.length = 0;
	ip->chanmap[ip->en_count].message_name.data = NULL;
	str_update(&(ip->chanmap[ip->en_count].message_name), 16, tmpS);
	ip->chanmap[ip->en_count].deviceAddr = devAddr;
	ip->chanmap[ip->en_count].func = &i2c_ina219_read_busVoltage;
	ip->en_count++;

	snprintf(tmpS, 16, "0x%02x:BusCurrent", devAddr);
	ip->chanmap[ip->en_count].messageID = baseID+2;
	ip->chanmap[ip->en_count].message_name.length = 0;
	ip->chanmap[ip->en_count].message_name.data = NULL;
	str_update(&(ip->chanmap[ip->en_count].message_name), 16, tmpS);
	ip->chanmap[ip->en_count].deviceAddr = devAddr;
	ip->chanmap[ip->en_count].func = &i2c_ina219_read_current;
	ip->en_count++;

	return true;
}
