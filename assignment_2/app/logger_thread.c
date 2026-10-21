/*
 * logger_thread.c - Logger thread: writes log lines to /tmp/sms.log
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-18
 *
 * Receives text lines from the log queue, prefixes a timestamp and writes
 * them to the log file. The file is rotated above 1 MB and flushed every
 * few seconds instead of after every line, to avoid needless disk writes.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <mqueue.h>
#include "sms_common.h"
#include "logger_thread.h"

#define MAX_LOG_BYTES   (1024 * 1024)
#define FLUSH_EVERY_SEC 5

/* Move sms.log to sms.log.1 and start a new file.
 * Returns the new FILE, or NULL on failure (the old file is closed). */
static FILE *rotate_log(FILE *f)
{
    fclose(f);
    if (rename(LOG_FILE_PATH, LOG_FILE_PATH ".1") != 0) {
        perror("logger: rename");
        return NULL;
    }
    return fopen(LOG_FILE_PATH, "a");
}

void *logger_thread(void *arg)
{
    mqd_t mq = (mqd_t)(intptr_t)arg;
    char  line[LOG_LINE_SIZE + 1];          /* +1 for the terminating '\0' */
    FILE *f = fopen(LOG_FILE_PATH, "a");
    time_t last_flush = time(NULL);

    if (!f) {
        perror("logger: fopen");
        return NULL;
    }

    for (;;) {
        struct timespec ts;
        ssize_t n;

        /* Wait for a message, but wake up after FLUSH_EVERY_SEC even if none
         * arrives, so the periodic flush still happens. */
        if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
            perror("logger: clock_gettime");
            break;
        }
        ts.tv_sec += FLUSH_EVERY_SEC;
        n = mq_timedreceive(mq, line, LOG_LINE_SIZE, NULL, &ts);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno != ETIMEDOUT) {
                perror("logger: mq_timedreceive");
                break;
            }
            /* ETIMEDOUT: no message, fall through to the flush check */
        } else {
            time_t now = time(NULL);
            struct tm local_time;
            char timestamp[20];

            line[n] = '\0';
            if (strcmp(line, LOG_QUIT_TEXT) == 0) {
                break;                      /* main asked us to stop */
            }

            if (localtime_r(&now, &local_time) == NULL ||
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S",
                         &local_time) == 0) {
                fprintf(stderr, "logger: failed to format timestamp\n");
                break;
            }
            if (fprintf(f, "%s %s\n", timestamp, line) < 0) {
                perror("logger: fprintf");
                break;
            }
            if (ftell(f) > MAX_LOG_BYTES) {
                f = rotate_log(f);
                if (!f) {
                    break;
                }
            }
        }

        if (time(NULL) - last_flush >= FLUSH_EVERY_SEC) {
            if (fflush(f) != 0) {
                perror("logger: fflush");
                break;
            }
            last_flush = time(NULL);
        }
    }

    /* f is NULL if a log rotation failed; do not touch it in that case */
    if (f) {
        if (fflush(f) != 0) {
            perror("logger: final fflush");
        }
        if (fclose(f) != 0) {
            perror("logger: fclose");
        }
    }
    return NULL;
}