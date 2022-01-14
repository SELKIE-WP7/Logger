#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <version.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMQTT.h"

bool shutdown = false;

void signalShutdown(int signnum __attribute__((unused))) {
	shutdown = true;
}

int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

        char *usage =  "Usage: %1$s -h host -p port topic [topic ...]\n"
                "\t-h\tIncrease verbosity\n"
		"\t-p\tDecrease verbosity\n"
                "\nVersion: " GIT_VERSION_STRING "\n";

	char *host = NULL;
	int port = 0;

        opterr = 0; // Handle errors ourselves
        int go = 0;
	bool doUsage = false;
        while ((go = getopt(argc, argv, "h:p:")) != -1) {
                switch(go) {
                        case 'h':
				if (host) {
					log_error(&state, "Only a single hostname is supported");
					doUsage = true;
				}
				host = strdup(optarg);
                                break;
			case 'p':
				errno = 0;
				port = strtol(optarg, NULL, 0);
				if (port <= 0 || errno) {
					log_error(&state, "Invalid port number (%s)", optarg);
					doUsage = true;
				}
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
	}

	if (host == NULL) {
		host = strdup("localhost");
	}

	if (port == 0) {
		port = 1883;
	}

	mqtt_queue_map qm = {0};
	mqtt_init_queue_map(&qm, remaining);
	for (int t=0; t < remaining; t++) {
		sa_create_entry(&qm.topics, t, strlen(argv[optind+t]), argv[optind+t]);
		qm.msgnums[t] = 4 + t;
		qm.msgtext[t] = true;
	}

	log_info(&state, 1, "Connecting to %s:%d...", host, port);
	mqtt_conn *mc = mqtt_openConnection(host, port, &qm);
	if (!mqtt_subscribe_batch(mc, &(qm.topics))) {
		log_error(&state, "Unable to subscribe to topics");
		return EXIT_FAILURE;
	}

	sigset_t *hMask = calloc(1, sizeof(sigset_t));

	// If any new signal handlers are added, they also need to be added to this list
	sigemptyset(hMask);
	sigaddset(hMask, SIGINT);
	sigaddset(hMask, SIGQUIT);
	sigdelset(hMask, SIGINT);
	sigdelset(hMask, SIGQUIT);

	const struct sigaction saShutdown = {.sa_handler = signalShutdown, .sa_mask = *hMask, .sa_flags = SA_RESTART};
	sigaction(SIGINT, &saShutdown, NULL);
	sigaction(SIGQUIT, &saShutdown, NULL);
	sigaction(SIGRTMIN + 1, &saShutdown, NULL);

	while (!shutdown) {
		msg_t *in = NULL;
		in = queue_pop(&qm.q);
		if (in == NULL) {
			continue;
		}
		log_info(&state, 1, "[0x%02x] %s - %s", in->type, qm.topics.strings[in->type-4].data, in->data.string.data);
		msg_destroy(in);
		free(in);
	}
	log_info(&state, 1, "Closing connections");
	mqtt_closeConnection(mc);
	mqtt_destroy_queue_map(&qm);
	free(hMask);
	free(host);

}
