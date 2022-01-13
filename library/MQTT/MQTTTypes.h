#ifndef SELKIELoggerMQTT_Types
#define SELKIELoggerMQTT_Types

/*!
 * @file MQTTTypes.h Serial Data types and definitions for communication with MQTT devices
 * @ingroup SELKIELoggerMQTT
 */

#include <stdbool.h>
#include "SELKIELoggerBase.h"

/*!
 * @addtogroup SELKIELoggerMQTT
 * @{
 */

enum dtype {
	MQTT_TEXT,
	MQTT_NUMERIC
} ;

typedef struct {
	char *name;
	enum dtype type;
	union {
		string *text;
		float value;
	} data;
} mqtt_msg_t;


typedef struct {
	msgqueue *q;
	uint8_t sourcenum;
	strarray topics;
	uint8_t *msgnums;
	bool *msgtext;
	bool dumpall;
} mqtt_queue_map;

//! @}
#endif
