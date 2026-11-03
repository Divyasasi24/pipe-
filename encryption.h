#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stddef.h>

/* Symmetric XOR cipher (bonus feature). Applying it twice restores the data. */
void encryption_apply(char *buf, size_t len);

#endif