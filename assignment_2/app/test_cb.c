#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "circular_buffer.h"

#define N_SAMPLES 1000
#define CAPACITY  8

static CircularBuffer *cb;
static unsigned long attempted;     /* how many pushes the producer tried */
static unsigned long received;      /* how many items the consumer got */
static int           order_errors;  /* timestamps not strictly increasing */

static void *producer(void *arg)
{
    (void)arg;
    /* Count every push attempt, including samples rejected when the buffer is full. */
    for (int i = 0; i < N_SAMPLES; i++) {
        SensorData sample = {
            .timestamp_ms = (uint64_t)i,
            .temperature = 20.0,
            .humidity = 50.0,
        };

        (void)cb_push(cb, &sample);
        attempted++;
        /* Periodically yield so the consumer has time to drain the queue. */
        if ((i + 1) % 50 == 0) {
            usleep(1000);
        }
    }
    return NULL;
}

static void *consumer(void *arg)
{
    (void)arg;
    SensorData sample;
    uint64_t previous_timestamp = 0;
    int have_previous = 0;

    /* Keep consuming until shutdown and all queued samples have been drained. */
    while (cb_pop(cb, &sample) == 0) {
        if (have_previous && sample.timestamp_ms <= previous_timestamp) {
            order_errors++;
        }
        previous_timestamp = sample.timestamp_ms;
        have_previous = 1;
        received++;
    }
    return NULL;
}

int main(void)
{
    pthread_t p, c;
    unsigned long produced, consumed, overflow;
    int err;
    int passed;

    cb = cb_create(CAPACITY);
    if (!cb) {
        fprintf(stderr, "cb_create failed\n");
        return 1;
    }

    err = pthread_create(&p, NULL, producer, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_create producer failed: %s\n", strerror(err));
        cb_destroy(cb);
        return 1;
    }

    err = pthread_create(&c, NULL, consumer, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_create consumer failed: %s\n", strerror(err));
        if (pthread_join(p, NULL) != 0) {
            return 1;
        }
        cb_shutdown(cb);
        cb_destroy(cb);
        return 1;
    }

    /* Finish production before shutdown, allowing the consumer to drain the tail. */
    err = pthread_join(p, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join producer failed: %s\n", strerror(err));
        return 1;
    }
    cb_shutdown(cb);

    err = pthread_join(c, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join consumer failed: %s\n", strerror(err));
        return 1;
    }

    produced = cb_total_produced(cb);
    consumed = cb_total_consumed(cb);
    overflow = cb_overflow_count(cb);
    passed = produced == consumed && consumed == received &&
             produced + overflow == attempted && order_errors == 0;

    printf("produced=%lu consumed=%lu overflow=%lu attempted=%lu received=%lu "
           "order_errors=%d %s\n",
           produced, consumed, overflow, attempted, received, order_errors,
           passed ? "PASS" : "FAIL");

    cb_destroy(cb);
    return passed ? 0 : 1;
}