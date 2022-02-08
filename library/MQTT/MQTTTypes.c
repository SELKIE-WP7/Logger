#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "MQTTTypes.h"

bool mqtt_init_queue_map(mqtt_queue_map *qm) {
	queue_init(&qm->q);
	qm->numtopics = 0;
	for (int i = 0; i < 120; i++) {
		qm->tc[i].topic = NULL;
		qm->tc[i].name = NULL;
	}
	return true;
}

void mqtt_destroy_queue_map(mqtt_queue_map *qm) {
	queue_destroy(&qm->q);
	for (int i = 0; i < 120; i++) {
		if (qm->tc[i].topic) {
			free(qm->tc[i].topic);
			qm->tc[i].topic = NULL;
		}
		if (qm->tc[i].name) {
			free(qm->tc[i].name);
			qm->tc[i].name = NULL;
		}
	}
}
