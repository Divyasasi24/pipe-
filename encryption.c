#include "encryption.h"

static const char KEY[] = "PIPEchat#Key";

void encryption_apply(char *buf, size_t len)
{
    size_t k = sizeof(KEY) - 1;
    for (size_t i = 0; i < len; i++)
        buf[i] ^= KEY[i % k];
}