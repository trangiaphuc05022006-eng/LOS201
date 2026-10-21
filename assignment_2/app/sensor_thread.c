/*
 * sensor_thread.c - Sensor thread: periodic reading with RT scheduling & jitter tracking
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-21
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include "sms_common.h"
#include "circular_buffer.h"
#include "sensor_thread.h"

/* Send a short text line to the Logger; drop it if the queue is full */
static void log_text(SensorCtx *ctx, const char *text)
{
    if (mq_send(ctx->log_mq, text, strlen(text), 0) == -1) {
        if (errno != EAGAIN) {
            perror("sensor: mq_send");
        }
    }
}

/* Add ms milliseconds to t, keeping tv_nsec below 1,000,000,000 */
static void add_ms(struct timespec *t, int ms)
{
    t->tv_sec += ms / 1000;
    t->tv_nsec += (ms % 1000) * 1000000L;
    if (t->tv_nsec >= 1000000000L) {
        t->tv_sec++;
        t->tv_nsec -= 1000000000L;
    }
}

/* Configure SCHED_FIFO when requested; fall back to SCHED_OTHER on error */
static void apply_policy(SensorCtx *ctx)
{
    if (ctx->cfg->realtime) {
        struct sched_param sp = { .sched_priority = RT_PRIORITY };
        int err = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);

        if (err != 0) {
            fprintf(stderr,
                    "warning: pthread_setschedparam(SCHED_FIFO, %d) failed: %s "
                    "(continuing with SCHED_OTHER)\n",
                    RT_PRIORITY, strerror(err));
            log_text(ctx, "sensor: SCHED_OTHER");
        } else {
            log_text(ctx, "sensor: SCHED_FIFO prio 50");
        }
    } else {
        log_text(ctx, "sensor: SCHED_OTHER");
    }
}

/* Return (a - b) in microseconds; a is the actual wake time, b is expected */
static long diff_us(const struct timespec *a, const struct timespec *b)
{
    return (long)(a->tv_sec - b->tv_sec) * 1000000L +
           (long)(a->tv_nsec - b->tv_nsec) / 1000L;
}

void *sensor_thread(void *arg)
{
    SensorCtx *ctx = arg;
    char line[64];
    struct timespec next;

    apply_policy(ctx);

    if (clock_gettime(CLOCK_MONOTONIC, &next) != 0) {
        perror("sensor: clock_gettime");
        return NULL;
    }

    while (!*ctx->stop) {
        ssize_t n = read(ctx->fd, line, sizeof(line) - 1);
        int sleep_result;

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                int read_error = errno;

                ctx->read_errors++;
                if (read_error == EAGAIN || read_error == EWOULDBLOCK) {
                    log_text(ctx, "sensor: ERROR: read returned EAGAIN");
                } else {
                    errno = read_error;
                    perror("sensor: read");
                    log_text(ctx, "sensor: ERROR: read failed");
                }
                goto next_period;
            }
        } else if (n == 0) {
            ctx->read_errors++;
            log_text(ctx, "sensor: ERROR: EOF reached");
            goto next_period;
        } else {
            line[n] = '\0';
        }

        uint64_t ts;
        double t, h;
        int result = sscanf(line, "%" SCNu64 ",%lf,%lf", &ts, &t, &h);
        if (result != 3) {
            ctx->read_errors++;
            log_text(ctx, "sensor: ERROR: sscanf failed");
            goto next_period;
        }

        SensorData data = { .timestamp_ms = ts, .temperature = t, .humidity = h };
        int push_result = cb_push(ctx->buffer, &data);
        if (push_result == -1) {
            log_text(ctx, "sensor: WARN: buffer full, sample dropped");
        } else {
            ctx->samples_read++;
        }

next_period:
        add_ms(&next, ctx->cfg->interval_ms);
        do {
            sleep_result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                           &next, NULL);
        } while (sleep_result == EINTR && !*ctx->stop);
        if (sleep_result != 0 && !(sleep_result == EINTR && *ctx->stop)) {
            log_text(ctx, "sensor: ERROR: clock_nanosleep failed");
        } else if (sleep_result == 0 && ctx->jitter_us &&
                   ctx->jitter_count < ctx->jitter_cap) {
            struct timespec actual;
            if (clock_gettime(CLOCK_MONOTONIC, &actual) == 0) {
                ctx->jitter_us[ctx->jitter_count++] = diff_us(&actual, &next);
            }
        }
    }
    return NULL;
}