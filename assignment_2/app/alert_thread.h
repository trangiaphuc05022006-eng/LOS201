#ifndef ALERT_THREAD_H
#define ALERT_THREAD_H
#include <mqueue.h>

typedef struct {
    mqd_t         alarm_mq;       /* read alarms from here */
    mqd_t         log_mq;         /* send log lines here */
    unsigned long count_temp_high;
    unsigned long count_humid_high;
    unsigned long count_temp_spike;
} AlertCtx;

/* arg must be an AlertCtx *; main reads the counters after joining */
void *alert_thread(void *arg);

#endif