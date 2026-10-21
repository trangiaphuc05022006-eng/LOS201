#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <mqueue.h>
#include "sms_common.h"
#include "circular_buffer.h"
#include "sensor_thread.h"

/* Send a short text line to the Logger; drop it if the queue is full */
static void log_text(SensorCtx *ctx, const char *text)
{
    /* mq_send(ctx->log_mq, text, strlen(text), 0);
     *         ignore EAGAIN, perror on other errors */
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

void *sensor_thread(void *arg)
{
    SensorCtx *ctx = arg;
    char line[64];
    struct timespec next;

    if (clock_gettime(CLOCK_MONOTONIC, &next) != 0) {
        perror("sensor: clock_gettime");
        return NULL;
    }

    while (!*ctx->stop) {
        ssize_t n = read(ctx->fd, line, sizeof(line) - 1);
        int sleep_result;

        /* n < 0: EINTR -> continue (re-check stop);
         *         other errors (EAGAIN included): read_errors++, log it.
         *         n == 0 (EOF): read_errors++, log it.
         *         n > 0: line[n] = '\0' and go on to parse */
        if (n < 0) { 
            if(errno == EINTR) {
                continue; // re-check stop
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

        /* sscanf(line, "%" SCNu64 ",%lf,%lf", &ts, &t, &h) must
         *         return 3; otherwise read_errors++ and log it */
        uint64_t ts;
        double t, h; 
        int result = sscanf(line, "%" SCNu64 ",%lf,%lf", &ts, &t, &h);
        if (result != 3) {
            ctx->read_errors++; 
            log_text(ctx, "sensor: ERROR: sscanf failed");
            goto next_period;
        }

        /* fill a SensorData and cb_push it.
         *         -1 -> log "WARN: buffer full, sample dropped";
         *         0  -> samples_read++ */
        SensorData data = { .timestamp_ms = ts, .temperature = t, .humidity = h };
        int push_result = cb_push(ctx->buffer, &data);
        if (push_result == -1) { 
            log_text(ctx, "sensor: WARN: buffer full, sample dropped");
        } else {
            ctx->samples_read++;
        }
 
        /* sleep until the next period: add_ms(&next, interval_ms),
         *         then clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL).
         *         It returns an error NUMBER (not -1): if it is EINTR and
         *         we are not stopping, call it again with the same `next`. */
next_period:
        add_ms(&next, ctx->cfg->interval_ms);
        do {
            sleep_result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                           &next, NULL);
        } while (sleep_result == EINTR && !*ctx->stop);
        if (sleep_result != 0 && !(sleep_result == EINTR && *ctx->stop)) {
            log_text(ctx, "sensor: ERROR: clock_nanosleep failed");
        }
    }
    return NULL;
}