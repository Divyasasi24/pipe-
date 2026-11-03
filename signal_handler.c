#include <signal.h>
#include <string.h>
#include "signal_handler.h"

static volatile sig_atomic_t got_sigint = 0;

static void on_sigint(int sig)
{
    (void)sig;
    got_sigint = 1;
}

void signal_setup_parent(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                 /* no SA_RESTART: select() returns EINTR */
    sigaction(SIGINT, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);        /* report write errors through errno */
}

void signal_setup_child(void)
{
    signal(SIGINT, SIG_IGN);         /* parent coordinates the shutdown */
    signal(SIGPIPE, SIG_IGN);
}

int signal_interrupted(void) { return got_sigint; }