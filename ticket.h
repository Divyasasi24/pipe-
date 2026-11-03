#ifndef TICKET_H
#define TICKET_H

#include <stddef.h>
#include <time.h>

#define TICKET_TEXT_MAX 200

typedef enum { MSG_TICKET = 1, MSG_STATUS = 2, MSG_QUIT = 3 } msg_type_t;
typedef enum { PRIO_LOW = 0, PRIO_MEDIUM, PRIO_HIGH } priority_t;
typedef enum { ST_RECEIVED = 0, ST_IN_PROGRESS, ST_RESOLVED } status_t;

/* A support ticket. The text field carries the customer issue (request)
 * or the processor's note (status update). */
typedef struct {
    int        id;
    priority_t priority;
    status_t   status;
    time_t     created;
    char       text[TICKET_TEXT_MAX];
} ticket_t;

/* Fixed-size message exchanged over the pipes (smaller than PIPE_BUF,
 * so every write() is atomic). */
typedef struct {
    int      type;
    ticket_t ticket;
} message_t;

void        ticket_init(ticket_t *t, int id, const char *text);
priority_t  ticket_detect_priority(const char *text);
const char *ticket_status_str(status_t s);
const char *ticket_priority_str(priority_t p);
void        ticket_make_note(const ticket_t *t, status_t s, char *out, size_t n);

#endif