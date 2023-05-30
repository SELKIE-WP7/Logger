/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SELKIELoggerMQTT_Connection
#define SELKIELoggerMQTT_Connection

#include <stdbool.h>

#include "MQTTTypes.h"
#include "SELKIELoggerBase.h"

/*!
 * @file MQTTConnection.h MQTT connection handling
 * @ingroup SELKIELoggerMQTT
 */

//! Open and configure a connection to an MQTT server
mqtt_conn *mqtt_openConnection(const char *host, const int port, mqtt_queue_map *qm);

//! Close MQTT server connection
void mqtt_closeConnection(mqtt_conn *conn);

//! Subscribe to all topics configured in a mqtt_queue_map
bool mqtt_subscribe_batch(mqtt_conn *conn, mqtt_queue_map *qm);

//! MQTT callback: Accept incoming messages and process
void mqtt_enqueue_messages(mqtt_conn *conn, void *userdat_qm, const struct mosquitto_message *inmsg);

//! Send MQTT keepalive commands required by Victron systems
bool mqtt_victron_keepalive(mqtt_conn *conn, mqtt_queue_map *qm, char *sysid);
#endif
