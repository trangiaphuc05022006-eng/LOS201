#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H
#include <mqueue.h>

/* arg must be the mqd_t of the log queue, passed as (void *)(intptr_t)mq */
void *logger_thread(void *arg);

#endif