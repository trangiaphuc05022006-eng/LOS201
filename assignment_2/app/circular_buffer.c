/*
 * circular_buffer.c
 * A simple circular buffer implementation for managing sensor data.
 * Author: <Tran Hoang Khoi - SE203155>
 */

#include <stdlib.h>
#include <pthread.h>
#include "circular_buffer.h"

struct CircularBuffer {
    SensorData     *items;        /* heap array, allocated once */
    size_t          capacity, head, tail, size;
    unsigned long   total_produced, total_consumed, overflow_count;
    int             shutting_down;
    pthread_mutex_t lock;
    pthread_cond_t  not_empty;
};

CircularBuffer *cb_create(size_t capacity)
{
    /* calloc the struct and the items array (check both!), set
     * capacity, init mutex and cond. On any failure free what you
     * allocated and return NULL. */
    struct CircularBuffer *cb;

    if (capacity == 0) {
        return NULL;
    }

    cb = calloc(1, sizeof(*cb));
    if (!cb) {
        return NULL;
    }

    cb->items = calloc(capacity, sizeof(*cb->items));
    if (!cb->items) {
        free(cb);
        return NULL;
    }

    cb->capacity = capacity;
    if (pthread_mutex_init(&cb->lock, NULL) != 0) {
        free(cb->items);
        free(cb);
        return NULL;
    }

    if (pthread_cond_init(&cb->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&cb->lock);
        free(cb->items);
        free(cb);
        return NULL;
    }

    return cb;

    
}

void cb_destroy(CircularBuffer *cb)
{
    /*  destroy cond and mutex, free items, free cb (cb may be NULL) */
    if (cb == NULL) { 
            return; 
    }
    pthread_cond_destroy(&cb->not_empty);
    pthread_mutex_destroy(&cb->lock);
    free(cb->items);
    free(cb);
}

int cb_push(CircularBuffer *cb, const SensorData *d)
{
    /* lock. If full: overflow_count++, unlock, return -1.
     *         Else copy *d to items[head], advance head (wrap around),
     *         size++, total_produced++, signal not_empty, unlock, return 0 */
    pthread_mutex_lock(&cb->lock);
    if (cb->size == cb->capacity) { 
            cb->overflow_count++;
            pthread_mutex_unlock(&cb->lock); 
            return -1;
    }
    else {
        cb->items[cb->head] = *d;
        cb->head = (cb->head + 1) % cb->capacity;
        cb->size++;
        cb->total_produced++;
        pthread_cond_signal(&cb->not_empty);
        pthread_mutex_unlock(&cb->lock);
        return 0;
    }
}

int cb_pop(CircularBuffer *cb, SensorData *out)
{
    /* lock. While (size == 0 && !shutting_down) wait on not_empty.
     * If size == 0 now (we were shut down): unlock, return -1.
     * Else take items[tail] into *out, advance tail, size--,
     * total_consumed++, unlock, return 0 */
    pthread_mutex_lock(&cb->lock); 
    while (cb->size == 0 && !cb->shutting_down) { 
        pthread_cond_wait(&cb->not_empty, &cb->lock); 
    }
    if (cb->size == 0) {
        pthread_mutex_unlock(&cb->lock);
        return -1;
    }

    *out = cb->items[cb->tail];
    cb->tail = (cb->tail + 1) % cb->capacity;
    cb->size--;
    cb->total_consumed++;
    pthread_mutex_unlock(&cb->lock);
    return 0;
}

void cb_shutdown(CircularBuffer *cb)
{
    /* lock, shutting_down = 1, broadcast not_empty, unlock */
    pthread_mutex_lock(&cb->lock); 
    cb->shutting_down = 1; 
    pthread_cond_broadcast(&cb->not_empty);
    pthread_mutex_unlock(&cb->lock);

}

size_t cb_size(CircularBuffer *cb)
{
    size_t value;

    pthread_mutex_lock(&cb->lock);
    value = cb->size;
    pthread_mutex_unlock(&cb->lock);
    return value;
}

unsigned long cb_total_produced(CircularBuffer *cb)
{
    unsigned long value;

    pthread_mutex_lock(&cb->lock);
    value = cb->total_produced;
    pthread_mutex_unlock(&cb->lock);
    return value;
}

unsigned long cb_total_consumed(CircularBuffer *cb)
{
    unsigned long value;

    pthread_mutex_lock(&cb->lock);
    value = cb->total_consumed;
    pthread_mutex_unlock(&cb->lock);
    return value;
}

unsigned long cb_overflow_count(CircularBuffer *cb)
{
    unsigned long value;

    pthread_mutex_lock(&cb->lock);
    value = cb->overflow_count;
    pthread_mutex_unlock(&cb->lock);
    return value;
}