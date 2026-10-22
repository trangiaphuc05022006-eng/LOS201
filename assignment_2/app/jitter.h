/*
 * jitter.h - Wake-up jitter statistics and text histogram reporting
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-21
 */
#ifndef JITTER_H
#define JITTER_H

#include <stddef.h>
#include <stdio.h>

void jitter_report(const long *us, size_t n, FILE *out);

#endif