/*
 * alert_thread.c - Alert thread: reports alarms sent by the Processor
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-18
 *
 * Blocks on the alarm queue, prints each alarm to stderr, counts them per
 * type and forwards the same text to the Logger. Stops on ALARM_QUIT.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <mqueue.h>
#include "sms_common.h"
#include "alert_thread.h"

/* Return the printable name of an alarm type */
static const char *alarm_name(AlarmType t)
{
    /*switch on t, return "TEMP_HIGH", "HUMID_HIGH" or "TEMP_SPIKE" */
    switch (t) {
    case ALARM_TEMP_HIGH:
        return "TEMP_HIGH";
    case ALARM_HUMID_HIGH:
        return "HUMID_HIGH";
    case ALARM_TEMP_SPIKE:
        return "TEMP_SPIKE";
    default:
        return "UNKNOWN";
    }
}

void *alert_thread(void *arg)
{
    AlertCtx *ctx = arg;
    AlarmMsg  msg;
    char      text[LOG_LINE_SIZE];

    for (;;) {
        ssize_t n;

        /* n = mq_receive(ctx->alarm_mq, (char *)&msg, sizeof(msg), NULL)
         *   n < 0: EINTR -> continue; otherwise perror and break.
         *   Careful: the buffer passed must be at least the queue's msgsize
         *   (main will create the queue with msgsize = sizeof(AlarmMsg)) */
        n = mq_receive(ctx->alarm_mq, (char *)&msg, sizeof(msg), NULL);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("alert: mq_receive");
            break;
        }
        if ((size_t)n != sizeof(msg)) {
            fprintf(stderr, "alert: received malformed alarm message\n");
            continue;
        }

        /* if msg.type == ALARM_QUIT: break */
        if (msg.type == ALARM_QUIT) {
            break;
        }

        /* count it: increment the counter for msg.type */
        switch (msg.type) {
        case ALARM_TEMP_HIGH:
            ctx->count_temp_high++;
            break;
        case ALARM_HUMID_HIGH:
            ctx->count_humid_high++;
            break;
        case ALARM_TEMP_SPIKE:
            ctx->count_temp_spike++;
            break;
        default:
            fprintf(stderr, "alert: received unknown alarm type %d\n", msg.type);
            continue;
        }

        /* wait for the next alarm; the buffer must be at least the queue's message size */
        /* format once: the same text goes to stderr and to the log queue */
        if (snprintf(text, sizeof(text),
                     "[ALERT][%" PRIu64 "] TYPE: %s VALUE: %.1f THRESHOLD: %.1f",
                     msg.timestamp_ms, alarm_name(msg.type), msg.value,
                     msg.threshold) < 0) {
            fprintf(stderr, "alert: failed to format alarm message\n");
            continue;
        }
        fprintf(stderr, "%s\n", text);

        if (mq_send(ctx->log_mq, text, strlen(text), 0) != 0 && errno != EAGAIN) {
            perror("alert: mq_send");
        }
    }
    return NULL;
}