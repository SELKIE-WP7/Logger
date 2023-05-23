#ifndef SELKIELoggerBase_Queue
#define SELKIELoggerBase_Queue

#include <pthread.h>
#include <stdbool.h>

#include "messages.h"

/*!
 * @file queue.h Queue definition and handling functions
 * @ingroup SELKIELoggerBase
 */

/*!
 * @addtogroup queue Message queues
 * @ingroup SELKIELoggerBase
 * @{
 */

//! Each queue item is a message and pointer to the next queue entry, if any.
typedef struct queueitem queueitem;

/*!
 * @brief Represent a simple FIFO message queue
 *
 * Represents a queue of items starting at msgqueue.head, with the last item at or near msgqueue.tail.
 *
 * The queue is protected by the mutex at msgqueue.lock, and will only have
 * items added and removed while msgqueue.valid remains true.
 */
typedef struct msgqueue {
	queueitem *head;      //!< Points to first message, or NULL if empty
	queueitem *tail;      //!< brief Tail entry hint
	pthread_mutex_t lock; //!< Queue lock
	bool valid;           //!< Queue status
} msgqueue;

/*!
 * If *next is NULL, it will be assumed that this item is the end of the queue.
 *
 * Messages pointed to by queueitem.item "belong" to the queue until popped,
 * when they are then the responsibility of queue_pop()'s caller. Messages
 * pointed to by queueitem.item **must** be dynamically allocated.
 *
 * @sa msg_t
 */
struct queueitem {
	msg_t *item;     //!< Queued message
	queueitem *next; //!< Pointer to next item, or NULL for queue tail
};

//! Ensure queue structure is set to known good values and marked valid
bool queue_init(msgqueue *queue);

//! Invalidate queue and destroy all contents
void queue_destroy(msgqueue *queue);

//! Add a message to the tail of the queue
bool queue_push(msgqueue *queue, msg_t *item);

//! Add a queue item to the tail of the queue
bool queue_push_qi(msgqueue *queue, queueitem *item);

//! Remove topmost item from the queue and return it, if queue is not empty
msg_t *queue_pop(msgqueue *queue);

//! Iterate over queue and return current number of items
int queue_count(const msgqueue *queue);
//! @}
#endif
