#include "logger_thread.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#define LOG_FILE "/tmp/sms.log"
#define LOG_FILE_OLD "/tmp/sms.log.1"
#define MAX_LOG_SIZE (1024 * 1024) // 1MB
extern volatile int keep_running;
void *logger_thread_func(void *arg) {
    LoggerThreadArgs *args = (LoggerThreadArgs *)arg;
    char buffer[256];
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return NULL;
    time_t last_flush = time(NULL);
    while (keep_running) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;
        ssize_t bytes = mq_timedreceive(args->log_mq, buffer, sizeof(buffer), NULL, &timeout);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            fprintf(fp, "%s", buffer);
        }
        time_t now = time(NULL);
        if (now - last_flush >= 5) {
            fflush(fp);
            last_flush = now;
            struct stat st;
            if (stat(LOG_FILE, &st) == 0 && st.st_size > MAX_LOG_SIZE) {
                fclose(fp);
                rename(LOG_FILE, LOG_FILE_OLD);
                fp = fopen(LOG_FILE, "a");
            }
        }
    }
    if (fp) fclose(fp);
    return NULL;
}
