#include "sensor_thread.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>

extern volatile int keep_running;

// Hàm tính khoảng thời gian chênh lệch (micro giây)
long timespec_diff_us(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000000 + (end->tv_nsec - start->tv_nsec) / 1000;
}

void *sensor_thread_func(void *arg) {
    SensorThreadArgs *args = (SensorThreadArgs *)arg;
    int fd = open("/dev/sms_sensor", O_RDONLY);
    if (fd < 0) return NULL;

    // Task 4: Kích hoạt chế độ Real-time SCHED_FIFO (yêu cầu quyền sudo)
    struct sched_param param;
    param.sched_priority = 50;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("[Cảnh báo] Không thể set SCHED_FIFO (Nhớ chạy app bằng sudo nhé!)");
    }
    mlockall(MCL_CURRENT | MCL_FUTURE); // Khóa bộ nhớ để tránh page fault

    char buf[128]; SensorData data;
    struct timespec expected, actual;
    clock_gettime(CLOCK_MONOTONIC, &expected);

    FILE *jf = fopen("../logs/jitter_report.txt", "a");

    while (keep_running) {
        // Cộng thêm thời gian ngủ dự kiến (interval)
        expected.tv_nsec += args->interval_ms * 1000000;
        if (expected.tv_nsec >= 1000000000) {
            expected.tv_nsec -= 1000000000;
            expected.tv_sec++;
        }
        
        // Ngủ chính xác tuyệt đối bằng clock_nanosleep
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &expected, NULL);
        clock_gettime(CLOCK_MONOTONIC, &actual);

        // Tính Jitter và ghi ra file
        long jitter = timespec_diff_us(&expected, &actual);
        if (jf && jitter >= 0) fprintf(jf, "%ld\n", jitter);

        int bytes = pread(fd, buf, sizeof(buf) - 1, 0);
        if (bytes > 0) {
            buf[bytes] = '\0';
            if (sscanf(buf, "%lu,%f,%f", &data.timestamp, &data.temperature, &data.humidity) == 3) {
                cb_push(args->cb, data);
            }
        }
    }
    if (jf) fclose(jf);
    close(fd); return NULL;
}
