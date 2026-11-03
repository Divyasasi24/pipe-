#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

void signal_setup_parent(void);      /* SIGINT -> flag, SIGPIPE ignored */
void signal_setup_child(void);       /* child ignores SIGINT            */
int  signal_interrupted(void);

#endif