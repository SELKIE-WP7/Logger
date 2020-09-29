#include <stdlib.h>

#include "queue.h"
#include "messages.h"

/*!
 * Will not re-initialise a queue if it is still valid or has a head or tail
 * value set. Other threads might be able to add entries to the queue between
 * the queue being marked valid and this function returning to the caller.
 */
bool queue_init(msgqueue *queue) {
	// Do not reinitialise valid or partially valid queue
	if (queue->valid || queue->head || queue->tail) {
		return false;
	}
	queue->head = NULL;
	queue->tail = NULL;
	// Make atomic
	queue->valid = true;
	return queue->valid;
}

/*!
 * The queue is immediately marked as invalid, and this is not undone if an error occurs.
 * Each item is removed from the queue and destroyed.
 */
void queue_destroy(msgqueue *queue) {
	// Make atomic
	queue->valid = false;
	if (queue->head == NULL) {
		// Queue is empty
		// Tail should already be null, but set it just in case
		queue->tail = NULL;
		return;
	}
	queueitem *qi = queue->head;
	while (qi->next) {
		// Use message destroy to handle underlying storage
		msg_destroy(qi->item);
		free(qi->item);
		qi->item = NULL;
		qi = qi->next;
	}
	queue->head = NULL;
	queue->tail = NULL;
}

/*!
 * Messages are wrapped into a queue item structure, and passed immediately to
 * queue_push_qi()
 */
bool queue_push(msgqueue *queue, msg_t *msg) {
	queueitem *qi = calloc(1, sizeof(queueitem));
	qi->item = msg;
	return queue_push_qi(queue, qi);
}

/*!
 * Will not append to an invalid queue.
 *
 * For a valid but empty queue, only a single attempt is made to make our item the head.
 *
 * For a valid but not empty queue, up to 100 attempts are made to append our item to the end of the list.
 * The append operation (or insertion of an item as the queue head) are atomic,
 * using the __sync_bool_compare_and_swap() function.
 *
 * If the queue is heavily contended, the tail pointer might not track the
 * final entry in the queue, but will be close to it.
 */
bool queue_push_qi(msgqueue *queue, queueitem *item) {
	if (!queue->valid) {
		return false;
	}

	queueitem *qi = queue->tail;

	if (queue->head == NULL) {
		// Empty queue!
		bool swapped = false;
		swapped = __sync_bool_compare_and_swap(&(queue->head), NULL, item);
		if (swapped) {
			queue->tail = queue->head;
			return true;
		}

		if (queue->tail) {
			qi = queue->tail;
			// No return, will fall out of enclosing if and continue with rest of function
		} else {
			// Odd scenario. Could attempt to enqueue again, but for now will just fail
			return false;
		}
	}

	if (qi->next) {
		do {
			qi = qi->next;
		} while (qi->next);
	}

	int attempts = 0;
	bool swapped = false;
	while (!swapped && attempts < 100) {
		if (qi->next) {
			do {
				qi = qi->next;
			} while (qi->next);
		}
		swapped = __sync_bool_compare_and_swap(&(qi->next), NULL, item);
		if (swapped) {
			// The seek and atomic compare and swap above ensure we only append to the real tail.
			// If there's no contention, we are still the tail of the queue and this works as planned.
			// If not, then we're still *near* the end of the queue, which is sufficient for our
			// purposes as we only ever treat the tail pointer as a hint.
			queue->tail = item;
		}
		attempts++;
	}
	return swapped;
}

/*!
 * Atomically remove the first entry in the queue and return it.
 * The underlying queue item is freed, but our caller must destroy and free the
 * message itself.
 */
msg_t * queue_pop(msgqueue *queue) {
	queueitem *head = queue->head;
	if (queue->head == NULL || !queue->valid) {
		// Empty or invalid queue
		return NULL;
	}

	if (__sync_bool_compare_and_swap(&(queue->head), head, head->next)) {
		msg_t * item = head->item;
		free(head);
		return item;
	}
	// The plan is only to have a single consumer, so this shouldn't ever fail.
	// If it does, something odd is going on and best to leave things alone rather than retry!
	return NULL;
}

int queue_count(const msgqueue *queue) {
	if (!queue->valid) {
		return -1;
	}

	if (queue->head == NULL) {
		return 0;
	}

	int count = 1;
	queueitem *item = queue->head;
	while (item->next) {
		count++;
		item = item->next;
	}
	return count;
}
