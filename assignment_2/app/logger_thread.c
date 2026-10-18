#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <mqueue.h>
#include <sys/stat.h>
#include "sms_common.h"
#include "logger_thread.h"

#define MAX_LOG_BYTES   (1024 * 1024)
#define FLUSH_EVERY_SEC 5

/* Rename sms.log to sms.log.1 and open a fresh file */
static FILE *rotate_log(FILE *f)
{
    /* TODO 1: fclose(f), rename(LOG_FILE_PATH, LOG_FILE_PATH ".1"),
     *         fopen(LOG_FILE_PATH, "a") and return it (NULL on failure) */
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
    char  line[LOG_LINE_SIZE + 1];
    FILE *f = fopen(LOG_FILE_PATH, "a");
    time_t last_flush = time(NULL);

    if (!f) { perror("logger: fopen"); return NULL; }

    for (;;) {
        struct timespec ts;
        ssize_t n;

        /* TODO 2: wait for a message, but never longer than FLUSH_EVERY_SEC.
         *   build an absolute timeout: clock_gettime(CLOCK_REALTIME, &ts);
         *   ts.tv_sec += FLUSH_EVERY_SEC;
         *   n = mq_timedreceive(mq, line, sizeof(line), NULL, &ts);
         *   - n < 0 and errno == ETIMEDOUT: no message, just flush (TODO 5)
         *   - n < 0 and errno == EINTR: continue
         *   - any other error: break */
        if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
            perror("logger: clock_gettime");
            break;
        }
        ts.tv_sec += FLUSH_EVERY_SEC;
        n = mq_timedreceive(mq, line, LOG_LINE_SIZE, NULL, &ts);
        if (n < 0) {
            if (errno == ETIMEDOUT) {
                /* No message arrived; continue to the periodic flush below. */
            } else if (errno == EINTR) {
                continue;
            } else {
                perror("logger: mq_timedreceive");
                break;
            }
        } else {
            line[n] = '\0';

            /* TODO 3: if the message equals LOG_QUIT_TEXT: break */
            if (strcmp(line, LOG_QUIT_TEXT) == 0) {
                break;
            }

            /* TODO 4: write "<YYYY-MM-DD HH:MM:SS> <line>\n" to f.
             *   (time(), localtime_r(), strftime(), fprintf).
             *   After writing, if ftell(f) > MAX_LOG_BYTES: f = rotate_log(f) */
            time_t now = time(NULL);
            struct tm local_time;
            char timestamp[20];

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

        /* TODO 5: if FLUSH_EVERY_SEC seconds passed since last_flush:
         *         fflush(f) and update last_flush */
        time_t now = time(NULL);
        if (now - last_flush >= FLUSH_EVERY_SEC) {
            if (fflush(f) != 0) {
                perror("logger: fflush");
                break;
            }
            last_flush = now;
        }
    }

    /* TODO 6: fflush and fclose f */
    if (fflush(f) != 0) {
        perror("logger: final fflush");
    }
    if (fclose(f) != 0) {
        perror("logger: fclose");
    }
    return NULL;
}