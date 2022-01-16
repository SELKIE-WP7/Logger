#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "MQTTConnection.h"

#include "SELKIELoggerBase.h"

mqtt_conn *mqtt_openConnection(const char *host, const int port, mqtt_queue_map *qm) {
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

void mqtt_closeConnection(mqtt_conn *conn) {
	mosquitto_disconnect(conn);
	mosquitto_loop_stop(conn, true);
	mosquitto_destroy(conn);
}

bool mqtt_subscribe_batch(mqtt_conn *conn, mqtt_queue_map *qm) {
#if LIBMOSQUITTO_VERSION_NUMBER > 1006000
	char **topStr = calloc(sizeof(char *), qm->numtopics);
	if (topStr == NULL) {
		perror("mqtt_subscribe_batch:topStr");
		return false;
	}
	for (int i=0; i < qm->numtopics; i++) {
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
	for (int i=0; i < qm->numtopics; i++) {
		if(mosquitto_subscribe(conn, NULL, qm->tc[i].topic, 0) != MOSQ_ERR_SUCCESS) {
			perror("mqtt_subscribe_batch:mosquitto");
			return false;
		}
	}
#endif

	return true;
}

void mqtt_enqueue_messages(mqtt_conn *conn, void *userdat_qm, const struct mosquitto_message *inmsg) {
	mqtt_queue_map *qm = (mqtt_queue_map *) (userdat_qm);

	int ix= -1;
	for (int m=0; m < qm->numtopics; m++) {
		if (strcasecmp(inmsg->topic, qm->tc[m].topic) == 0) {
			// Found it!
			ix = m;
			break;
		}
	}

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
