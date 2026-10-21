#ifndef PROCESSOR_THREAD_H
#define PROCESSOR_THREAD_H
#include "circular_buffer.h"
#include <mqueue.h>
typedef struct {
    CircularBuffer *cb;
    mqd_t alarm_mq;
    mqd_t log_mq;
    float temp_threshold;
    float hum_threshold;
} ProcessorThreadArgs;
void *processor_thread_func(void *arg);
#endif
