#ifndef IPC_H
#define IPC_H

#include "ticket.h"

/* Two unidirectional pipes = full-duplex channel.
 * p2c : parent -> child   (ticket requests)
 * c2p : child  -> parent  (status updates)   */
typedef struct {
    int p2c[2];
    int c2p[2];
} ipc_channel_t;

int  ipc_create(ipc_channel_t *ch);
void ipc_setup_parent(ipc_channel_t *ch);     /* close unused ends in parent */
void ipc_setup_child(ipc_channel_t *ch);      /* close unused ends in child  */
int  ipc_send(int fd, const message_t *msg);  /* 0 ok, -1 error */
int  ipc_recv(int fd, message_t *msg);        /* 1 ok, 0 EOF, -1 error */

#endif