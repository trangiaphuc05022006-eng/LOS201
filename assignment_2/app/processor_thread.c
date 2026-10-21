/*
 * processor_thread.c - Processes sensor samples and emits alarms and logs
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-18
 *
 * Maintains rolling averages, checks temperature and humidity thresholds,
 * detects temperature spikes, and forwards alarms and sample logs to queues.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <mqueue.h>
#include "sms_common.h"
#include "circular_buffer.h"
#include "processor_thread.h"

#define WINDOW       10
#define SPIKE_DELTA  5.0

typedef struct {
    double temp[WINDOW], hum[WINDOW];
    int    next, count;
} Window;

static void window_add(Window *w, double t, double h)
{
    /* Replace the oldest entry as needed and cap the valid count at WINDOW. */
    w->temp[w->next] = t;
    w->hum[w->next] = h;
    w->next = (w->next + 1) % WINDOW;
    if (w->count < WINDOW) {
        w->count++;
    }
}

static double window_avg_temp(const Window *w)
{
    /* Before the window fills, only the first count entries are initialized. */
    double sum = 0.0;

    if (w->count == 0) {
        return 0.0;
    }
    for (int i = 0; i < w->count; i++) {
        sum += w->temp[i];
    }
    return sum / w->count;
}
/* Average humidity over the same rolling window. */
static double window_avg_hum(const Window *w)
{
    double sum = 0.0;

    if (w->count == 0) {
        return 0.0;
    }
    for (int i = 0; i < w->count; i++) {
        sum += w->hum[i];
    }
    return sum / w->count;
}

/* Send one alarm; if the queue is full drop it and count it */
static void send_alarm(ProcessorCtx *ctx, AlarmType type, uint64_t ts,
                       double value, double threshold)
{
    /* Drop on EAGAIN rather than blocking sample processing on a full queue. */
    AlarmMsg msg = {
        .type = type,
        .timestamp_ms = ts,
        .value = value,
        .threshold = threshold,
    };

    if (mq_send(ctx->alarm_mq, (const char *)&msg, sizeof(msg), 0) != 0) {
        if (errno == EAGAIN) {
            ctx->alarms_dropped++;
        } else {
            perror("processor: mq_send alarm");
        }
    }
}

void *processor_thread(void *arg)
{
    ProcessorCtx *ctx = arg;
    SensorData s;
    Window w = {0};
    double prev_temp = 0.0;
    int have_prev = 0;
    char text[LOG_LINE_SIZE];
    int text_len;

    for (;;) {
        /* Stop when shutdown has drained the circular buffer. */
        if (cb_pop(ctx->buffer, &s) != 0) {
            break;
        }

        /* Add this sample to the rolling window and account for its processing. */
        window_add(&w, s.temperature, s.humidity);
        ctx->samples_processed++;

        /* Check configured limits and compare this temperature with the last. */
        if (s.temperature > ctx->cfg->temp_threshold) {
            send_alarm(ctx, ALARM_TEMP_HIGH, s.timestamp_ms, s.temperature,
                       ctx->cfg->temp_threshold);
        }
        if (s.humidity > ctx->cfg->hum_threshold) {
            send_alarm(ctx, ALARM_HUMID_HIGH, s.timestamp_ms, s.humidity,
                       ctx->cfg->hum_threshold);
        }
        if (have_prev) {
            double change = s.temperature - prev_temp;

            if (change < 0.0) {
                change = -change;
            }
            if (change > SPIKE_DELTA) {
                send_alarm(ctx, ALARM_TEMP_SPIKE, s.timestamp_ms, change,
                           SPIKE_DELTA);
            }
        }

        /* Remember this temperature for the spike check on the next sample. */
        prev_temp = s.temperature;
        have_prev = 1;

        /* Format the sample and its rolling averages, then forward it to Logger. */
        text_len = snprintf(text, sizeof(text),
                    "SAMPLE ts=%" PRIu64 " T=%.1f H=%.1f avgT=%.1f avgH=%.1f",
                    s.timestamp_ms, s.temperature, s.humidity,
                    window_avg_temp(&w), window_avg_hum(&w));
        if (text_len < 0) {
            fprintf(stderr, "processor: failed to format sample log\n");
            continue;
        }
        if (mq_send(ctx->log_mq, text, strlen(text), 0) != 0 && errno != EAGAIN) {
            perror("processor: mq_send log");
        }

        /* Mirror the log line to stdout when verbose mode is enabled. */
        if (ctx->cfg->verbose) {
            printf("%s\n", text);
        }

    }
    return NULL;
}