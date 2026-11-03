#include <stdio.h>
#include "statistics.h"

void stats_init(stats_t *s)
{
    *s = (stats_t){0};
    s->start = time(NULL);
}

void stats_record_sent(stats_t *s, const ticket_t *t)
{
    s->tickets_sent++;
    s->prio_count[t->priority]++;
    s->bytes_sent += (long)sizeof(message_t);
}

void stats_record_status(stats_t *s, const ticket_t *t)
{
    s->status_updates++;
    s->bytes_received += (long)sizeof(message_t);
    if (t->status == ST_RESOLVED) s->tickets_resolved++;
}

void stats_print(const stats_t *s)
{
    printf("\n========== Chat Statistics ==========\n");
    printf(" Tickets sent        : %d\n", s->tickets_sent);
    printf("   High priority     : %d\n", s->prio_count[PRIO_HIGH]);
    printf("   Medium priority   : %d\n", s->prio_count[PRIO_MEDIUM]);
    printf("   Low priority      : %d\n", s->prio_count[PRIO_LOW]);
    printf(" Status updates rcvd : %d\n", s->status_updates);
    printf(" Tickets resolved    : %d\n", s->tickets_resolved);
    printf(" Pending             : %d\n", s->tickets_sent - s->tickets_resolved);
    printf(" Bytes sent / rcvd   : %ld / %ld\n", s->bytes_sent, s->bytes_received);
    printf(" Session duration    : %ld sec\n", (long)(time(NULL) - s->start));
    printf("=====================================\n");
}