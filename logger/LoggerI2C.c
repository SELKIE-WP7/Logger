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

#include "LoggerI2C.h"
#include "LoggerSignals.h"

/*!
 * Connects to specified I2C bus after validating the configured channel map.
 *
 * Device specific parameters need to be specified in an i2c_params structure
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Error code set in ptargs->returnCode if required
 */
void *i2c_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	i2c_params *i2cInfo = (i2c_params *)args->dParams;

	if (!i2c_validate_chanmap(i2cInfo)) {
		log_error(args->pstate, "[I2C:%s] Invalid channel map", args->tag);
		args->returnCode = -2;
		return NULL;
	}

	i2cInfo->handle = i2c_openConnection(i2cInfo->busName);
	if (i2cInfo->handle < 0) {
		log_error(args->pstate, "[I2C:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}
	log_info(args->pstate, 2, "[I2C:%s] Connected", args->tag);
	args->returnCode = 0;
	return NULL;
}

/*!
 * Iterates over all registered channels, calling the appropriate read function against
 * the registered I2C device ID.
 *
 * After each conversion function has been called, this thread will sleep in an
 * attempt to keep the poll frequency at the rate requested. This is a best
 * effort attempt.
 *
 * Thread will exit on error
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Error code set in ptargs->returnCode if required
 */
void *i2c_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	i2c_params *i2cInfo = (i2c_params *)args->dParams;

	log_info(args->pstate, 1, "[I2C:%s] Logging thread started", args->tag);

	struct timespec lastIter = {0};
	clock_gettime(CLOCK_MONOTONIC, &lastIter);
	while (!shutdownFlag) {
		for (int mm = 0; mm < i2cInfo->en_count; mm++) {
			i2c_msg_map *map = &(i2cInfo->chanmap[mm]);
			float value = map->func(i2cInfo->handle, map->deviceAddr, map->ext);

			msg_t *msg = msg_new_float(i2cInfo->sourceNum, map->messageID, value);
			if (!queue_push(args->logQ, msg)) {
				log_error(args->pstate, "[I2C:%s] Error pushing message to queue",
				          args->tag);
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
		msg_t *msg = msg_new_timestamp(i2cInfo->sourceNum, SLCHAN_TSTAMP,
		                               (1000 * now.tv_sec + now.tv_nsec / 1000000));
		if (!queue_push(args->logQ, msg)) {
			log_error(args->pstate, "[I2C:%s] Error pushing message to queue",
			          args->tag);
			msg_destroy(msg);
			args->returnCode = -1;
			pthread_exit(&(args->returnCode));
		}
		struct timespec target = {0};
		if (timespec_subtract(&target, &lastIter, &now)) {
			// Target has passed!
			log_warning(args->pstate, "[I2C:%s] Deadline missed", args->tag);
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
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL
 */
void *i2c_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	i2c_params *i2cInfo = (i2c_params *)args->dParams;

	if (i2cInfo->handle >= 0) { // Admittedly 0 is unlikely
		i2c_closeConnection(i2cInfo->handle);
	}
	i2cInfo->handle = -1;

	if (i2cInfo->sourceName) {
		free(i2cInfo->sourceName);
		i2cInfo->sourceName = NULL;
	}
	if (i2cInfo->busName) {
		free(i2cInfo->busName);
		i2cInfo->busName = NULL;
	}
	if (i2cInfo->chanmap) {
		free(i2cInfo->chanmap);
		i2cInfo->chanmap = NULL;
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
void *i2c_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	i2c_params *i2cInfo = (i2c_params *)args->dParams;

	msg_t *m_sn = msg_new_string(i2cInfo->sourceNum, SLCHAN_NAME, strlen(i2cInfo->sourceName),
	                             i2cInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[I2C:%s] Error pushing channel name to queue", args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	uint8_t maxID = 3;
	for (int i = 0; i < i2cInfo->en_count; i++) {
		if (i2cInfo->chanmap[i].messageID > maxID) {
			maxID = i2cInfo->chanmap[i].messageID;
		}
	}
	strarray *channels = sa_new(maxID + 1);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");

	for (int i = 0; i < i2cInfo->en_count; i++) {
		sa_set_entry(channels, i2cInfo->chanmap[i].messageID,
		             &(i2cInfo->chanmap[i].message_name));
	}

	msg_t *m_cmap = msg_new_string_array(i2cInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[I2C:%s] Error pushing channel map to queue", args->tag);
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
 * @returns device_callbacks for I2C devices
 */
device_callbacks i2c_getCallbacks() {
	device_callbacks cb = {.startup = &i2c_setup,
	                       .logging = &i2c_logging,
	                       .shutdown = &i2c_shutdown,
	                       .channels = &i2c_channels};
	return cb;
}

/*!
 * @returns Default I2C parameters
 */
i2c_params i2c_getParams() {
	i2c_params i2c = {.busName = NULL,
	                  .sourceName = NULL,
	                  .sourceNum = SLSOURCE_I2C,
	                  .handle = -1,
	                  .frequency = 10,
	                  .en_count = 0,
	                  .chanmap = NULL};
	return i2c;
}

/*!
 * Ensures that the only one message is set for each channel, that no reserved
 * channels are used and that device addresses and read functions are set.
 *
 * @param[in] ip Pointer to i2c_params structure
 * @returns True if all parameters are valid, false otherwise
 */
bool i2c_validate_chanmap(i2c_params *ip) {
	i2c_msg_map *cmap = ip->chanmap;
	bool seen[128] = {0};
	if (!cmap) { return false; }
	// Reserved channels
	seen[SLCHAN_NAME] = true;
	seen[SLCHAN_MAP] = true;
	seen[SLCHAN_TSTAMP] = true;
	seen[SLCHAN_RAW] = true;
	seen[SLCHAN_LOG_INFO] = true;
	seen[SLCHAN_LOG_WARN] = true;
	seen[SLCHAN_LOG_ERR] = true;

	for (int i = 0; i < ip->en_count; i++) {
		if (seen[ip->chanmap[i].messageID]) { return false; }
		seen[ip->chanmap[i].messageID] = true;
		if (!ip->chanmap[i].func) { return false; }
		if (!ip->chanmap[i].deviceAddr) { return false; }
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
	if (!ip) { return false; }
	if (baseID < 4 || baseID > 121) { return false; }
	i2c_msg_map *tmp = realloc(ip->chanmap, sizeof(i2c_msg_map) * (ip->en_count + 3));
	if (!tmp) { return false; }
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
	ip->chanmap[ip->en_count].messageID = baseID + 1;
	ip->chanmap[ip->en_count].message_name.length = 0;
	ip->chanmap[ip->en_count].message_name.data = NULL;
	str_update(&(ip->chanmap[ip->en_count].message_name), 16, tmpS);
	ip->chanmap[ip->en_count].deviceAddr = devAddr;
	ip->chanmap[ip->en_count].func = &i2c_ina219_read_busVoltage;
	ip->en_count++;

	snprintf(tmpS, 16, "0x%02x:BusCurrent", devAddr);
	ip->chanmap[ip->en_count].messageID = baseID + 2;
	ip->chanmap[ip->en_count].message_name.length = 0;
	ip->chanmap[ip->en_count].message_name.data = NULL;
	str_update(&(ip->chanmap[ip->en_count].message_name), 16, tmpS);
	ip->chanmap[ip->en_count].deviceAddr = devAddr;
	ip->chanmap[ip->en_count].func = &i2c_ina219_read_current;
	ip->en_count++;

	return true;
}

/*!
 * Adds four entries to the channel map for a specified ADS1015 device,
 * representing the four single ended measurement channels
 *
 * - `baseID`: A0 [V]
 * - `baseID+1`: A1 [V]
 * - `baseID+2`: A2 [V]
 * - `baseID+3`: A3 [V]
 *
 * @param[in] ip i2c_params structure to modify
 * @param[in] devAddr ADS1015 Device Address
 * @param[in] baseID Message ID for first channel (A0)
 * @param[in] scale Multiply received values by this quantity
 * @param[in] offset Add this amount to received values
 * @param[in] minV Minimum valid value
 * @param[in] maxV Maximum valid value
 * @return True on success, false otherwise
 */
bool i2c_chanmap_add_ads1015(i2c_params *ip, const uint8_t devAddr, const uint8_t baseID,
                             const float scale, const float offset, const float minV,
                             const float maxV) {
	if (!ip) { return false; }
	if (baseID < 4 || baseID > 121) { return false; }
	i2c_msg_map *tmp = realloc(ip->chanmap, sizeof(i2c_msg_map) * (ip->en_count + 4));
	if (!tmp) { return false; }
	if ((minV == maxV) || (maxV < minV)) { return false; }

	ip->chanmap = tmp;

	char tmpS[50] = {0};
	i2c_ads1015_options *adsopts = calloc(1, sizeof(i2c_ads1015_options));
	if (adsopts == NULL) {
		perror("_add_ads1015");
		return false;
	}

	adsopts->min = minV;
	adsopts->max = maxV;
	adsopts->scale = scale;
	adsopts->offset = offset;

	snprintf(tmpS, 8, "0x%02x:A0", devAddr);
	ip->chanmap[ip->en_count].messageID = baseID;
	// Memory was allocated with malloc, so make sure we initialise these values
	ip->chanmap[ip->en_count].message_name.length = 0;
	ip->chanmap[ip->en_count].message_name.data = NULL;
	str_update(&(ip->chanmap[ip->en_count].message_name), 8, tmpS);
	ip->chanmap[ip->en_count].deviceAddr = devAddr;
	ip->chanmap[ip->en_count].func = &i2c_ads1015_read_ch0;
	ip->chanmap[ip->en_count].ext = adsopts;
	ip->en_count++;

	snprintf(tmpS, 8, "0x%02x:A1", devAddr);
	ip->chanmap[ip->en_count].messageID = baseID + 1;
	ip->chanmap[ip->en_count].message_name.length = 0;
	ip->chanmap[ip->en_count].message_name.data = NULL;
	str_update(&(ip->chanmap[ip->en_count].message_name), 8, tmpS);
	ip->chanmap[ip->en_count].deviceAddr = devAddr;
	ip->chanmap[ip->en_count].func = &i2c_ads1015_read_ch1;
	ip->chanmap[ip->en_count].ext = adsopts;
	ip->en_count++;

	snprintf(tmpS, 8, "0x%02x:A2", devAddr);
	ip->chanmap[ip->en_count].messageID = baseID + 2;
	ip->chanmap[ip->en_count].message_name.length = 0;
	ip->chanmap[ip->en_count].message_name.data = NULL;
	str_update(&(ip->chanmap[ip->en_count].message_name), 8, tmpS);
	ip->chanmap[ip->en_count].deviceAddr = devAddr;
	ip->chanmap[ip->en_count].func = &i2c_ads1015_read_ch2;
	ip->chanmap[ip->en_count].ext = adsopts;
	ip->en_count++;

	snprintf(tmpS, 8, "0x%02x:A3", devAddr);
	ip->chanmap[ip->en_count].messageID = baseID + 3;
	ip->chanmap[ip->en_count].message_name.length = 0;
	ip->chanmap[ip->en_count].message_name.data = NULL;
	str_update(&(ip->chanmap[ip->en_count].message_name), 8, tmpS);
	ip->chanmap[ip->en_count].deviceAddr = devAddr;
	ip->chanmap[ip->en_count].func = &i2c_ads1015_read_ch3;
	ip->chanmap[ip->en_count].ext = adsopts;
	ip->en_count++;

	return true;
}

/*!
 * @param[in] lta Pointer to log_thread_args_t
 * @param[in] s Pointer to config_section to be parsed
 * @returns True on success, false on error
 */
bool i2c_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[I2C:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	i2c_params *ip = calloc(1, sizeof(i2c_params));
	if (!ip) {
		log_error(lta->pstate, "[I2C:%s] Unable to allocate memory for device parameters",
		          lta->tag);
		return false;
	}
	(*ip) = i2c_getParams();

	config_kv *t = NULL;
	if ((t = config_get_key(s, "name"))) {
		ip->sourceName = config_qstrdup(t->value);
	} else {
		// Must set a name, so nick the tag value
		ip->sourceName = strdup(lta->tag);
	}
	t = NULL;

	if ((t = config_get_key(s, "bus"))) { ip->busName = config_qstrdup(t->value); }
	t = NULL;

	if ((t = config_get_key(s, "frequency"))) {
		errno = 0;
		ip->frequency = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[I2C:%s] Error parsing sample frequency: %s",
			          lta->tag, strerror(errno));
			free(ip);
			return false;
		}
		if (ip->frequency <= 0) {
			log_error(
				lta->pstate,
				"[I2C:%s] Invalid frequency requested (%d) - must be positive and non-zero",
				lta->tag, ip->frequency);
			free(ip);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "sourcenum"))) {
		errno = 0;
		int sn = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[I2C:%s] Error parsing source number: %s",
			          lta->tag, strerror(errno));
			free(ip);
			return false;
		}
		if (sn < 0) {
			log_error(lta->pstate, "[I2C:%s] Invalid source number (%s)", lta->tag,
			          t->value);
			free(ip);
			return false;
		}
		if (sn < 10) {
			ip->sourceNum += sn;
		} else {
			ip->sourceNum = sn;
			if (sn < SLSOURCE_I2C || sn > (SLSOURCE_I2C + 0x0F)) {
				log_warning(
					lta->pstate,
					"[I2C:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems",
					lta->tag, sn);
			}
		}
	}
	t = NULL;

	// Initial message ID, in case not specified in configuration.
	// There is scope for conflict in case of a mix of automatic and
	// manually specified IDs, which should be caught by i2c_validate_chanmap
	uint8_t baseMsgID = 4;

	// Loop over all keys present, as the options parsed below this line may be
	// present multiple times
	for (int i = 0; i < s->numopts; i++) {
		t = &(s->opts[i]);
		if (strncasecmp(t->key, "ina219", 7) == 0) {
			char *strtsp = NULL;
			if (strtok_r(t->value, ":", &strtsp)) {
				// First part is base address, second is base message ID
				errno = 0;
				int baddr = strtol(t->value, NULL, 16);
				int msgid = strtol(strtok_r(NULL, ":", &strtsp), NULL, 16);
				if (!i2c_chanmap_add_ina219(ip, baddr, msgid)) {
					log_error(
						lta->pstate,
						"[I2C:%s] Failed to register INA219 device at address 0x%02x and base message ID 0x%02x",
						lta->tag, baddr, msgid);
					free(ip);
					return false;
				}

			} else {
				// No base message ID, so start from next available
				errno = 0;
				int baddr = strtol(t->value, NULL, 16);
				if (!i2c_chanmap_add_ina219(ip, baddr, baseMsgID)) {
					log_error(
						lta->pstate,
						"[I2C:%s] Failed to register INA219 device at address 0x%02x and base message ID 0x%02x",
						lta->tag, baddr, baseMsgID);
					free(ip);
					return false;
				}
				baseMsgID += 3;
			}
		} else if (strncasecmp(t->key, "ads1015", 8) == 0) {
			char *strtsp = NULL;
			if (strtok_r(t->value, ":", &strtsp)) {
				// First part is base address, second is base message ID
				// Optional third and fourth parts are scale and offset
				// respectively
				errno = 0;
				int baddr = strtol(t->value, NULL, 16);
				int msgid = strtol(strtok_r(NULL, ":", &strtsp), NULL, 16);
				if (errno) {
					log_error(lta->pstate,
					          "[I2C:%s] Error parsing values in configuration",
					          lta->tag);
					free(ip);
					return false;
				}

				float scale = 1;
				float offset = 0;
				float min = -INFINITY;
				float max = INFINITY;
				char *tt = strtok_r(NULL, ":", &strtsp);
				if (tt) {
					scale = strtof(tt, NULL);
					if (errno) {
						log_error(
							lta->pstate,
							"[I2C:%s] Error parsing scale value in configuration",
							lta->tag);
						free(ip);
						return false;
					}

					tt = strtok_r(NULL, ":", &strtsp);
					if (tt) {
						errno = 0;
						offset = strtof(tt, NULL);
						if (errno) {
							log_error(
								lta->pstate,
								"[I2C:%s] Error parsing offset value in configuration",
								lta->tag);
							free(ip);
							return false;
						}
						tt = strtok_r(NULL, ":", &strtsp);
						if (tt) {
							errno = 0;
							min = strtof(tt, NULL);
							if (errno) {
								log_error(
									lta->pstate,
									"[I2C:%s] Error parsing minimum value in configuration",
									lta->tag);
								free(ip);
								return false;
							}
							tt = strtok_r(NULL, ":", &strtsp);
							if (tt) {
								errno = 0;
								max = strtof(tt, NULL);
								if (errno) {
									log_error(
										lta->pstate,
										"[I2C:%s] Error parsing maximum value in configuration",
										lta->tag);
									free(ip);
									return false;
								}
							}
						}
					}
				}

				if (!i2c_chanmap_add_ads1015(ip, baddr, msgid, scale, offset, min,
				                             max)) {
					log_error(
						lta->pstate,
						"[I2C:%s] Failed to register ADS1015 device at address 0x%02x and base message ID 0x%02x",
						lta->tag, baddr, msgid);
					free(ip);
					return false;
				}

			} else {
				// No base message ID, so start from next available
				errno = 0;
				int baddr = strtol(t->value, NULL, 16);
				if (!i2c_chanmap_add_ads1015(ip, baddr, baseMsgID, 1, 0, -INFINITY,
				                             INFINITY)) {
					log_error(
						lta->pstate,
						"[I2C:%s] Failed to register ADS1015 device at address 0x%02x and base message ID 0x%02x",
						lta->tag, baddr, baseMsgID);
					free(ip);
					return false;
				}
				baseMsgID += 4;
			}
		}
	}
	lta->dParams = ip;
	return i2c_validate_chanmap(ip);
}
