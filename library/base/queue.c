#include <stdio.h>
#include <stdlib.h>

#include "messages.h"
#include "queue.h"

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
	if (queue->valid || queue->head || queue->tail) { return false; }
	pthread_mutexattr_t ma = {0};
	pthread_mutexattr_init(&ma);
	pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&(queue->lock), &ma);
	pthread_mutexattr_destroy(&ma);

	if (pthread_mutex_lock(&(queue->lock))) {
		perror("queue_init");
		return false;
	}
	queue->head = NULL;
	queue->tail = NULL;
	queue->valid = true;
	pthread_mutex_unlock(&(queue->lock));
	return queue->valid;
}

/*!
 * The queue is immediately marked as invalid, and this is not undone if an error occurs.
 *
 * Any remaining items are removed from the queue and destroyed.
 *
 * @param[in] queue Pointer to queue structure to be destroyed
 */
void queue_destroy(msgqueue *queue) {
	queue->valid = false;
	if (pthread_mutex_lock(&(queue->lock))) {
		perror("queue_destroy");
		// Not returning, as we should still invalidate the queue
		// Just means that we're already in an odd case
	}
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
	pthread_mutex_unlock(&(queue->lock));
	pthread_mutex_destroy(&(queue->lock));
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
	if (!queue->valid) { return false; }

	queueitem *qi = NULL;

	if (pthread_mutex_lock(&(queue->lock))) {
		perror("queue_push_qi");
		return false;
	}

	// If head is NULL, the queue is empty, so point head and tail at our
	// queueitem, unlock the queue and return.
	if (queue->head == NULL) {
		queue->head = item;
		queue->tail = item;
		pthread_mutex_unlock(&(queue->lock));
		return true;
	}

	// If head wasn't empty, find the tail
	qi = queue->tail;
	if (qi) {
		// There is a slim chance that tail wasn't up to date, so follow queued
		// items all the way down
		if (qi->next) {
			// qi->next is valid, so can re-assign it to qi
			// The new qi->next is either valid (so loop and
			// reassign), or NULL and we fall out of the loop
			do {
				qi = qi->next;
			} while (qi->next);
		}
		// Once we've run out of valid qi->next pointers, we make our new item the
		// end of the queue
		qi->next = item;
	}
	// Update the tail pointer so the next push should jump direct to the end
	// Note that item is a pointer, so assigning it to qi->next and queue->tail is
	// valid
	queue->tail = item;
	pthread_mutex_unlock(&(queue->lock));
	return true;
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
msg_t *queue_pop(msgqueue *queue) {
	int e = pthread_mutex_lock(&(queue->lock));
	if (e != 0) {
		perror("queue_pop");
		return NULL;
	}
	queueitem *head = queue->head;
	if (head == NULL || !queue->valid) {
		// Empty or invalid queue
		pthread_mutex_unlock(&(queue->lock));
		return NULL;
	}

	msg_t *item = head->item;
	queue->head = head->next;

	head->next = NULL;
	head->item = NULL;
	if (queue->tail == head) { queue->tail = NULL; }
	pthread_mutex_unlock(&(queue->lock));
	// At this point we should have the only valid pointer to head
	// ** This is only valid while a single thread is consuming items from the queue
	// ** As with the explanation in queue_pop(), the cast keeps the compiler happy
	free((struct queueitem *)head);
	return item;
}

/*!
 * @param[in] queue Pointer to queue
 * @return Number of items in queue, or -1 on error
 */
int queue_count(const msgqueue *queue) {
	if (!queue->valid) { return -1; }

	if (queue->head == NULL) { return 0; }

	int count = 1;
	queueitem *item = queue->head;
	while (item->next) {
		count++;
		item = item->next;
	}
	return count;
}
