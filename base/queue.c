#include <stdlib.h>

#include "queue.h"

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
		free(qi->item);
		qi->item = NULL;
		qi = qi->next;
	}
	queue->head = NULL;
	queue->tail = NULL;
}

bool queue_push(msgqueue *queue, msg_t *msg) {
	queueitem *qi = calloc(1, sizeof(queueitem));
	qi->item = msg;
	return queue_push_qi(queue, qi);
}

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
