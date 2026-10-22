/*
 * sms_common.h - definitions shared by all threads of sms_app
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-18
 */
#ifndef SMS_COMMON_H
#define SMS_COMMON_H

#include <stdint.h>

/* Defaults (the command line can override them) */
#define DEFAULT_INTERVAL_MS      500
#define DEFAULT_TEMP_THRESHOLD   40.0
#define DEFAULT_HUM_THRESHOLD    80.0
#define DEFAULT_DURATION_SEC     60

/* Paths and names: nothing is hardcoded in the .c files */
#ifndef SENSOR_DEVICE_PATH
#define SENSOR_DEVICE_PATH   "/dev/sms_sensor"
#endif
#define LOG_FILE_PATH        "/tmp/sms.log"
#define ALARM_QUEUE_NAME     "/sms_alarm"
#define LOG_QUEUE_NAME       "/sms_log"

/* The log queue carries fixed-size text lines */
#define LOG_LINE_SIZE        128
/* Sent by main to tell the Logger to stop; no real log line starts like this */
#define LOG_QUIT_TEXT        "@@QUIT@@"

/* Settings, filled in from the command line */
typedef struct {
    int    interval_ms;
    double temp_threshold;
    double hum_threshold;
    int    duration_sec;    /* 0 = run until Ctrl+C */
    int    verbose;
} Config;

/* Kinds of alarm. ALARM_QUIT is not a real alarm: it is the message main
 * sends to wake up and stop the Alert thread. */
typedef enum {
    ALARM_TEMP_HIGH,
    ALARM_HUMID_HIGH,
    ALARM_TEMP_SPIKE,
    ALARM_QUIT
} AlarmType;

/* What travels through the alarm queue */
typedef struct {
    AlarmType type;
    uint64_t  timestamp_ms;
    double    value;        /* the measurement that caused the alarm */
    double    threshold;    /* filled in by the Processor, printed by Alert */
} AlarmMsg;

#endif