#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "logger.h"
#include "permissions.h"

static int  log_fd = -1;
static char log_role[16] = "";

int logger_init(const char *path, const char *role)
{
    mode_t existing;
    /* If the log already exists, its permissions are preserved;
     * a brand-new log is created as rw-r----- (0640). */
    int exists = (perm_get_mode(path, &existing) == 0);
    log_fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0640);
    if (log_fd < 0) return -1;
    if (exists) perm_set_mode(path, existing);
    strncpy(log_role, role, sizeof(log_role) - 1);
    return 0;
}

void logger_log(const char *fmt, ...)
{
    if (log_fd < 0) return;

    char msg[512], line[640], ts[32];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);

    int len = snprintf(line, sizeof(line), "[%s] [%-9s] %s\n", ts, log_role, msg);
    if (len > 0) {
        if (len >= (int)sizeof(line)) len = (int)sizeof(line) - 1;
        if (write(log_fd, line, (size_t)len) < 0) { /* ignore */ }
    }
}

void logger_close(void)
{
    if (log_fd >= 0) close(log_fd);
    log_fd = -1;
}