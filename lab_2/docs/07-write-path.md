# write() Path: Replace and Truncate

## Buffer contract

`BUFFER_SIZE` is `1024`. The driver reserves one byte for a `\0` terminator, so at most **1023 data bytes** can be stored in one write. `lab2_write()` reduces an oversized request to `BUFFER_SIZE - 1`, logs a warning, and copies the accepted bytes from userspace with `copy_from_user()`.

The callback then sets `buffer_len` to the accepted length, terminates the buffer, and returns the accepted byte count. Every write **replaces** the previous contents; it does not append. The `*offset` parameter is not used by this callback.

## Small verification example

```sh
printf 'first' > /dev/lab2
cat /dev/lab2
printf 'B' > /dev/lab2
cat /dev/lab2
cat /sys/class/lab2_class/lab2/buffer_len
```

If all operations succeed, the final data is `B` and the final stored length is `1`. In contrast, `echo 'B'` normally writes two bytes (`B` and a newline). For maximum-length checks, use a byte-counting utility and inspect both the return value and the driver's stored length rather than relying on printed terminal text.

## Boundary conditions

A requested write longer than 1023 bytes is truncated, not rejected. A failed `copy_from_user()` returns `-EFAULT`; the code does not finalize `buffer_len` in that error path. Avoid treating an untested oversize case as an observed result merely because the source contains the truncation logic.

## Source reference

[`lab2_driver.c`](../driver/lab2_driver.c): `lab2_write()`.
