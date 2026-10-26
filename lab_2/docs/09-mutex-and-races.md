# Shared State and Mutex Locking

## Protected state

The module defines one static `DEFINE_MUTEX(lab2_mutex)` and a shared device buffer. In the current source, the mutex protects the main state accesses in `open`, `read`, `write`, the procfs summary callback, and the three sysfs `show` callbacks.

| Path | Protected operation |
|---|---|
| `lab2_open()` | Increment `open_count` |
| `lab2_read()` | Inspect `buffer_len`, copy stored bytes, advance offset |
| `lab2_write()` | Replace bytes and update `buffer_len` |
| `lab2_proc_show()` | Capture and print driver statistics |
| sysfs `*_show()` | Read the associated value |

This is a **single-lock design**: it is straightforward to reason about but serializes all of these operations. The lock is a sleepable kernel mutex, not a spinlock.

## Concurrency scenario

Suppose reader A tries to read while writer B is replacing the buffer. Without coordination, A could observe a length from one version and bytes from another. With both operations taking the same lock, the reader and writer cannot execute their protected sections simultaneously.

The implementation keeps the mutex held while calling `copy_to_user()` or `copy_from_user()`. That is a design trade-off: the code is simple, but such copies may be slower than reading or writing only kernel-resident fields. This lab does not provide throughput or contention benchmarks, so no performance claim is made here.

## Suggested validation

Run multiple independent `cat /dev/lab2` and write processes, then inspect procfs and the kernel log. Concurrent results are not deterministic, so record actual output instead of hard-coding a success claim.

## Source reference

[`lab2_driver.c`](../driver/lab2_driver.c): `lab2_mutex` and all callbacks that lock it.
