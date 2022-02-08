#ifndef SELKIELoggerMQTT_Types
#define SELKIELoggerMQTT_Types

/*!
 * @file MQTTTypes.h Serial Data types and definitions for communication with MQTT devices
 * @ingroup SELKIELoggerMQTT
 */

#include "SELKIELoggerBase.h"
#include <mosquitto.h>
#include <stdbool.h>

/*!
 * @addtogroup SELKIELoggerMQTT
 * @{
 */

typedef struct mosquitto mqtt_conn;

//! MQTT Topic mapping
typedef struct {
	uint8_t type;
	char *topic;
	char *name;
	bool text;
} mqtt_topic_config;

typedef struct {
	msgqueue q;
	uint8_t sourceNum;
	int numtopics;
	mqtt_topic_config tc[120];
	bool dumpall;
} mqtt_queue_map;

bool mqtt_init_queue_map(mqtt_queue_map *qm);
void mqtt_destroy_queue_map(mqtt_queue_map *qm);
//! @}
#endif
