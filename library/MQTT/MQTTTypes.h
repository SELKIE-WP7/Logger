#ifndef SELKIELoggerMQTT_Types
#define SELKIELoggerMQTT_Types

/*!
 * @file MQTTTypes.h Serial Data types and definitions for communication with MQTT devices
 * @ingroup SELKIELoggerMQTT
 */

#include <stdbool.h>
#include <mosquitto.h>
#include "SELKIELoggerBase.h"

/*!
 * @addtogroup SELKIELoggerMQTT
 * @{
 */

typedef struct mosquitto mqtt_conn;

typedef struct {
	msgqueue q;
	uint8_t sourcenum;
	strarray topics;
	strarray names;
	uint8_t *msgnums;
	bool *msgtext;
	bool dumpall;
} mqtt_queue_map;

bool mqtt_init_queue_map(mqtt_queue_map *qm, const int entries);
void mqtt_destroy_queue_map(mqtt_queue_map *qm);
//! @}
#endif
