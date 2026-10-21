#include "circular_buffer.h"
#include <stdlib.h>
#include <stdio.h>
extern volatile int keep_running;
int cb_init(CircularBuffer *cb, int capacity) {
    cb->buffer = (SensorData *)malloc(capacity * sizeof(SensorData));
    if (!cb->buffer) return -1;
    cb->capacity = capacity; cb->head = 0; cb->tail = 0; cb->count = 0;
    cb->overflow_count = 0; cb->total_produced = 0; cb->total_consumed = 0;
    pthread_mutex_init(&cb->mutex, NULL); pthread_cond_init(&cb->not_empty, NULL);
    return 0;
}
void cb_cleanup(CircularBuffer *cb) {
    free(cb->buffer); pthread_mutex_destroy(&cb->mutex); pthread_cond_destroy(&cb->not_empty);
}
int cb_push(CircularBuffer *cb, SensorData data) {
    pthread_mutex_lock(&cb->mutex);
    if (cb->count == cb->capacity) {
        cb->overflow_count++; pthread_mutex_unlock(&cb->mutex); return -1; 
    }
    cb->buffer[cb->head] = data; cb->head = (cb->head + 1) % cb->capacity;
    cb->count++; cb->total_produced++;
    pthread_cond_signal(&cb->not_empty); pthread_mutex_unlock(&cb->mutex);
    return 0;
}
int cb_pop(CircularBuffer *cb, SensorData *data) {
    pthread_mutex_lock(&cb->mutex);
    while (cb->count == 0 && keep_running) {
        pthread_cond_wait(&cb->not_empty, &cb->mutex);
    }
    if (!keep_running && cb->count == 0) {
        pthread_mutex_unlock(&cb->mutex); return -1;
    }
    *data = cb->buffer[cb->tail]; cb->tail = (cb->tail + 1) % cb->capacity;
    cb->count--; cb->total_consumed++;
    pthread_mutex_unlock(&cb->mutex); return 0;
}
