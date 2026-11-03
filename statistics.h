#ifndef STATISTICS_H
#define STATISTICS_H

#include <time.h>
#include "ticket.h"

typedef struct {
    int    tickets_sent;
    int    status_updates;
    int    tickets_resolved;
    int    prio_count[3];
    long   bytes_sent;
    long   bytes_received;
    time_t start;
} stats_t;

void stats_init(stats_t *s);
void stats_record_sent(stats_t *s, const ticket_t *t);
void stats_record_status(stats_t *s, const ticket_t *t);
void stats_print(const stats_t *s);

#endif