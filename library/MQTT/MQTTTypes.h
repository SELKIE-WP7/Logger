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

//! Convenient alias for library structure
typedef struct mosquitto mqtt_conn;

//! MQTT Topic mapping
typedef struct {
	uint8_t type; //!< Channel number to use
	char *topic;  //!< MQTT topic to subscribe/match against
	char *name;   //!< Channel name
	bool text;    //!< Treat received data as text
} mqtt_topic_config;

/*!
 * Configuration and supporting data for mapping MQTT data to internal message format.
 *
 * Messages matching the topics subscribed to in mqtt_queue_map.tc are wrapped as msg_t instances and queued to
 * mqtt_queue_map.q by the callback function.
 * @sa mqtt_enqueue_messages
 */
typedef struct {
	msgqueue q;                //!< Internal message queue
	uint8_t sourceNum;         //!< Source number
	int numtopics;             //!< Number of topics registered
	mqtt_topic_config tc[120]; //!< Individual topic configuration
	bool dumpall;              //!< Dump any message, not just matches in .tc
} mqtt_queue_map;

//! Initialise mqtt_queue_map to sensible defaults
bool mqtt_init_queue_map(mqtt_queue_map *qm);

//! Release resources used by mqtt_queue_map instance
void mqtt_destroy_queue_map(mqtt_queue_map *qm);
//! @}
#endif
