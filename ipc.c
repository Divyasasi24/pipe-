#include <errno.h>
#include <unistd.h>
#include "ipc.h"

int ipc_create(ipc_channel_t *ch)
{
    if (pipe(ch->p2c) == -1) return -1;
    if (pipe(ch->c2p) == -1) {
        close(ch->p2c[0]);
        close(ch->p2c[1]);
        return -1;
    }
    return 0;
}

void ipc_setup_parent(ipc_channel_t *ch)
{
    close(ch->p2c[0]);   /* parent only writes to child  */
    close(ch->c2p[1]);   /* parent only reads from child */
}

void ipc_setup_child(ipc_channel_t *ch)
{
    close(ch->p2c[1]);   /* child only reads from parent */
    close(ch->c2p[0]);   /* child only writes to parent  */
}

int ipc_send(int fd, const message_t *msg)
{
    const char *p = (const char *)msg;
    size_t left = sizeof(*msg);
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += n;
        left -= (size_t)n;
    }
    return 0;
}

int ipc_recv(int fd, message_t *msg)
{
    char *p = (char *)msg;
    size_t got = 0;
    while (got < sizeof(*msg)) {
        ssize_t n = read(fd, p + got, sizeof(*msg) - got);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return 0;     /* other side closed the pipe */
        got += (size_t)n;
    }
    return 1;
}