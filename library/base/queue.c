#include <stdlib.h>

#include "queue.h"
#include "messages.h"

/*!
 * Will not re-initialise a queue if it is still valid or has a head or tail
 * value set. Other threads might be able to add entries to the queue between
 * the queue being marked valid and this function returning to the caller.
 *
 * @param[in] queue Pointer to queue structure to be initialised
 * @return True on success, false otherwise
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
 *
 * Any remaining items are removed from the queue and destroyed.
 *
 * @param[in] queue Pointer to queue structure to be destroyed
 * @return True on success, false otherwise
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
 *
 * @param[in] queue Pointer to queue
 * @param[in] msg Pointer to message
 * @return Return value of queue_push_qi()
 */
bool queue_push(msgqueue *queue, msg_t *msg) {
	queueitem *qi = calloc(1, sizeof(queueitem));
	qi->item = msg;
	if (queue_push_qi(queue, qi)) {
		return true;
	} else {
		// free() complains about the volatile flag on qi
		// If we end up in this branch, the item was never queued and
		// this is the only pointer left to qi, so we're safe to free
		// it here and the cast keeps the compiler happy.
		free((struct queueitem *)qi);
		return false;
	}
}

/*!
 * Will not append to an invalid queue.
 *
 * For a valid but empty queue, only a single attempt is made to make our item the head.
 *
 * For a valid but not empty queue, up to 100 attempts are made to append our
 * item to the end of the list.  The append operation (or insertion of an item
 * as the queue head) are atomic, using __sync_bool_compare_and_swap()
 *
 * If the queue is heavily contended, the tail pointer might not track the
 * final entry in the queue, but will be close to it.
 *
 * Once pushed to the queue, the queue owns the message embedded in `item` and
 * the caller should not destroy or free it. That will be handled in
 * queue_destroy() or the function responsible for consuming items out of the
 * queue.
 *
 * @param[in] queue Pointer to queue
 * @param[in] item  Pointer to a queue item
 * @return True if item successfully appended to queue, false otherwise
 */
bool queue_push_qi(msgqueue *queue, queueitem *item) {
	if (!queue->valid) {
		return false;
	}

	queueitem *qi = NULL;


	int attempts = 0;
	bool swapped = false;
	while (!swapped && attempts < 100) {
		if (queue->head == NULL) {
			// Empty queue!
			swapped = __sync_bool_compare_and_swap(&(queue->head), NULL, item);
			if (swapped) {
				queue->tail = queue->head;
				return true;
			}
		} else {
			if (queue->tail) {
				qi = queue->tail;
			} else if (queue->head) {
				qi = queue->head;
			}

			if (qi && qi->next) {
				do {
					qi = qi->next;
					if (qi && qi == qi->next) {
						// So there's a pathological case somewhere that ends up with the queue
						// item pointing to itself as the next item. Hopefully this is fixed by
						// explicitly nulling items in _pop, but we will also check explicitly here.
						qi = queue->tail;
					}
				} while (qi && qi->next);
			}
			if (!qi) {
				continue;
			}
			swapped = __sync_bool_compare_and_swap(&(qi->next), NULL, item);
			if (swapped) {
				// The seek and atomic compare and swap above ensure we only append to the real tail.
				// If there's no contention, we are still the tail of the queue and this works as planned.
				// If not, then we're still *near* the end of the queue, which is sufficient for our
				// purposes as we only ever treat the tail pointer as a hint.
				queue->tail = item;
				return true;
			}
		}
		attempts++;
	}
	return swapped;
}

/*!
 * Atomically remove the first entry in the queue and return it.
 * The underlying queue item is freed, but our caller must destroy and free the
 * message itself.
 *
 * The queue item is freed, but the caller is responsible for destroying and
 * freeing the message itself after use (i.e. the caller now owns the message,
 * not the queue or the sending function).
 *
 * @param[in] queue Pointer to queue
 * @return Pointer to previously queued message
 */
msg_t * queue_pop(msgqueue *queue) {
	queueitem *head = queue->head;
	if (head == NULL || !queue->valid) {
		// Empty or invalid queue
		return NULL;
	}

	if (__sync_bool_compare_and_swap(&(queue->head), head, head->next)) {
		msg_t * item = head->item;
		head->next = NULL;
		head->item = NULL;
		if (queue->tail == head) {
			queue->tail = NULL;
		}
		// At this point we should have the only valid pointer to head
		// ** This is only valid while a single thread is consuming items from the queue **
		// As with the explanation in queue_pop(), the cast keeps the compiler happy
		free((struct queueitem *)head);
		return item;
	}
	// The plan is only to have a single consumer, so this shouldn't ever fail.
	// If it does, something odd is going on and best to leave things alone rather than retry!
	return NULL;
}

/*!
 * @param[in] queue Pointer to queue
 * @return Number of items in queue, or -1 on error
 */
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
