/*
 * jitter.c - Wake-up jitter statistics (Min/Avg/Max/P99) and text histogram
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-21
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jitter.h"

static int cmp_long(const void *a, const void *b)
{
    long va = *(const long *)a;
    long vb = *(const long *)b;

    return (va > vb) - (va < vb);
}

void jitter_report(const long *us, size_t n, FILE *out)
{
    long *sorted;
    double sum = 0.0;
    size_t p99_idx;
    size_t b_0_100 = 0;
    size_t b_100_500 = 0;
    size_t b_500_1000 = 0;
    size_t b_gt_1000 = 0;

    if (!out) {
        return;
    }
    if (!us || n == 0) {
        fprintf(out, "jitter: no samples\n");
        return;
    }

    sorted = malloc(n * sizeof(*sorted));
    if (!sorted) {
        perror("jitter_report: malloc");
        return;
    }
    memcpy(sorted, us, n * sizeof(*sorted));
    qsort(sorted, n, sizeof(*sorted), cmp_long);

    for (size_t i = 0; i < n; i++) {
        long v = sorted[i];

        sum += (double)v;
        if (v < 100) {
            b_0_100++;
        } else if (v < 500) {
            b_100_500++;
        } else if (v <= 1000) {
            b_500_1000++;
        } else {
            b_gt_1000++;
        }
    }

    p99_idx = (size_t)ceil(0.99 * (double)n);
    if (p99_idx > 0) {
        p99_idx--;
    }
    if (p99_idx >= n) {
        p99_idx = n - 1;
    }

    fprintf(out,
            "Jitter summary: samples=%zu Min=%ld us Avg=%.2f us Max=%ld us P99=%ld us\n"
            "Histogram:\n"
            "  0-100 us   : %zu\n"
            "  100-500 us : %zu\n"
            "  500-1000 us: %zu\n"
            "  >1 ms      : %zu\n",
            n, sorted[0], sum / (double)n, sorted[n - 1], sorted[p99_idx],
            b_0_100, b_100_500, b_500_1000, b_gt_1000);

    free(sorted);
}