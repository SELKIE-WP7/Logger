#include "Logger.h"
#include "LoggerMQTT.h"

/*!
 * Sets up MQTT server connection
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *mqtt_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	mqtt_params *mqttInfo = (mqtt_params *) args->dParams;

	mqttInfo->conn = mqtt_openConnection(mqttInfo->addr, mqttInfo->port, &mqttInfo->qm);
	if (mqttInfo->conn == NULL) {
		log_error(args->pstate, "[MQTT:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}

	if (!mqtt_subscribe_batch(mqttInfo->conn, &(mqttInfo->qm.topics))) {
		log_error(args->pstate, "Unable to subscribe to topics");
		return NULL;
	}

	log_info(args->pstate, 2, "[MQTT:%s] Connected", args->tag);
	args->returnCode = 0;
	return NULL;
}

void *mqtt_logging(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	// mqtt_params *mqttInfo = (mqtt_params *) args->dParams;
	log_error(args->pstate, "[MQTT:%s] Logging not implemented", args->tag);
	return NULL;
}

void *mqtt_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	mqtt_params *mqttInfo = (mqtt_params *) args->dParams;
	mqtt_closeConnection(mqttInfo->conn);
	mqtt_destroy_queue_map(&(mqttInfo->qm));
	return NULL;
}

void *mqtt_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *) ptargs;
	mqtt_params *mqttInfo = (mqtt_params *) args->dParams;

	msg_t *m_sn = msg_new_string(mqttInfo->sourceNum, SLCHAN_NAME, strlen(mqttInfo->sourceName), mqttInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[MQTT:%s] Error pushing channel name to queue", args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(4 + mqttInfo->qm.names.entries);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 1, "-");
	for (int t=0; t < mqttInfo->qm.names.entries; t++) {
		sa_set_entry(channels, 4+t, &(mqttInfo->qm.names.strings[t]));
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

device_callbacks mqtt_getCallbacks() {
	device_callbacks cb = {
		.startup = &mqtt_setup,
		.logging = &mqtt_logging,
		.shutdown = &mqtt_shutdown,
		.channels = &mqtt_channels
	};
	return cb;
}

mqtt_params mqtt_getParams() {
	mqtt_params mp = {
		.sourceName = NULL,
		.sourceNum = 0x68,
		.addr = NULL,
		.port = 1883,
		.conn = NULL,
		.qm = {{0}},
	};
	return mp;
}

bool mqtt_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[MQTT:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	mqtt_params *mqtt = calloc(1, sizeof(mqtt_params));
	if (!mqtt) {
		log_error(lta->pstate, "[MQTT:%s] Unable to allocate memory for device parameters", lta->tag);
		return false;
	}
	(*mqtt) = mqtt_getParams();

	for (int i=0; i < s->numopts; i++) {
		config_kv *t = &(s->opts[i]);
		if (strcasecmp(t->key, "host") == 0) {
			mqtt->addr = config_qstrdup(t->value);
		} else if (strcasecmp(t->key, "port") == 0) {
			errno = 0;
			mqtt->port = strtol(t->value, NULL, 0);
			if (errno) {
				log_error(lta->pstate, "[MQTT:%s] Error parsing port number: %s", lta->tag, strerror(errno));
				return false;
			}
		} else if (strcasecmp(t->key, "name") == 0) {
			mqtt->sourceName = config_qstrdup(t->value);
		} else if (strcasecmp(t->key, "sourcenum") == 0) {
			errno = 0;
			int sn = strtol(t->value, NULL, 0);
			if (errno) {
				log_error(lta->pstate, "[MQTT:%s] Error parsing source number: %s", lta->tag, strerror(errno));
				return false;
			}
			if (sn < 0) {
				log_error(lta->pstate, "[MQTT:%s] Invalid source number (%s)", lta->tag, t->value);
				return false;
			}
			if (sn < 10) {
				mqtt->sourceNum += sn;
			} else {
				mqtt->sourceNum = sn;
				if (sn < SLSOURCE_EXT || sn > (SLSOURCE_EXT + 0x0F)) {
					log_warning(lta->pstate, "[MQTT:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems", lta->tag, sn);
				}
			}
		} else if (strcasecmp(t->key, "topic") == 0) {
			log_error(lta->pstate, "[MQTT:%s] Configuring topics not yet implemented", lta->tag);
		} else {
			log_warning(lta->pstate, "[MQTT:%s] Unrecognised configuration option: %s", lta->tag, t->key);
		}
	}

	lta->dParams = mqtt;
	return true;
}
