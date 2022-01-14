#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "MQTTTypes.h"

bool mqtt_init_queue_map(mqtt_queue_map *qm, const int entries) {
	queue_init(&qm->q);
	sa_init(&qm->topics, entries);

	qm->msgnums = calloc(entries, sizeof(uint8_t));
	if (qm->msgnums == NULL) {
		perror("mqtt_init_queue_map:msgnums");
		mqtt_destroy_queue_map(qm);
		return false;
	}

	qm->msgtext = calloc(entries, sizeof(bool));
	if (qm->msgtext == NULL) {
		perror("mqtt_init_queue_map:msgtext");
		mqtt_destroy_queue_map(qm);
		return false;
	}
	return true;
}

void mqtt_destroy_queue_map(mqtt_queue_map *qm) {
	sa_destroy(&qm->topics);
	free(qm->msgnums);
	free(qm->msgtext);
}
