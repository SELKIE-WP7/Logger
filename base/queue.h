#ifndef SELKIEQueue
#define SELKIEQueue

#include <stdbool.h>

#include "messages.h"

typedef struct queueitem queueitem;

typedef struct {
	queueitem *head;
	queueitem *tail;
	bool valid;
} msgqueue;

struct queueitem {
	msg_t *item;
	queueitem *next;
};

bool queue_init(msgqueue *queue);
void queue_destroy(msgqueue *queue);

bool queue_push(msgqueue *queue, msg_t *item);
bool queue_push_qi(msgqueue *queue, queueitem *item);
msg_t * queue_pop(msgqueue *queue);

int queue_count(const msgqueue *queue);

#endif
