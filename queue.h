#ifndef QUEUE_H
#define QUEUE_H

#include "ticket.h"

/* FIFO queue implemented as a singly linked list. */
typedef struct queue_node {
    ticket_t           ticket;
    struct queue_node *next;
} queue_node_t;

typedef struct {
    queue_node_t *head;
    queue_node_t *tail;
    int           size;
} queue_t;

void queue_init(queue_t *q);
int  queue_enqueue(queue_t *q, const ticket_t *t);   /* 0 ok, -1 error */
int  queue_dequeue(queue_t *q, ticket_t *out);       /* 0 ok, -1 empty */
int  queue_is_empty(const queue_t *q);
int  queue_size(const queue_t *q);
void queue_destroy(queue_t *q);

#endif