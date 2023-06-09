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

#include <time.h>

#include "Logger.h"

#include "LoggerMQTT.h"

/*!
 * Sets up MQTT server connection
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Error code stored in ptarges->returnCode
 */
void *mqtt_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	mqtt_params *mqttInfo = (mqtt_params *)args->dParams;

	queue_init(&(mqttInfo->qm.q));
	mqttInfo->qm.sourceNum = mqttInfo->sourceNum;
	mqttInfo->conn = mqtt_openConnection(mqttInfo->addr, mqttInfo->port, &(mqttInfo->qm));
	if (mqttInfo->conn == NULL) {
		log_error(args->pstate, "[MQTT:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}

	if (!mqtt_subscribe_batch(mqttInfo->conn, &(mqttInfo->qm))) {
		log_error(args->pstate, "Unable to subscribe to topics");
		return NULL;
	}

	log_info(args->pstate, 2, "[MQTT:%s] Connected", args->tag);
	args->returnCode = 0;
	return NULL;
}

/*!
 * MQTT Logging thread
 *
 * Handles sending keepalive commands (if enabled) and popping items from
 * within the MQTT specific queue and moving them into the main message queue.
 *
 * Unlike the _logging function for other sources, this doesn't read anything
 * directly. Message processing is handled in a thread managed by the mosquitto
 * library and we pass in a callback function to format and enqueue the
 * messages.
 *
 * Terminates thread on error
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Error code stored in ptarges->returnCode
 */
void *mqtt_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	mqtt_params *mqttInfo = (mqtt_params *)args->dParams;
	log_info(args->pstate, 1, "[MQTT:%s] Logging thread started", args->tag);

	time_t lastKA = 0;
	time_t lastMessage = 0;
	uint16_t count = 0; // Unsigned so that it wraps around
	while (!shutdownFlag) {
		if (mqttInfo->victron_keepalives &&
		    (count % 200) == 0) { // Check ~ every 10 seconds
			time_t now = time(NULL);
			if ((now - lastKA) >= mqttInfo->keepalive_interval) {
				lastKA = now;
				if (!mqtt_victron_keepalive(mqttInfo->conn, &(mqttInfo->qm),
				                            mqttInfo->sysid)) {
					log_warning(args->pstate,
					            "[MQTT:%s] Error sending keepalive message",
					            args->tag);
				}
			}
			if ((now - lastMessage) > 180) {
				log_warning(
					args->pstate,
					"[MQTT:%s] More than 3 minutes since last message. Resetting timer",
					args->tag);
				lastMessage = now;
			}
		}
		count++;

		msg_t *in = NULL;
		in = queue_pop(&mqttInfo->qm.q);
		if (in == NULL) {
			usleep(5E4);
			continue;
		}
		if (!queue_push(args->logQ, in)) {
			log_error(args->pstate, "[MQTT:%s] Error pushing message to queue",
			          args->tag);
			msg_destroy(in);
			args->returnCode = -1;
			pthread_exit(&(args->returnCode));
		}
		lastMessage = time(NULL);
	}
	return NULL;
}

/*!
 * Cleanly shutdown MQTT connection and release resources.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL
 */
void *mqtt_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	mqtt_params *mqttInfo = (mqtt_params *)args->dParams;
	mqtt_closeConnection(mqttInfo->conn);
	mqtt_destroy_queue_map(&(mqttInfo->qm));
	if (mqttInfo->addr) {
		free(mqttInfo->addr);
		mqttInfo->addr = NULL;
	}
	if (mqttInfo->sourceName) {
		free(mqttInfo->sourceName);
		mqttInfo->sourceName = NULL;
	}
	if (mqttInfo->sysid) {
		free(mqttInfo->sysid);
		mqttInfo->sysid = NULL;
	}
	return NULL;
}

/*!
 * Create channel map from configured IDs and push to queue.
 *
 * Terminates thread on error.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Error code stored in ptarges->returnCode
 */
void *mqtt_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	mqtt_params *mqttInfo = (mqtt_params *)args->dParams;

	msg_t *m_sn = msg_new_string(mqttInfo->sourceNum, SLCHAN_NAME,
	                             strlen(mqttInfo->sourceName), mqttInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[MQTT:%s] Error pushing channel name to queue",
		          args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(4 + mqttInfo->qm.numtopics);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 1, "-");
	for (int t = 0; t < mqttInfo->qm.numtopics; t++) {
		sa_create_entry(channels, 4 + t, strlen(mqttInfo->qm.tc[t].name),
		                mqttInfo->qm.tc[t].name);
	}

	msg_t *m_cmap = msg_new_string_array(mqttInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[MQTT:%s] Error pushing channel map to queue", args->tag);
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
 * @returns device_callbacks for MQTT network sources
 */
device_callbacks mqtt_getCallbacks() {
	device_callbacks cb = {.startup = &mqtt_setup,
	                       .logging = &mqtt_logging,
	                       .shutdown = &mqtt_shutdown,
	                       .channels = &mqtt_channels};
	return cb;
}

/*!
 * @returns Default parameters for MQTT network sources
 */
mqtt_params mqtt_getParams() {
	mqtt_params mp = {
		.sourceName = NULL,
		.sourceNum = 0x68,
		.addr = NULL,
		.port = 1883,
		.victron_keepalives = false,
		.keepalive_interval = 30,
		.sysid = NULL,
		.conn = NULL,
		.qm = {{0}, 0, 0, {{0}}, false},
	};
	return mp;
}

/*!
 * @param[in] lta Pointer to log_thread_args_t
 * @param[in] s Pointer to config_section to be parsed
 * @returns True on success, false on error
 */
bool mqtt_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[MQTT:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	mqtt_params *mqtt = calloc(1, sizeof(mqtt_params));
	if (!mqtt) {
		log_error(lta->pstate, "[MQTT:%s] Unable to allocate memory for device parameters",
		          lta->tag);
		return false;
	}
	(*mqtt) = mqtt_getParams();

	for (int i = 0; i < s->numopts; i++) {
		config_kv *t = &(s->opts[i]);
		if (strcasecmp(t->key, "host") == 0) {
			mqtt->addr = config_qstrdup(t->value);
		} else if (strcasecmp(t->key, "port") == 0) {
			errno = 0;
			mqtt->port = strtol(t->value, NULL, 0);
			if (errno) {
				log_error(lta->pstate, "[MQTT:%s] Error parsing port number: %s",
				          lta->tag, strerror(errno));
				free(mqtt);
				return false;
			}
		} else if (strcasecmp(t->key, "name") == 0) {
			mqtt->sourceName = config_qstrdup(t->value);
		} else if (strcasecmp(t->key, "sourcenum") == 0) {
			errno = 0;
			int sn = strtol(t->value, NULL, 0);
			if (errno) {
				log_error(lta->pstate, "[MQTT:%s] Error parsing source number: %s",
				          lta->tag, strerror(errno));
				free(mqtt);
				return false;
			}
			if (sn < 0) {
				log_error(lta->pstate, "[MQTT:%s] Invalid source number (%s)",
				          lta->tag, t->value);
				free(mqtt);
				return false;
			}
			if (sn < 10) {
				mqtt->sourceNum += sn;
			} else {
				mqtt->sourceNum = sn;
				if (sn < SLSOURCE_EXT || sn > (SLSOURCE_EXT + 0x0F)) {
					log_warning(
						lta->pstate,
						"[MQTT:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems",
						lta->tag, sn);
				}
			}
			mqtt->qm.sourceNum = mqtt->sourceNum;
		} else if (strcasecmp(t->key, "victron_keepalives") == 0) {
			int tmp = config_parse_bool(t->value);
			if (tmp < 0) {
				log_error(
					lta->pstate,
					"[MQTT:%s] Invalid value provided for 'victron_keepalives': %s",
					lta->tag, t->value);
				free(mqtt);
				return false;
			}
			mqtt->victron_keepalives = (tmp > 0);
		} else if (strcasecmp(t->key, "sysid") == 0) {
			if (mqtt->sysid != NULL) {
				log_error(
					lta->pstate,
					"[MQTT:%s] Only a single system ID is supported for Victron keepalive messages",
					lta->tag);
				free(mqtt);
				return false;
			}
			mqtt->sysid = strdup(t->value);
		} else if (strcasecmp(t->key, "keepalive_interval") == 0) {
			errno = 0;
			int ki = strtol(t->value, NULL, 0);
			if (errno) {
				log_error(lta->pstate,
				          "[MQTT:%s] Error parsing keepalive interval: %s",
				          lta->tag, strerror(errno));
				free(mqtt);
				return false;
			}
			mqtt->keepalive_interval = ki;
		} else if (strcasecmp(t->key, "dumpall") == 0) {
			int tmp = config_parse_bool(t->value);
			if (tmp < 0) {
				log_error(lta->pstate,
				          "[MQTT:%s] Invalid value provided for 'dumpall': %s",
				          lta->tag, t->value);
				free(mqtt);
				return false;
			}
			mqtt->qm.dumpall = (tmp > 0);
		} else if (strcasecmp(t->key, "topic") == 0) {
			int n = mqtt->qm.numtopics++;
			char *strtsp = NULL;
			mqtt->qm.tc[n].type = 4 + n;
			mqtt->qm.tc[n].text = true;
			char *token = NULL;
			if ((token = strtok_r(t->value, ":", &strtsp))) {
				// Topic:ChannelName:text
				mqtt->qm.tc[n].topic = strdup(token);
				mqtt->qm.tc[n].name = strdup(strtok_r(NULL, ":", &strtsp));
				if ((token = strtok_r(NULL, ":", &strtsp))) {
					int tmp = config_parse_bool(token);
					if (tmp < 0) {
						log_error(
							lta->pstate,
							"[MQTT:%s] Invalid textmode specifier (%s) for topic '%s' ",
							lta->tag, t->value, mqtt->qm.tc[n].topic);
						free(mqtt);
						return false;
					}
					mqtt->qm.tc[n].text = (tmp > 0);
				}
			} else {
				mqtt->qm.tc[n].topic = strdup(t->value);
				mqtt->qm.tc[n].name = strdup(t->value);
			}
		} else {
			if (!(strcasecmp(t->key, "type") == 0 || strcasecmp(t->key, "tag") == 0)) {
				log_warning(lta->pstate,
				            "[MQTT:%s] Unrecognised configuration option: %s",
				            lta->tag, t->key);
			}
		}
	}

	if (mqtt->qm.numtopics < 1) {
		log_error(lta->pstate, "[MQTT:%s] Must provide at least one topic to log",
		          lta->tag);
		free(mqtt);
		return false;
	}
	if (mqtt->victron_keepalives && mqtt->sysid == NULL) {
		log_error(
			lta->pstate,
			"[MQTT:%s] Must provide a system ID if enabling Victron-style keepalive messages",
			lta->tag);
		free(mqtt);
		return false;
	}
	lta->dParams = mqtt;
	return true;
}
