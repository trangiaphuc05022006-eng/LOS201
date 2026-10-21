#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "sms_sensor.h"
#include "circular_buffer.h"
#include "sensor_thread.h"
#include "processor_thread.h"
#include "alert_thread.h"
#include "logger_thread.h"

#define ALARM_MQ_NAME "/alarm_queue"
#define LOG_MQ_NAME "/log_queue"

// Cờ toàn cục để dừng tất cả các thread khi bấm Ctrl+C
volatile int keep_running = 1;

void sig_handler(int sig) {
    printf("\n[Main] Nhận tín hiệu dừng (SIGINT), đang tắt hệ thống an toàn...\n");
    keep_running = 0;
}

int main(int argc, char *argv[]) {
    int interval_ms = 500;
    float temp_thresh = 40.0;
    float hum_thresh = 80.0;
    int duration_sec = 60;
    int verbose = 0;
    int opt;

    // Task 1: Parse tham số dòng lệnh
    while ((opt = getopt(argc, argv, "i:t:h:d:v")) != -1) {
        switch (opt) {
            case 'i': interval_ms = atoi(optarg); break;
            case 't': temp_thresh = atof(optarg); break;
            case 'h': hum_thresh = atof(optarg); break;
            case 'd': duration_sec = atoi(optarg); break;
            case 'v': verbose = 1; break;
            default: 
                fprintf(stderr, "Cách dùng: %s [-i ms] [-t temp] [-h hum] [-d sec] [-v]\n", argv[0]);
                return 1;
        }
    }

    signal(SIGINT, sig_handler);

    // Khởi tạo Circular Buffer
    CircularBuffer cb;
    if (cb_init(&cb, 64) != 0) {
        perror("Lỗi cấp phát bộ nhớ cho Buffer");
        return 1;
    }

    // Khởi tạo POSIX Message Queue
    struct mq_attr attr = { .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = 256, .mq_curmsgs = 0 };
    mq_unlink(ALARM_MQ_NAME);
    mq_unlink(LOG_MQ_NAME);
    mqd_t alarm_mq = mq_open(ALARM_MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    mqd_t log_mq = mq_open(LOG_MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);

    // Đóng gói tham số cho từng thread
    SensorThreadArgs s_args = { &cb, interval_ms, verbose };
    ProcessorThreadArgs p_args = { &cb, alarm_mq, log_mq, temp_thresh, hum_thresh };
    AlertThreadArgs a_args = { alarm_mq, log_mq };
    LoggerThreadArgs l_args = { log_mq };

    pthread_t t_sensor, t_processor, t_alert, t_logger;

    // Khởi chạy các thread
    pthread_create(&t_logger, NULL, logger_thread_func, &l_args);
    pthread_create(&t_alert, NULL, alert_thread_func, &a_args);
    pthread_create(&t_processor, NULL, processor_thread_func, &p_args);
    pthread_create(&t_sensor, NULL, sensor_thread_func, &s_args);

    printf("[Main] Hệ thống bắt đầu chạy (Thời gian: %d giây)...\n", duration_sec);

    // Đếm thời gian chạy, nếu hết giờ hoặc có tín hiệu dừng thì thoát vòng lặp
    int elapsed = 0;
    while (keep_running && (duration_sec == 0 || elapsed < duration_sec)) {
        sleep(1);
        elapsed++;
    }
    keep_running = 0;

    // Đánh thức luồng Processor phòng khi nó đang ngủ chờ dữ liệu
    pthread_mutex_lock(&cb.mutex);
    pthread_cond_signal(&cb.not_empty);
    pthread_mutex_unlock(&cb.mutex);

    // Chờ các thread kết thúc an toàn (Graceful shutdown)
    pthread_join(t_sensor, NULL);
    pthread_join(t_processor, NULL);
    pthread_join(t_alert, NULL);
    pthread_join(t_logger, NULL);

    // Dọn dẹp tài nguyên
    cb_cleanup(&cb);
    mq_close(alarm_mq);
    mq_close(log_mq);
    mq_unlink(ALARM_MQ_NAME);
    mq_unlink(LOG_MQ_NAME);

    printf("[Main] Đã dọn dẹp bộ nhớ và đóng hệ thống thành công!\n");
    return 0;
}
