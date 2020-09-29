#include <stdio.h>
#include <stdlib.h>

#include "queue.h"
#include "messages.h"

/*! @file QueueTest.c
 *
 * @brief Basic Queue testing
 *
 * Creates a queue and several messages, adding and removing them in different
 * combinations.  The queue length is verified at various points, and messages
 * removed from the queue are checked to ensure ordering is maintained.
 *
 * Note that this is a single threaded test.
 *
 * Ideally this test should also be checked with valgrind to ensure memory is
 * not being leaked through basic queue operations.
 *
 * @addtogroup testing Test programs for use with CTest
 */
int main(int argc, char *argv[]) {

	msgqueue QT = {0};

	if (!queue_init(&QT)) {
		fprintf(stderr, "Failed to initialise queue\n");
		perror("queue_init");
		return EXIT_FAILURE;
	}

	int count = queue_count(&QT);
	fprintf(stdout, "Empty queue initialised: %d messages in queue\n", count);
	if (count != 0) {
		fprintf(stderr, "Incorrect item count for empty queue (expected 0, got %d)\n", count);
		queue_destroy(&QT);
		return EXIT_FAILURE;
	}

	fprintf(stdout, "Push initial message...\n");
	if (!queue_push(&QT, msg_new_float(1,5, 13.0))) {
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return EXIT_FAILURE;
	}

	count = queue_count(&QT);
	if (count != 1) {
		fprintf(stderr, "Incorrect item count (expected 1, got %d)\n", count);
		return EXIT_FAILURE;
	}

	msg_t *out = queue_pop(&QT);
	if (out == NULL || out->data.value != 13.0) {
		fprintf(stderr, "Failed to retrieve initial test message\n.");
		perror("queue_pop");
		return EXIT_FAILURE;
	}
	msg_destroy(out);
	out = NULL;

	count = queue_count(&QT);
	if (count != 0) {
		fprintf(stderr, "Incorrect item count for empty queue (expected 0, got %d)\n", count);
		queue_destroy(&QT);
		return EXIT_FAILURE;
	}

	fprintf(stdout, "Push sequence of messages...\n");
	if (!queue_push(&QT, msg_new_float(1,5, 1.0))) {
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return EXIT_FAILURE;
	}
	if (!queue_push(&QT, msg_new_float(1,5, 2.0))) {
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return EXIT_FAILURE;
	}
	if (!queue_push(&QT, msg_new_float(1,5, 3.0))) {
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return EXIT_FAILURE;
	}
	if (!queue_push(&QT, msg_new_float(1,5, 4.0))) {
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return EXIT_FAILURE;
	}
	if (!queue_push(&QT, msg_new_float(1,5, 5.0))) {
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return EXIT_FAILURE;
	}

	count = queue_count(&QT);
	if (count != 5) {
		fprintf(stderr, "Incorrect item count (expected 5, got %d)\n", count);
		return EXIT_FAILURE;
	}

	while (count--) {
		msg_t *item = queue_pop(&QT);
		if (item->data.value != (5 - count)) {
			fprintf(stderr, "Messages out of order? (Expected value %d, got %.0f)\n", (5 - count), item->data.value);
			queue_destroy(&QT);
			return EXIT_FAILURE;
		}
		msg_destroy(item);
	}

	count = queue_count(&QT);
	if (count != 0) {
		fprintf(stderr, "Incorrect item count (expected 0, got %d)\n", count);
		return EXIT_FAILURE;
	}

	if (!queue_push(&QT, msg_new_string(1,5, 20, "Test Message - 1234"))) {
		fprintf(stderr, "Unable to push string message to queue");
		return EXIT_FAILURE;
	}

	out = queue_pop(&QT);
	fprintf(stdout, "Logged message: %s\n", out->data.string.data);
	msg_destroy(out);

	queue_destroy(&QT);

	count = queue_count(&QT);
	if (count != -1) {
		fprintf(stderr, "Incorrect item count for destroyed queue (expected -1 [Error], got %d)\n", count);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


