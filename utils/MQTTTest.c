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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <version.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMQTT.h"

/*!
 * @file
 * @brief Test MQTT broker connection and subscriptions
 * @ingroup Executables
 */

//! Set true to start clean shutdown
bool shutdown = false;

void signalShutdown(int signnum);

/*!
 * Connects to MQTT broker, subscribing to topics given on the command line.
 *
 * Topics will be printed to stdout when received, optionally dumping messages
 * not matching subscription topics (i.e. messages matching wildcards)
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns -1 on error, otherwise 0
 */
int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	char *usage =
		"Usage: %1$s [-h host] [-p port] [-k sysid] topic [topic ...]\n"
		"\t-h\tMQTT Broker Host name\n"
		"\t-p\tMQTT Broker port\n"
		"\t-k sysid\tEnable Victron compatible keepalive messages/requests for given system ID\n"
		"\t-v\tDump all messages\n"
		"\nDefaults equivalent to %1$s -h localhost -p 1883\n"
		"\nVersion: " GIT_VERSION_STRING "\n";

	char *host = NULL;
	int port = 0;
	bool dumpAll = false;
	bool keepalive = false;
	char *sysid = NULL;

	opterr = 0; // Handle errors ourselves
	int go = 0;
	bool doUsage = false;
	while ((go = getopt(argc, argv, "h:k:p:v")) != -1) {
		switch (go) {
			case 'h':
				if (host) {
					log_error(&state, "Only a single hostname is supported");
					doUsage = true;
				} else {
					host = strdup(optarg);
				}
				break;
			case 'p':
				errno = 0;
				port = strtol(optarg, NULL, 0);
				if (port <= 0 || errno) {
					log_error(&state, "Invalid port number (%s)", optarg);
					doUsage = true;
				}
				break;
			case 'k':
				keepalive = true;
				if (sysid) {
					log_error(
						&state,
						"Cannot specify keepalive option multiple times");
					doUsage = true;
				} else {
					sysid = strdup(optarg);
				}
				break;
			case 'v':
				dumpAll = true;
				break;
			case '?':
				log_error(&state, "Unknown option `-%c'", optopt);
				doUsage = true;
		}
	}

	int remaining = argc - optind;
	if (remaining == 0) {
		log_error(&state, "No topics supplied");
		doUsage = true;
	}

	if (doUsage) {
		log_error(&state, "Invalid options provided");
		fprintf(stderr, usage, argv[0]);
		if (host) { free(host); }
		if (sysid) { free(sysid); }
		return -1;
	}

	if (host == NULL) { host = strdup("localhost"); }

	if (port == 0) { port = 1883; }

	mqtt_queue_map qm = {0};
	mqtt_init_queue_map(&qm);
	for (int t = 0; t < remaining; t++) {
		qm.numtopics++;
		qm.tc[t].type = 4 + t;
		qm.tc[t].topic = strdup(argv[optind + t]);
		qm.tc[t].name = strdup(argv[optind + t]);
		qm.tc[t].text = true;
	}

	qm.dumpall = dumpAll;

	log_info(&state, 1, "Connecting to %s:%d...", host, port);
	mqtt_conn *mc = mqtt_openConnection(host, port, &qm);
	if (!mqtt_subscribe_batch(mc, &qm)) {
		log_error(&state, "Unable to subscribe to topics");
		if (host) { free(host); }
		if (sysid) { free(sysid); }
		return EXIT_FAILURE;
	}

	sigset_t *hMask = calloc(1, sizeof(sigset_t));

	// If any new signal handlers are added, they also need to be added to this list
	sigemptyset(hMask);
	sigaddset(hMask, SIGINT);
	sigaddset(hMask, SIGQUIT);
	sigdelset(hMask, SIGINT);
	sigdelset(hMask, SIGQUIT);

	const struct sigaction saShutdown = {
		.sa_handler = signalShutdown, .sa_mask = *hMask, .sa_flags = SA_RESTART};
	sigaction(SIGINT, &saShutdown, NULL);
	sigaction(SIGQUIT, &saShutdown, NULL);
	sigaction(SIGRTMIN + 1, &saShutdown, NULL);

	log_info(&state, 1, "Starting message loop...");

	time_t lastKA = 0;
	uint16_t count = 0;
	while (!shutdown) {
		if (keepalive && (count % 100) == 0) {
			time_t now = time(NULL);
			if ((now - lastKA) >= 60) {
				lastKA = now;
				mqtt_victron_keepalive(mc, &qm, sysid);
			}
		}
		count++;

		msg_t *in = NULL;
		in = queue_pop(&qm.q);
		if (in == NULL) {
			usleep(5000);
			continue;
		}
		log_info(&state, 1, "[0x%02x] %s - %s", in->type,
		         in->type >= 4 ? qm.tc[in->type - 4].name : "RAW", in->data.string.data);
		msg_destroy(in);
		free(in);
	}
	log_info(&state, 1, "Closing connections");
	mqtt_closeConnection(mc);
	mqtt_destroy_queue_map(&qm);
	free(hMask);
	free(host);
	free(sysid);
	return 0;
}

/*!
 * Signal handler - sets shutdown flag
 * @internal
 * @param[in] signnum Signal number (ignored)
 */
void signalShutdown(int signnum __attribute__((unused))) {
	shutdown = true;
}
