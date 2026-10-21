#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H
#include <mqueue.h>
#include <signal.h>
#include "circular_buffer.h"
#include "sms_common.h"

typedef struct {
    int                    fd;            /* sensor device, already open */
    CircularBuffer        *buffer;
    mqd_t                  log_mq;        /* write end, non-blocking */
    const Config          *cfg;
    volatile sig_atomic_t *stop;          /* set by main / signal handler */
    unsigned long          samples_read;
    unsigned long          read_errors;
} SensorCtx;

/* arg must be a SensorCtx *; main reads the counters after joining */
void *sensor_thread(void *arg);

#endif