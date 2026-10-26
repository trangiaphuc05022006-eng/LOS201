# procfs: /proc/lab2_info

## Purpose

The module registers a read-only procfs entry named `/proc/lab2_info` with mode `0444`. Its `proc_ops` table connects `lab2_proc_open()` to `single_open()`, and uses `seq_read()`, `seq_lseek()`, and `single_release()` for the file lifecycle.

The show callback reports a small status snapshot: driver name, major number, buffer size, current stored length, cumulative open count, and the most recently written text if nonempty.

## Reading it

```sh
cat /proc/lab2_info
printf 'LAB02' > /dev/lab2
cat /proc/lab2_info
```

The first read can display an empty buffer; the second should reflect the replacement write if the driver is loaded and the operation succeeds. The procfs entry is not a device-data channel: writing to `/dev/lab2`, not `/proc/lab2_info`, changes the buffer.

## Implementation detail

The `seq_file` helper handles formatting and normal file read behavior without manual tracking of an ad-hoc procfs buffer. The current callback takes `lab2_mutex` while reading and formatting driver fields. It only prints `Last data` when `buffer_len > 0`.

The procfs entry is optional during `lab2_init()`: failure to create it produces a kernel warning while the module continues to load. Therefore, verify the procfs path independently from module load status.

## Source reference

[`lab2_driver.c`](../driver/lab2_driver.c): `lab2_proc_show()`, `lab2_proc_open()`, and the `proc_ops` declaration.
