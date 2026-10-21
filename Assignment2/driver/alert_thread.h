#ifndef ALERT_THREAD_H
#define ALERT_THREAD_H

#include <mqueue.h>

// Cấu trúc truyền tham số cho Alert Thread
typedef struct {
    mqd_t alarm_mq;
    mqd_t log_mq;
} AlertThreadArgs;

void *alert_thread_func(void *arg);

#endif
