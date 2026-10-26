# open() and release() Lifecycle

## Driver callbacks

The module's `struct file_operations` maps `.open` to `lab2_open()` and `.release` to `lab2_release()`. Whenever a process opens the character device, `lab2_open()` increments the global `open_count` while holding `lab2_mutex`. Closing a file descriptor triggers `lab2_release()`, which logs a closure but does **not** decrement the counter.

Consequently, `open_count` means **cumulative successful opens since module insertion**, not the number of currently active clients. Two separate commands such as `echo 'hello' > /dev/lab2` and `cat /dev/lab2` each open the device; the count will normally increase for both.

## Why file descriptors matter

A file's read offset belongs to that open file description. `lab2_read()` checks `*offset` against `buffer_len`, and returns zero at end-of-file. When a new `cat` invocation opens `/dev/lab2`, it gets a new offset and can read the last stored data again. Closing and reopening does not reset the module's shared buffer or its `open_count`.

The callback increments `open_count` under the mutex, but prints the shared count after releasing it. In highly concurrent use, the exact value printed in an individual log line may no longer be a stable snapshot.

## How to observe

```sh
cat /proc/lab2_info
cat /dev/lab2
cat /proc/lab2_info
```

Compare `Open count` before and after. Reading `/proc/lab2_info` does not call the character-device `lab2_open()` callback; it uses its own procfs file operations.

## Source reference

[`lab2_driver.c`](../driver/lab2_driver.c): `lab2_open()`, `lab2_release()`, and `lab2_read()`.
