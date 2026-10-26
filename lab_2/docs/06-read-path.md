# read() Path: Offset, Length, and EOF

## Implementation

`lab2_read()` locks `lab2_mutex` and compares the incoming file offset with `buffer_len`. When `*offset >= buffer_len`, it returns `0` to report EOF. Otherwise, it limits the requested length to the number of bytes still available and calls:

```c
copy_to_user(buf, device_buffer + *offset, len)
```

The call copies bytes from a kernel-owned buffer to the caller's userspace buffer. Failure returns `-EFAULT`; success advances `*offset` by the number of bytes transferred. The callback returns the transferred byte count, not the entire buffer capacity.

## Behavior to expect

Assume `printf 'ABC' > /dev/lab2` stores three bytes. A reader that requests two bytes should first receive `AB`, then `C`, and finally `0` at EOF, **if it keeps the same open file description**. A fresh open starts at offset zero. The regular `cat /dev/lab2` command repeatedly reads until it receives EOF, then exits.

Note that `echo` usually writes a trailing newline; for examples requiring exact byte counts, use `printf` instead. `/dev/lab2` is not a streaming sensor. It is a simple shared buffer: writes replace its contents rather than appending to a queue.

## Concurrency note

The current code holds the mutex across its buffer-length check, `copy_to_user()`, and offset increment. This keeps its own read operation consistent with writes that use the same lock, although it may also hold the lock across a userspace memory copy.

## Source reference

[`lab2_driver.c`](../driver/lab2_driver.c): `lab2_read()`.
