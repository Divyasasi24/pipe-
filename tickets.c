#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ticket.h"

void ticket_init(ticket_t *t, int id, const char *text)
{
    memset(t, 0, sizeof(*t));
    t->id = id;
    t->status = ST_RECEIVED;
    t->created = time(NULL);
    strncpy(t->text, text, TICKET_TEXT_MAX - 1);
    t->priority = ticket_detect_priority(t->text);
}

/* Case-insensitive substring search (word must be lower-case). */
static int contains_ci(const char *text, const char *word)
{
    size_t n = strlen(word);
    for (; *text; text++) {
        size_t i = 0;
        while (i < n && text[i] && tolower((unsigned char)text[i]) == word[i]) i++;
        if (i == n) return 1;
    }
    return 0;
}

priority_t ticket_detect_priority(const char *text)
{
    static const char *high[] = { "urgent", "down", "outage", "crash", "security", "refund", NULL };
    static const char *med[]  = { "error", "slow", "bug", "login", "payment", "broken", NULL };
    for (int i = 0; high[i]; i++) if (contains_ci(text, high[i])) return PRIO_HIGH;
    for (int i = 0; med[i];  i++) if (contains_ci(text, med[i]))  return PRIO_MEDIUM;
    return PRIO_LOW;
}

const char *ticket_status_str(status_t s)
{
    switch (s) {
    case ST_RECEIVED:    return "RECEIVED";
    case ST_IN_PROGRESS: return "IN_PROGRESS";
    case ST_RESOLVED:    return "RESOLVED";
    }
    return "UNKNOWN";
}

const char *ticket_priority_str(priority_t p)
{
    switch (p) {
    case PRIO_LOW:    return "LOW";
    case PRIO_MEDIUM: return "MEDIUM";
    case PRIO_HIGH:   return "HIGH";
    }
    return "UNKNOWN";
}

void ticket_make_note(const ticket_t *t, status_t s, char *out, size_t n)
{
    const char *team = (t->priority == PRIO_HIGH)   ? "Tier-2 escalation team"
                     : (t->priority == PRIO_MEDIUM) ? "Tier-1 support agent"
                     :                                "Self-service queue";
    switch (s) {
    case ST_RECEIVED:
        snprintf(out, n, "Ticket logged with %s priority", ticket_priority_str(t->priority));
        break;
    case ST_IN_PROGRESS:
        snprintf(out, n, "Assigned to %s", team);
        break;
    case ST_RESOLVED:
        snprintf(out, n, "Issue resolved by %s", team);
        break;
    }
}