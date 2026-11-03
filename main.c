/*
 * PIPEchat - Customer Support Ticket Processing Communication System
 *
 *   PARENT = Ticket Reception service  (reads tickets typed by the user)
 *   CHILD  = Ticket Processing service (processes them, sends status updates)
 *
 * Two pipes give full-duplex communication:
 *   parent --(ticket requests)--> child
 *   parent <--(status updates)--- child
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "encryption.h"
#include "ipc.h"
#include "logger.h"
#include "permissions.h"
#include "queue.h"
#include "signal_handler.h"
#include "statistics.h"
#include "ticket.h"

#define BACKUP_PATH "Log/Chat.log.bak"

static void msleep(long ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static void print_help(void)
{
    printf("Type a customer issue and press Enter to raise a ticket.\n"
           "Commands:  /stats  show statistics\n"
           "           /help   show this help\n"
           "           /quit   finish pending tickets and exit  (Ctrl+C = exit now)\n");
}

/* ---------------------------------------------------------------- */
/* CHILD: ticket processing service                                  */
/* ---------------------------------------------------------------- */
static int send_status(ipc_channel_t *ch, const ticket_t *t, status_t st)
{
    message_t m;
    memset(&m, 0, sizeof(m));
    m.type = MSG_STATUS;
    m.ticket = *t;
    m.ticket.status = st;
    ticket_make_note(t, st, m.ticket.text, TICKET_TEXT_MAX);
    logger_log("Status of ticket #%d -> %s (%s)", t->id, ticket_status_str(st), m.ticket.text);
    encryption_apply(m.ticket.text, TICKET_TEXT_MAX);
    return ipc_send(ch->c2p[1], &m);
}

static int run_child(ipc_channel_t *ch)
{
    queue_t work;
    message_t msg;
    ticket_t t;
    int r;

    signal_setup_child();
    logger_init(LOG_PATH, "PROCESSOR");
    queue_init(&work);
    logger_log("Ticket processor started (pid %d)", (int)getpid());

    while ((r = ipc_recv(ch->p2c[0], &msg)) == 1) {
        if (msg.type == MSG_QUIT) {
            logger_log("Quit request received");
            break;
        }
        if (msg.type != MSG_TICKET) continue;

        encryption_apply(msg.ticket.text, TICKET_TEXT_MAX);   /* decrypt */
        logger_log("Ticket #%d received [%s]: %s", msg.ticket.id,
                   ticket_priority_str(msg.ticket.priority), msg.ticket.text);
        queue_enqueue(&work, &msg.ticket);

        while (queue_dequeue(&work, &t) == 0) {
            if (send_status(ch, &t, ST_RECEIVED) < 0) goto out;
            msleep(300);
            if (send_status(ch, &t, ST_IN_PROGRESS) < 0) goto out;
            msleep(600);
            if (send_status(ch, &t, ST_RESOLVED) < 0) goto out;
        }
    }
out:
    if (r == 0) logger_log("Parent closed the pipe");
    queue_destroy(&work);
    logger_log("Ticket processor stopped");
    logger_close();
    close(ch->p2c[0]);
    close(ch->c2p[1]);
    return 0;
}

/* ---------------------------------------------------------------- */
/* PARENT: ticket reception service                                  */
/* ---------------------------------------------------------------- */
static int flush_outbox(ipc_channel_t *ch, queue_t *outbox)
{
    ticket_t t;
    message_t m;
    while (queue_dequeue(outbox, &t) == 0) {
        memset(&m, 0, sizeof(m));
        m.type = MSG_TICKET;
        m.ticket = t;
        encryption_apply(m.ticket.text, TICKET_TEXT_MAX);      /* encrypt */
        if (ipc_send(ch->p2c[1], &m) < 0) {
            perror("send to processor");
            return -1;
        }
    }
    return 0;
}

static void handle_line(char *line, ipc_channel_t *ch, queue_t *outbox,
                        stats_t *st, int *next_id, int *quitting)
{
    while (*line == ' ' || *line == '\t') line++;
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' ')) line[--len] = '\0';
    if (len == 0) return;

    if (strcmp(line, "/quit") == 0)  { *quitting = 1; return; }
    if (strcmp(line, "/stats") == 0) { stats_print(st); return; }
    if (strcmp(line, "/help") == 0)  { print_help(); return; }

    ticket_t t;
    ticket_init(&t, (*next_id)++, line);
    stats_record_sent(st, &t);
    logger_log("Ticket #%d raised [%s]: %s", t.id, ticket_priority_str(t.priority), t.text);
    printf("[Reception] Ticket #%d created (priority %s)\n", t.id, ticket_priority_str(t.priority));
    queue_enqueue(outbox, &t);
    flush_outbox(ch, outbox);
}

static void handle_status(const message_t *in, stats_t *st)
{
    message_t m = *in;
    encryption_apply(m.ticket.text, TICKET_TEXT_MAX);          /* decrypt */
    stats_record_status(st, &m.ticket);
    logger_log("Update for ticket #%d: %s - %s", m.ticket.id,
               ticket_status_str(m.ticket.status), m.ticket.text);
    printf("[Processor] Ticket #%d %-11s - %s\n", m.ticket.id,
           ticket_status_str(m.ticket.status), m.ticket.text);
}

static int run_parent(ipc_channel_t *ch, pid_t child)
{
    queue_t outbox;
    stats_t stats;
    int next_id = 1, quitting = 0, stdin_open = 1, child_alive = 1;
    char buf[1024];

    signal_setup_parent();
    logger_init(LOG_PATH, "RECEPTION");
    queue_init(&outbox);
    stats_init(&stats);
    logger_log("Ticket reception started (pid %d), processor pid %d", (int)getpid(), (int)child);

    printf("=== PIPEchat : Customer Support Ticket System ===\n");
    print_help();
    printf("> ");
    fflush(stdout);

    while (!signal_interrupted() && child_alive) {
        int pending = stats.tickets_sent - stats.tickets_resolved;
        if ((quitting || !stdin_open) && pending <= 0) break;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(ch->c2p[0], &rfds);
        int maxfd = ch->c2p[0];
        if (stdin_open && !quitting) {
            FD_SET(STDIN_FILENO, &rfds);
            if (STDIN_FILENO > maxfd) maxfd = STDIN_FILENO;
        }

        int rc = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (rc < 0) {
            if (errno == EINTR) continue;       /* SIGINT is checked by the loop */
            perror("select");
            break;
        }

        if (FD_ISSET(ch->c2p[0], &rfds)) {
            message_t m;
            int r = ipc_recv(ch->c2p[0], &m);
            if (r == 1 && m.type == MSG_STATUS) {
                handle_status(&m, &stats);
                printf("> ");
                fflush(stdout);
            } else if (r <= 0) {
                child_alive = 0;
                logger_log("Processor closed the pipe");
            }
        }

        if (stdin_open && !quitting && FD_ISSET(STDIN_FILENO, &rfds)) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
            if (n <= 0) {
                stdin_open = 0;                 /* EOF: finish pending tickets */
                printf("\n[Reception] Input closed, waiting for pending tickets...\n");
            } else {
                buf[n] = '\0';
                char *save = NULL;
                for (char *ln = strtok_r(buf, "\n", &save); ln; ln = strtok_r(NULL, "\n", &save))
                    handle_line(ln, ch, &outbox, &stats, &next_id, &quitting);
                if (!quitting) { printf("> "); fflush(stdout); }
            }
        }
    }

    if (signal_interrupted()) {
        printf("\n[Reception] SIGINT caught - shutting down\n");
        logger_log("SIGINT received - shutting down");
    }

    /* Tell the processor to stop, then reap it. */
    message_t q;
    memset(&q, 0, sizeof(q));
    q.type = MSG_QUIT;
    ipc_send(ch->p2c[1], &q);
    close(ch->p2c[1]);
    close(ch->c2p[0]);
    waitpid(child, NULL, 0);

    stats_print(&stats);
    logger_log("Session ended: %d sent, %d resolved", stats.tickets_sent, stats.tickets_resolved);
    queue_destroy(&outbox);
    logger_close();
    return 0;
}

/* Back up the log and show that its permissions were preserved. */
static void backup_log(void)
{
    mode_t orig, copy;
    char s1[10], s2[10];
    if (perm_backup_file(LOG_PATH, BACKUP_PATH) == 0 &&
        perm_get_mode(LOG_PATH, &orig) == 0 &&
        perm_get_mode(BACKUP_PATH, &copy) == 0) {
        perm_to_string(orig, s1);
        perm_to_string(copy, s2);
        printf("Log backup: %s (%s) <- %s (%s) : permissions %s\n",
               BACKUP_PATH, s2, LOG_PATH, s1, orig == copy ? "preserved" : "MISMATCH");
    }
}

int main(void)
{
    ipc_channel_t ch;

    if (perm_ensure_dir(LOG_DIR, 0755) < 0) {
        perror("Log directory");
        return EXIT_FAILURE;
    }
    if (ipc_create(&ch) < 0) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (pid == 0) {                     /* child */
        ipc_setup_child(&ch);
        _exit(run_child(&ch));
    }

    ipc_setup_parent(&ch);              /* parent */
    int rc = run_parent(&ch, pid);
    backup_log();
    return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}