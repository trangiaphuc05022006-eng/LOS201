# sysfs: Per-Device Read-Only Attributes

## Attribute locations

`lab2_init()` creates a device class named `lab2_class` and a device named `lab2`. It attaches a sysfs attribute group containing three `DEVICE_ATTR_RO()` entries:

```text
/sys/class/lab2_class/lab2/buffer_len
/sys/class/lab2_class/lab2/open_count
/sys/class/lab2_class/lab2/last_data
```

`buffer_len` and `open_count` return decimal numbers. `last_data` prints the buffered text or `(empty)`. These paths are device attributes; the file name `/proc/lab2_info` belongs to a different virtual filesystem.

## Example

```sh
SYS=/sys/class/lab2_class/lab2
cat "$SYS/buffer_len"
cat "$SYS/open_count"
cat "$SYS/last_data"
```

Try writing through `/dev/lab2` and reading all three again. For precise byte counts, use `printf` rather than `echo`, because `echo` generally appends a newline.

## API and caution

`DEVICE_ATTR_RO(name)` connects each attribute to a `name_show()` callback and defines read-only permissions. The source uses `sysfs_create_group()` after `device_create()` and removes the group in `lab2_exit()`.

If group creation fails, the driver logs a warning but continues initialization. Do not infer that sysfs is available solely because `/dev/lab2` works. Also, this project uses `sprintf()` and a simple text representation; it is a teaching implementation, not a general-purpose binary-data sysfs interface.

## Source reference

[`lab2_driver.c`](../driver/lab2_driver.c): `DEVICE_ATTR_RO`, `lab2_attr_group`, and `sysfs_create_group()`.
