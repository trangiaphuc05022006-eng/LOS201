# Crossing the User/Kernel Boundary

## Why explicit copy helpers are used

The driver receives pointers annotated as `__user`. These addresses originate from a process, not from a trusted kernel allocation. It uses `copy_from_user()` in `lab2_write()` and `copy_to_user()` in `lab2_read()`, rather than dereferencing user pointers directly or using ordinary `memcpy()` against them.

A failing copy is reported to userspace as `-EFAULT`. The path is important even in a small teaching module because an invalid userspace pointer must not be assumed safe to access as a normal kernel pointer.

## Direction of data

```mermaid
flowchart LR
    A[Userspace write buffer] -->|copy_from_user| B[device_buffer in kernel]
    B -->|copy_to_user| C[Userspace read buffer]
```

The module stores bytes in a single shared array. It tracks their meaningful length separately in `buffer_len`, then adds a NUL terminator so the last stored data can be displayed as a string in procfs/sysfs. The data interface itself is byte-oriented: callers should not assume every possible payload is printable text.

## Return conventions

- `write()` returns the number of bytes accepted from the caller, which may be lower than requested due to truncation.
- `read()` returns bytes copied, `0` at EOF, or a negative error from the kernel callback.
- An `-EFAULT` result is propagated through the syscall interface and becomes an error seen by the userspace caller.

## Source reference

[`lab2_driver.c`](../driver/lab2_driver.c): `lab2_read()` and `lab2_write()`.
