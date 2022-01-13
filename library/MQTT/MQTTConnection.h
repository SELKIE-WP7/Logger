#ifndef SELKIELoggerMQTT_Messages
#define SELKIELoggerMQTT_Messages

#include <stdbool.h>
#include <mosquitto.h>

#include "SELKIELoggerBase.h"
#include "MQTTTypes.h"

typedef struct mosquitto mqtt_conn;


mqtt_conn *mqtt_openConnection(const char *host, const int port, mqtt_queue_map *qm);
void mqtt_closeConnection(mqtt_conn *conn);
bool mqtt_subscribe_batch(mqtt_conn *conn, strarray *topics);
void mqtt_enqueue_messages(mqtt_conn *conn, void *userdat_qm, const struct mosquitto_message *inmsg);
#endif
