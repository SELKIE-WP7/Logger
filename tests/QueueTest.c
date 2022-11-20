#include <stdio.h>
#include <stdlib.h>

#include "SELKIELoggerBase.h"

/*! @file QueueTest.c
 *
 * @brief Basic Queue testing
 *
 * @test Creates a queue and several messages, adding and removing them in different
 * combinations.  The queue length is verified at various points, and messages
 * removed from the queue are checked to ensure ordering is maintained.
 *
 * Note that this is a single threaded test.
 *
 * Ideally this test should also be checked with valgrind to ensure memory is
 * not being leaked through basic queue operations.
 *
 * @ingroup testing
 */

/*!
 * Initialise, fill, drain, and destroy queue instance.
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns 0 (Pass), -1 (Fail)
 */
int main(int argc, char *argv[]) {
	msgqueue QT = {0};

	if (!queue_init(&QT)) {
		// LCOV_EXCL_START
		fprintf(stderr, "Failed to initialise queue\n");
		perror("queue_init");
		return -1;
		// LCOV_EXCL_STOP
	}

	int count = queue_count(&QT);
	fprintf(stdout, "Empty queue initialised: %d messages in queue\n", count);
	if (count != 0) {
		// LCOV_EXCL_START
		fprintf(stderr, "Incorrect item count for empty queue (expected 0, got %d)\n",
		        count);
		queue_destroy(&QT);
		return -1;
		// LCOV_EXCL_STOP
	}

	fprintf(stdout, "Push initial message...\n");
	if (!queue_push(&QT, msg_new_float(1, 5, 13.0))) {
		// LCOV_EXCL_START
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return -1;
		// LCOV_EXCL_STOP
	}

	count = queue_count(&QT);
	if (count != 1) {
		// LCOV_EXCL_START
		fprintf(stderr, "Incorrect item count (expected 1, got %d)\n", count);
		return -1;
		// LCOV_EXCL_STOP
	}

	msg_t *out = queue_pop(&QT);
	if (out == NULL || out->data.value != 13.0) {
		// LCOV_EXCL_START
		fprintf(stderr, "Failed to retrieve initial test message\n.");
		perror("queue_pop");
		return -1;
		// LCOV_EXCL_STOP
	}
	msg_destroy(out);
	free(out);
	out = NULL;

	count = queue_count(&QT);
	if (count != 0) {
		// LCOV_EXCL_START
		fprintf(stderr, "Incorrect item count for empty queue (expected 0, got %d)\n",
		        count);
		queue_destroy(&QT);
		return -1;
		// LCOV_EXCL_STOP
	}

	fprintf(stdout, "Push sequence of messages...\n");
	if (!queue_push(&QT, msg_new_float(1, 5, 1.0))) {
		// LCOV_EXCL_START
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return -1;
		// LCOV_EXCL_STOP
	}
	if (!queue_push(&QT, msg_new_float(1, 5, 2.0))) {
		// LCOV_EXCL_START
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return -1;
		// LCOV_EXCL_STOP
	}
	if (!queue_push(&QT, msg_new_float(1, 5, 3.0))) {
		// LCOV_EXCL_START
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return -1;
		// LCOV_EXCL_STOP
	}
	if (!queue_push(&QT, msg_new_float(1, 5, 4.0))) {
		// LCOV_EXCL_START
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return -1;
		// LCOV_EXCL_STOP
	}
	if (!queue_push(&QT, msg_new_float(1, 5, 5.0))) {
		// LCOV_EXCL_START
		fprintf(stderr, "Unable to push item to queue\n");
		perror("queue_push");
		return -1;
		// LCOV_EXCL_STOP
	}

	count = queue_count(&QT);
	if (count != 5) {
		// LCOV_EXCL_START
		fprintf(stderr, "Incorrect item count (expected 5, got %d)\n", count);
		return -1;
		// LCOV_EXCL_STOP
	}

	while (count--) {
		msg_t *item = queue_pop(&QT);
		if (item->data.value != (5 - count)) {
			// LCOV_EXCL_START
			fprintf(stderr, "Messages out of order? (Expected value %d, got %.0f)\n",
			        (5 - count), item->data.value);
			queue_destroy(&QT);
			return -1;
			// LCOV_EXCL_STOP
		}
		msg_destroy(item);
		free(item);
	}

	count = queue_count(&QT);
	if (count != 0) {
		// LCOV_EXCL_START
		fprintf(stderr, "Incorrect item count (expected 0, got %d)\n", count);
		return -1;
		// LCOV_EXCL_STOP
	}

	if (!queue_push(&QT, msg_new_string(1, 5, 20, "Test Message - 1234"))) {
		// LCOV_EXCL_START
		fprintf(stderr, "Unable to push string message to queue");
		return -1;
		// LCOV_EXCL_STOP
	}

	out = queue_pop(&QT);
	fprintf(stdout, "Logged message: %s\n", out->data.string.data);
	msg_destroy(out);
	free(out);

	// Destroy the now-empty queue structure
	queue_destroy(&QT);

	// Reinitialise the queue
	if (!queue_init(&QT)) {
		// LCOV_EXCL_START
		fprintf(stderr, "Failed to initialise queue\n");
		perror("queue_init");
		return -1;
		// LCOV_EXCL_STOP
	}

	if (!queue_push(&QT, msg_new_string(1, 6, 20, "Test Message - 5678"))) {
		// LCOV_EXCL_START
		fprintf(stderr, "Unable to push string message to queue");
		return -1;
		// LCOV_EXCL_STOP
	}

	if (!queue_push(&QT, msg_new_string(1, 7, 20, "Test Message - ABCD"))) {
		// LCOV_EXCL_START
		fprintf(stderr, "Unable to push string message to queue");
		return -1;
		// LCOV_EXCL_STOP
	}
	// Now destroy a queue with multiple items still present
	queue_destroy(&QT);

	count = queue_count(&QT);
	if (count != -1) {
		// LCOV_EXCL_START
		fprintf(stderr,
		        "Incorrect item count for destroyed queue (expected -1 [Error], "
		        "got %d)\n",
		        count);
		return -1;
		// LCOV_EXCL_STOP
	}

	queue_init(&QT);
	if (queue_pop(&QT) != NULL) {
		// LCOV_EXCL_START
		fprintf(stderr, "Gained an item from a destroyed queue!\n");
		return -1;
		// LCOV_EXCL_STOP
	}
	queue_destroy(&QT);

	return 0;
}
