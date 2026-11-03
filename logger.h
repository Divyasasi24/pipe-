#ifndef LOGGER_H
#define LOGGER_H

#define LOG_DIR  "Log"
#define LOG_PATH "Log/Chat.log"

int  logger_init(const char *path, const char *role);
void logger_log(const char *fmt, ...);
void logger_close(void);

#endif