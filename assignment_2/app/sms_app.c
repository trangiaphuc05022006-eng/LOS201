/*
 * sms_app.c - Sensor Monitoring System, main program
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-18
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <mqueue.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include "sms_common.h"
#include "sms_sensor.h"
#include "circular_buffer.h"
#include "sensor_thread.h"
#include "processor_thread.h"
#include "alert_thread.h"
#include "logger_thread.h"

#define BUFFER_CAPACITY  64
#define QUEUE_MAXMSG     10

static volatile sig_atomic_t g_stop = 0;

static void on_signal(int sig)
{
    (void)sig;
    g_stop = 1;
}

static void usage(const char *prog)
{
    /* Show supported options and their default values. */
    fprintf(stderr,
            "Usage: %s [-i interval_ms] [-t temp_threshold] [-h hum_threshold] "
            "[-d duration_sec] [-v]\n"
            "  -i interval in ms (default %d)\n"
            "  -t temperature threshold (default %.1f)\n"
            "  -h humidity threshold (default %.1f)\n"
            "  -d duration in seconds, 0 runs until a signal (default %d)\n"
            "  -v verbose sample output\n",
            prog, DEFAULT_INTERVAL_MS, DEFAULT_TEMP_THRESHOLD,
            DEFAULT_HUM_THRESHOLD, DEFAULT_DURATION_SEC);
}

/* Fill cfg from the defaults and the command line. 0 = ok, -1 = bad input */
static int parse_args(int argc, char **argv, Config *cfg)
{
    /* Apply defaults and validate every supplied option before storing it. */
    *cfg = (Config){0};
    cfg->interval_ms = DEFAULT_INTERVAL_MS;
    cfg->temp_threshold = DEFAULT_TEMP_THRESHOLD;
    cfg->hum_threshold = DEFAULT_HUM_THRESHOLD;
    cfg->duration_sec = DEFAULT_DURATION_SEC;
    cfg->verbose = 0;

    opterr = 0;
    int opt;
    char *end;
    long integer_value;
    double real_value;

    while ((opt = getopt(argc, argv, "i:t:h:d:v")) != -1) {
        switch (opt) {
        case 'i':
            errno = 0;
            integer_value = strtol(optarg, &end, 10);
            if (errno == ERANGE || end == optarg || *end != '\0' ||
                integer_value < SMS_MIN_INTERVAL_MS ||
                integer_value > SMS_MAX_INTERVAL_MS) {
                usage(argv[0]);
                return -1;
            }
            cfg->interval_ms = (int)integer_value;
            break;
        case 't':
        case 'h':
            errno = 0;
            real_value = strtod(optarg, &end);
            if (errno == ERANGE || end == optarg || *end != '\0' ||
                !isfinite(real_value) || real_value <= 0.0) {
                usage(argv[0]);
                return -1;
            }
            if (opt == 't') {
                cfg->temp_threshold = real_value;
            } else {
                cfg->hum_threshold = real_value;
            }
            break;
        case 'd':
            errno = 0;
            integer_value = strtol(optarg, &end, 10);
            if (errno == ERANGE || end == optarg || *end != '\0' ||
                integer_value < 0 || integer_value > INT_MAX) {
                usage(argv[0]);
                return -1;
            }
            cfg->duration_sec = (int)integer_value;
            break;
        case 'v':
            cfg->verbose = 1;
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }

    if (optind != argc) {
        usage(argv[0]);
        return -1;
    }
    return 0;
}

/* Create a fresh queue, then close our descriptor. 0 = ok, -1 = failure */
static int create_queue(const char *name, long msgsize)
{
    /* Remove any stale queue and create a fresh queue with this message size. */
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = QUEUE_MAXMSG,
        .mq_msgsize = msgsize,
        .mq_curmsgs = 0,
    };
    mqd_t mq;

    (void)mq_unlink(name);
    mq = mq_open(name, O_CREAT | O_RDWR, 0600, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return -1;
    }
    if (mq_close(mq) == -1) {
        perror("mq_close");
        (void)mq_unlink(name);
        return -1;
    }
    return 0;
}

static int queue_send_timed(mqd_t mq, const char *message, size_t length,
                            const char *operation)
{
    struct timespec deadline;

    if (clock_gettime(CLOCK_REALTIME, &deadline) == -1) {
        perror("clock_gettime shutdown deadline");
        return -1;
    }
    deadline.tv_sec += 2;
    if (mq_timedsend(mq, message, length, 0, &deadline) == -1) {
        perror(operation);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    Config cfg;
    int fd = -1;
    int status = 1;
    int err;
    int sensor_started = 0;
    int processor_started = 0;
    int alert_started = 0;
    int logger_started = 0;
    int can_destroy_cb = 1;
    int sigint_installed = 0;
    int sigterm_installed = 0;
    struct timespec start_time;
    int run_started = 0;
    CircularBuffer *cb = NULL;
    mqd_t alarm_reader = (mqd_t)-1;
    mqd_t alarm_writer = (mqd_t)-1;
    mqd_t alarm_control = (mqd_t)-1;
    mqd_t log_reader = (mqd_t)-1;
    mqd_t log_writer = (mqd_t)-1;
    mqd_t log_control = (mqd_t)-1;
    pthread_t sensor_tid;
    pthread_t processor_tid;
    pthread_t alert_tid;
    pthread_t logger_tid;
    struct sigaction action = {0};
    struct sigaction old_sigint;
    struct sigaction old_sigterm;
    SensorCtx sensor_ctx = {0};
    ProcessorCtx processor_ctx = {0};
    AlertCtx alert_ctx = {0};

    /* Parse command-line configuration before initializing resources. */
    if (parse_args(argc, argv, &cfg) != 0) {
        return 1;
    }
    /* Leave SA_RESTART disabled so signals can interrupt blocking calls. */
    action.sa_handler = on_signal;
    action.sa_flags = 0;
    if (sigemptyset(&action.sa_mask) == -1 ||
        sigaction(SIGINT, &action, &old_sigint) == -1) {
        perror("sigaction SIGINT");
        goto cleanup;
    }
    sigint_installed = 1;
    if (sigaction(SIGTERM, &action, &old_sigterm) == -1) {
        perror("sigaction SIGTERM");
        goto cleanup;
    }
    sigterm_installed = 1;

    /* Open the sensor device and request the configured sampling interval. */
    fd = open(SENSOR_DEVICE_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open sensor device");
        goto cleanup;
    }
    if (ioctl(fd, SMS_SET_INTERVAL, &cfg.interval_ms) == -1) {
        if (errno == ENOTTY) {
            fprintf(stderr,
                    "warning: sensor path does not support interval ioctl; "
                    "continuing with its current interval\n");
        } else {
            perror("ioctl SMS_SET_INTERVAL");
            goto cleanup;
        }
    }

    /* Create queues and the sample buffer before starting worker threads. */
    if (create_queue(ALARM_QUEUE_NAME, sizeof(AlarmMsg)) == -1 ||
        create_queue(LOG_QUEUE_NAME, LOG_LINE_SIZE) == -1) {
        goto cleanup;
    }
    cb = cb_create(BUFFER_CAPACITY);
    if (!cb) {
        fprintf(stderr, "cb_create failed\n");
        goto cleanup;
    }

    alarm_reader = mq_open(ALARM_QUEUE_NAME, O_RDONLY);
    alarm_writer = mq_open(ALARM_QUEUE_NAME, O_WRONLY | O_NONBLOCK);
    alarm_control = mq_open(ALARM_QUEUE_NAME, O_WRONLY);
    log_reader = mq_open(LOG_QUEUE_NAME, O_RDONLY);
    log_writer = mq_open(LOG_QUEUE_NAME, O_WRONLY | O_NONBLOCK);
    log_control = mq_open(LOG_QUEUE_NAME, O_WRONLY);
    if (alarm_reader == (mqd_t)-1 || alarm_writer == (mqd_t)-1 ||
        alarm_control == (mqd_t)-1 || log_reader == (mqd_t)-1 ||
        log_writer == (mqd_t)-1 || log_control == (mqd_t)-1) {
        perror("mq_open worker queues");
        goto cleanup;
    }

    sensor_ctx = (SensorCtx){
        .fd = fd,
        .buffer = cb,
        .log_mq = log_writer,
        .cfg = &cfg,
        .stop = &g_stop,
    };
    processor_ctx = (ProcessorCtx){
        .buffer = cb,
        .alarm_mq = alarm_writer,
        .log_mq = log_writer,
        .cfg = &cfg,
    };
    alert_ctx = (AlertCtx){
        .alarm_mq = alarm_reader,
        .log_mq = log_writer,
    };

    err = pthread_create(&logger_tid, NULL, logger_thread,
                         (void *)(intptr_t)log_reader);
    if (err != 0) {
        fprintf(stderr, "pthread_create logger: %s\n", strerror(err));
        goto cleanup;
    }
    logger_started = 1;
    err = pthread_create(&alert_tid, NULL, alert_thread, &alert_ctx);
    if (err != 0) {
        fprintf(stderr, "pthread_create alert: %s\n", strerror(err));
        goto cleanup;
    }
    alert_started = 1;
    err = pthread_create(&processor_tid, NULL, processor_thread, &processor_ctx);
    if (err != 0) {
        fprintf(stderr, "pthread_create processor: %s\n", strerror(err));
        goto cleanup;
    }
    processor_started = 1;
    err = pthread_create(&sensor_tid, NULL, sensor_thread, &sensor_ctx);
    if (err != 0) {
        fprintf(stderr, "pthread_create sensor: %s\n", strerror(err));
        goto cleanup;
    }
    sensor_started = 1;

    /* Run until the duration expires or a signal requests shutdown. */
    struct timespec deadline;
    if (clock_gettime(CLOCK_MONOTONIC, &deadline) == -1) {
        perror("clock_gettime");
        goto cleanup;
    }
    start_time = deadline;
    run_started = 1;
    if (cfg.duration_sec > 0) {
        deadline.tv_sec += cfg.duration_sec;
    }
    while (!g_stop) {
        struct timespec now;
        if (cfg.duration_sec > 0) {
            if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
                perror("clock_gettime");
                goto cleanup;
            }
            if (now.tv_sec > deadline.tv_sec ||
                (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec)) {
                break;
            }
        }

        struct timespec wake;
        if (clock_gettime(CLOCK_MONOTONIC, &wake) == -1) {
            perror("clock_gettime");
            goto cleanup;
        }
        wake.tv_nsec += 100000000L;
        if (wake.tv_nsec >= 1000000000L) {
            wake.tv_sec++;
            wake.tv_nsec -= 1000000000L;
        }
        err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, NULL);
        if (err != 0 && err != EINTR) {
            errno = err;
            perror("clock_nanosleep main");
            goto cleanup;
        }
    }
    status = 0;

    /* Stop the producer before shutting down the buffer; the processor then drains it. */
cleanup:
    g_stop = 1;
    if (sensor_started) {
        (void)pthread_kill(sensor_tid, SIGTERM);
        err = pthread_join(sensor_tid, NULL);
        if (err != 0) {
            fprintf(stderr, "pthread_join sensor: %s\n", strerror(err));
            status = 1;
            can_destroy_cb = 0;
        }
    }
    if (cb) {
        cb_shutdown(cb);
    }
    if (processor_started) {
        err = pthread_join(processor_tid, NULL);
        if (err != 0) {
            fprintf(stderr, "pthread_join processor: %s\n", strerror(err));
            status = 1;
            can_destroy_cb = 0;
        }
    }
    if (alert_started) {
        AlarmMsg quit_msg = {.type = ALARM_QUIT};
        if (queue_send_timed(alarm_control, (const char *)&quit_msg,
                             sizeof(quit_msg), "mq_timedsend alarm quit") == -1) {
            status = 1;
        }
        err = pthread_join(alert_tid, NULL);
        if (err != 0) {
            fprintf(stderr, "pthread_join alert: %s\n", strerror(err));
            status = 1;
        }
    }
    if (logger_started) {
        if (queue_send_timed(log_control, LOG_QUIT_TEXT, strlen(LOG_QUIT_TEXT),
                             "mq_timedsend logger quit") == -1) {
            status = 1;
        }
        err = pthread_join(logger_tid, NULL);
        if (err != 0) {
            fprintf(stderr, "pthread_join logger: %s\n", strerror(err));
            status = 1;
        }
    }

    if (sensor_started || processor_started || alert_started || logger_started) {
        struct timespec end_time;
        double uptime = 0.0;
        unsigned long overflow_count = 0;

        if (cb) {
            overflow_count = cb_overflow_count(cb);
        }

        if (run_started && clock_gettime(CLOCK_MONOTONIC, &end_time) == -1) {
            perror("clock_gettime end");
            status = 1;
        } else if (run_started) {
            uptime = (double)(end_time.tv_sec - start_time.tv_sec) +
                     (double)(end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        }

        unsigned long total_alerts = alert_ctx.count_temp_high +
                                     alert_ctx.count_humid_high +
                                     alert_ctx.count_temp_spike;
        fprintf(stderr,
                "uptime=%.3f sec samples_read=%lu read_errors=%lu "
                "samples_processed=%lu buffer_overflows=%lu alarms_dropped=%lu "
                "alerts=%lu temp_high=%lu humid_high=%lu temp_spike=%lu\n",
                uptime, sensor_ctx.samples_read, sensor_ctx.read_errors,
                processor_ctx.samples_processed, overflow_count,
                processor_ctx.alarms_dropped, total_alerts,
                alert_ctx.count_temp_high, alert_ctx.count_humid_high,
                alert_ctx.count_temp_spike);
    }

    if (can_destroy_cb) {
        cb_destroy(cb);
    }
    if (fd != -1) {
        close(fd);
    }
    if (alarm_reader != (mqd_t)-1) mq_close(alarm_reader);
    if (alarm_writer != (mqd_t)-1) mq_close(alarm_writer);
    if (alarm_control != (mqd_t)-1) mq_close(alarm_control);
    if (log_reader != (mqd_t)-1) mq_close(log_reader);
    if (log_writer != (mqd_t)-1) mq_close(log_writer);
    if (log_control != (mqd_t)-1) mq_close(log_control);
    (void)mq_unlink(ALARM_QUEUE_NAME);
    (void)mq_unlink(LOG_QUEUE_NAME);
    if (sigterm_installed) sigaction(SIGTERM, &old_sigterm, NULL);
    if (sigint_installed) sigaction(SIGINT, &old_sigint, NULL);
    return status;
}