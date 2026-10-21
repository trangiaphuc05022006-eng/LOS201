#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

#include "circular_buffer.h"

// Cấu trúc truyền tham số từ hàm main vào thread
typedef struct {
    CircularBuffer *cb;
    int interval_ms;
    int verbose;
} SensorThreadArgs;

void *sensor_thread_func(void *arg);

#endif
