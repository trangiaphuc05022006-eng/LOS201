#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H

#include <mqueue.h>

// Cấu trúc truyền tham số cho Logger Thread
typedef struct {
    mqd_t log_mq;
} LoggerThreadArgs;

void *logger_thread_func(void *arg);

#endif
