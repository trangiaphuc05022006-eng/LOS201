#include "alert_thread.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
extern volatile int keep_running;
void *alert_thread_func(void *arg) {
    AlertThreadArgs *args = (AlertThreadArgs *)arg;
    char buffer[256]; unsigned int prio;
    while (keep_running) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;
        ssize_t bytes = mq_timedreceive(args->alarm_mq, buffer, sizeof(buffer), &prio, &timeout);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            fprintf(stderr, "%s\n", buffer);
            mq_send(args->log_mq, buffer, strlen(buffer), 0);
        }
    }
    return NULL;
}
