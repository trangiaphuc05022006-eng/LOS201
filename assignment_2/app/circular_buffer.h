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

/* Returns NULL if capacity is 0 or allocation/initialization fails. */
CircularBuffer *cb_create(size_t capacity);

/* Call only after all threads using the buffer have exited and been joined. */
void            cb_destroy(CircularBuffer *cb);

/* Returns 0 on success; -1 if full, with overflow_count incremented. */
int             cb_push(CircularBuffer *cb, const SensorData *d);

/* Blocks for data; drains queued items after shutdown. Returns -1 only when
 * shutdown is set and the buffer is empty. */
int             cb_pop(CircularBuffer *cb, SensorData *out);

/* Prevents further pushes and wakes any blocked consumer. */
void            cb_shutdown(CircularBuffer *cb);

size_t          cb_size(CircularBuffer *cb);
unsigned long   cb_total_produced(CircularBuffer *cb);
unsigned long   cb_total_consumed(CircularBuffer *cb);
unsigned long   cb_overflow_count(CircularBuffer *cb);
#endif