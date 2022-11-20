#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "MQTTConnection.h"

#include "SELKIELoggerBase.h"

/*!
 * Connects to the specified host and port, then configures it based on the configuration in an mqtt_queue_map instance.
 * Sets mqtt_enqueue_messages as the callback for new messages and starts the mosquitto event loop.
 *
 * @param[in] host Hostname or IP address (as string)
 * @param[in] port Port number
 * @param[in] qm mqtt_queue_map instance to use for configuration
 * @return Pointer to mqtt_conn structure on success, NULL on error
 */
mqtt_conn *mqtt_openConnection(const char *host, const int port, mqtt_queue_map *qm) {
	if (host == NULL || qm == NULL || port < 0) { return NULL; }

	if (mosquitto_lib_init() != MOSQ_ERR_SUCCESS) {
		perror("mosquitto_lib_init");
		return NULL;
	}

	mqtt_conn *conn = {0};
	conn = mosquitto_new(NULL, true, qm);
	if (conn == NULL) {
		perror("mosquitto_new");
		return NULL;
	}

	if (mosquitto_connect(conn, host, port, 30) != MOSQ_ERR_SUCCESS) {
		perror("mosquitto_connect");
		mosquitto_destroy(conn);
		return NULL;
	}

	mosquitto_message_callback_set(conn, &mqtt_enqueue_messages);

	if (mosquitto_loop_start(conn) != MOSQ_ERR_SUCCESS) {
		perror("mosquitto_loop_start");
		mosquitto_destroy(conn);
		return NULL;
	}

	return conn;
}

/*!
 * Disconnects from server (if still connected), stops the mosquitto event loop
 * and frees resources associated with the connection.
 *
 * @param[in,out] conn Connection to close out
 */
void mqtt_closeConnection(mqtt_conn *conn) {
	mosquitto_disconnect(conn);
	mosquitto_loop_stop(conn, true);
	mosquitto_destroy(conn);
}

/*!
 * Subscribes to all configured topics in an mqtt_queue_map using
 * mosquitto_subscribe_multiple (if available) or by falling back to placing
 * multiple individual subscription requests if not.
 * @param[in] conn Pointer to MQTT Connection structure
 * @param[in] qm Pointer to mqtt_queue_map
 * @return True on success, false on error
 */
bool mqtt_subscribe_batch(mqtt_conn *conn, mqtt_queue_map *qm) {
	if (conn == NULL || qm == NULL) { return false; }
#if LIBMOSQUITTO_VERSION_NUMBER > 1006000
	char **topStr = calloc(sizeof(char *), qm->numtopics);
	if (topStr == NULL) {
		perror("mqtt_subscribe_batch:topStr");
		return false;
	}
	for (int i = 0; i < qm->numtopics; i++) {
		// Essentially, assemble topStr as an array of classical char * strings
		topStr[i] = qm->tc[i].topic;
	}

	if ((mosquitto_subscribe_multiple(conn, NULL, qm->numtopics, topStr, 0, 0, NULL)) != MOSQ_ERR_SUCCESS) {
		perror("mqtt_subscribe_batch:mosquitto");
		free(topStr);
		return false;
	}
	free(topStr);
#else
	for (int i = 0; i < qm->numtopics; i++) {
		if (mosquitto_subscribe(conn, NULL, qm->tc[i].topic, 0) != MOSQ_ERR_SUCCESS) {
			perror("mqtt_subscribe_batch:mosquitto");
			return false;
		}
	}
#endif

	return true;
}

/*!
 * Registered with mosquitto as a message callback by mqtt_openConnection and
 * called by the mosquitto event loop for every message matching our
 * subscriptions.
 *
 * If mqtt_queue_map.dumpall is true, messages not matching a configured topic
 * will be queued under SLCHAN_RAW as strings with the format "topic: payload".
 * Otherwise they will be ignored.
 *
 * Zero length messages are never queued.
 *
 * @param[in] conn Pointer to MQTT Connection structure
 * @param[in] userdat_qm mqtt_queue_map - passed as void pointer by mosquitto library
 * @param[in] inmsg Incoming message
 */
void mqtt_enqueue_messages(mqtt_conn *conn, void *userdat_qm, const struct mosquitto_message *inmsg) {
	(void) conn; // Deliberately unused

	mqtt_queue_map *qm = (mqtt_queue_map *)(userdat_qm);

	int ix = -1;
	for (int m = 0; m < qm->numtopics; m++) {
		if (strcasecmp(inmsg->topic, qm->tc[m].topic) == 0) {
			// Found it!
			ix = m;
			break;
		}
	}
	if (inmsg->payloadlen == 0) { return; } // Don't queue zero sized messages
	msg_t *out = NULL;
	if (ix < 0) {
		// Not a message we want

		// If we arent' dumping all messages into the queue then exit now
		if (!qm->dumpall) { return; }

		size_t msglen = strlen(inmsg->topic) + inmsg->payloadlen + 2;
		char *mqstr = calloc(msglen, sizeof(char));
		if (mqstr == NULL) {
			perror("mqtt_enqueue_messages:mqstr");
			return;
		}
		snprintf(mqstr, msglen, "%s: %s", inmsg->topic, (char *)inmsg->payload);
		out = msg_new_string(qm->sourceNum, SLCHAN_RAW, msglen, mqstr);
		free(mqstr);
	} else {
		// This message is needed and has an allocated channel number
		if (qm->tc[ix].text) {
			out = msg_new_string(qm->sourceNum, qm->tc[ix].type, inmsg->payloadlen, inmsg->payload);
		} else {
			float val = strtof(inmsg->payload, NULL);
			out = msg_new_float(qm->sourceNum, qm->tc[ix].type, val);
		}
	}
	if (!queue_push(&qm->q, out)) {
		perror("mqtt_enqueue_messages:queue_push");
		return;
	}
	return;
}

/*!
 * Sends a keepalive request for all configured topics in the format required
 * for Victron MQTT systems.
 *
 * @param[in] conn Pointer to MQTT Connection structure
 * @param[in] qm Pointer to mqtt_queue_map
 * @param[in] sysid System serial number
 * @return True on success, false on error
 */
bool mqtt_victron_keepalive(mqtt_conn *conn, mqtt_queue_map *qm, char *sysid) {
	if (sysid == NULL || qm == NULL || conn == NULL) { return false; }
	// R/<sysid>/keepalive
	const size_t toplen = strlen(sysid) + 13;
	char *topic = calloc(toplen, sizeof(char));
	if (topic == NULL) {
		perror("mqtt_victron_keepalive:topic");
		return false;
	}
	if (snprintf(topic, toplen, "R/%s/keepalive", sysid) < 0) {
		free(topic);
		return false;
	}

	const size_t prefixlen = 3 + strlen(sysid); // + 3 as looking for "N/<sysid>/"
	size_t payloadlen = 3;                      // [ + ] + \0
	for (int t = 0; t < qm->numtopics; t++) {
		payloadlen += strlen(qm->tc[t].topic) + 4; // topic + quotes + , + space
	}

	char *payload = calloc(payloadlen, sizeof(char));
	for (int t = 0; t < qm->numtopics; t++) {
		char *tmp = strdup(payload);
		char *target = NULL;
		if (strlen(qm->tc[t].topic) > (prefixlen + 1)) { // Need prefix and <at least one char>
			target = &(qm->tc[t].topic[prefixlen]);
		} else {
			target = qm->tc[t].topic;
		}
		if (snprintf(payload, payloadlen, "%s%s\"%s\"", tmp, (t == 0 ? "[" : ", "), target) < 0) {
			perror("mqtt_victron_keepalive:payload");
			free(tmp);
			free(payload);
			free(topic);
			return false;
		}
		free(tmp);
	}
	char *tmp = strdup(payload);
	if (snprintf(payload, payloadlen, "%s]", tmp) < 0) {
		perror("mqtt_victron_keepalive:payload-end");
		free(tmp);
		free(payload);
		free(topic);
		return false;
	}
	free(tmp);
	tmp = NULL;

	int rc = mosquitto_publish(conn, NULL, topic, strlen(payload), payload, 0, false);
	if (rc != MOSQ_ERR_SUCCESS) {
		perror("mqtt_victron_keepalive:publish");
		free(payload);
		free(topic);
		return false;
	}
	return true;
}
