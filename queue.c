#include <stdlib.h>
#include "queue.h"

void queue_init(queue_t *q)
{
    q->head = q->tail = NULL;
    q->size = 0;
}

int queue_enqueue(queue_t *q, const ticket_t *t)
{
    queue_node_t *n = malloc(sizeof(*n));
    if (!n) return -1;
    n->ticket = *t;
    n->next = NULL;
    if (q->tail) q->tail->next = n; else q->head = n;
    q->tail = n;
    q->size++;
    return 0;
}

int queue_dequeue(queue_t *q, ticket_t *out)
{
    if (!q->head) return -1;
    queue_node_t *n = q->head;
    *out = n->ticket;
    q->head = n->next;
    if (!q->head) q->tail = NULL;
    free(n);
    q->size--;
    return 0;
}

int queue_is_empty(const queue_t *q) { return q->head == NULL; }
int queue_size(const queue_t *q)     { return q->size; }

void queue_destroy(queue_t *q)
{
    ticket_t tmp;
    while (queue_dequeue(q, &tmp) == 0) { }
}