/*
 * sensor_thread.h - Sensor thread context and entry point
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-21
 */
#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H
#include <mqueue.h>
#include <signal.h>
#include <stddef.h>
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
    long                  *jitter_us;     /* preallocated array for wake-up jitter */
    size_t                 jitter_cap;    /* capacity of jitter_us */
    size_t                 jitter_count;  /* number of recorded jitter samples */
} SensorCtx;

/* arg must be a SensorCtx *; main reads the counters after joining */
void *sensor_thread(void *arg);

#endif