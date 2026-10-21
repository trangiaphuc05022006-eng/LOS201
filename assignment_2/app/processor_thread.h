#ifndef PROCESSOR_THREAD_H
#define PROCESSOR_THREAD_H
#include <mqueue.h>
#include "circular_buffer.h"
#include "sms_common.h"

typedef struct {
    CircularBuffer *buffer;           /* read samples from here */
    mqd_t           alarm_mq;         /* write end, non-blocking */
    mqd_t           log_mq;           /* write end, non-blocking */
    const Config   *cfg;              /* thresholds and verbose flag */
    unsigned long   samples_processed;
    unsigned long   alarms_dropped;   /* alarm queue was full */
} ProcessorCtx;

/* arg must be a ProcessorCtx *; main reads the counters after joining */
void *processor_thread(void *arg);

#endif