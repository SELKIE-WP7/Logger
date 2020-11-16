#ifndef SELKIELoggerBase_Queue
#define SELKIELoggerBase_Queue

#include <stdbool.h>
#include <pthread.h>

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

typedef struct queueitem queueitem;

//! Represent a simple FIFO message queue
struct msgqueue {
	queueitem *head; //!< Points to first message, or NULL if empty

	//! @brief Tail entry hint
	queueitem *tail;
	//! Queue lock
	pthread_mutex_t lock;
	//! Set in queue_init(), entries will only be added and removed while this remains true
	bool valid;
};

typedef struct msgqueue msgqueue;

//! Each queue item is a message and pointer to the next queue entry, if any.
//! @sa msg_t
struct queueitem {
	msg_t *item;
	queueitem *next;
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
msg_t * queue_pop(msgqueue *queue);

//! Iterate over queue and return current number of items
int queue_count(const msgqueue *queue);
//! @}
#endif
