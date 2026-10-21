#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H
#include <pthread.h>
#include "sms_sensor.h"
typedef struct {
    SensorData *buffer;
    int capacity;
    int head;
    int tail;
    int count;
    int overflow_count;
    int total_produced;
    int total_consumed;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} CircularBuffer;
int cb_init(CircularBuffer *cb, int capacity);
void cb_cleanup(CircularBuffer *cb);
int cb_push(CircularBuffer *cb, SensorData data);
int cb_pop(CircularBuffer *cb, SensorData *data);
#endif
