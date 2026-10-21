/*
 * circular_buffer.h
 * A simple circular buffer implementation for managing sensor data.
 * Author: <Tran Hoang Khoi - SE203155>
 */
                
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t timestamp_ms;
    double   temperature;
    double   humidity;
} SensorData;

typedef struct CircularBuffer CircularBuffer;   /* fields hidden in the .c file */

CircularBuffer *cb_create(size_t capacity);
void            cb_destroy(CircularBuffer *cb);
int             cb_push(CircularBuffer *cb, const SensorData *d);  /* 0 ok, -1 full */
int             cb_pop(CircularBuffer *cb, SensorData *out);       /* blocks; -1 after shutdown */
    void            cb_shutdown(CircularBuffer *cb);
size_t          cb_size(CircularBuffer *cb);
unsigned long   cb_total_produced(CircularBuffer *cb);
unsigned long   cb_total_consumed(CircularBuffer *cb);
unsigned long   cb_overflow_count(CircularBuffer *cb);
#endif