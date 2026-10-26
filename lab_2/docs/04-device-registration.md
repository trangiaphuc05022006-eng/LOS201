# Character Device Registration

## Initialization steps

`lab2_init()` is the module entry point. It calls `register_chrdev(240, "lab2", &lab2_fops)` to register a fixed-major character-device interface. After successful registration, it creates the class `lab2_class` and device `lab2` using `class_create()` and `device_create()`.

The `device_create()` call supplies `MKDEV(240, 0)`, so the intended node is `/dev/lab2` with major `240`, minor `0`. This is the interface used by `echo`, `cat`, and the provided test scripts. Registration does **not** mean the module owns every possible minor number for arbitrary purposes; the code documents and creates only this `lab2` device.

## Error paths and cleanup

If character registration fails, initialization returns the error. If class creation fails, the code calls `unregister_chrdev()`. If device creation fails, it destroys the class and unregisters the character device. This staged rollback prevents leaving the most important earlier allocations registered after failure.

After device creation, `lab2_init()` also attempts to register sysfs attributes and `/proc/lab2_info`. In the current implementation, failure of those optional interfaces logs a warning and does not abort module initialization. Therefore, seeing a loaded module does **not** by itself prove that both observability interfaces exist.

## Useful checks

```sh
lsmod | grep lab2
ls -l /dev/lab2
cat /proc/devices | grep -w lab2
```

Run these commands **inside the QEMU guest**. Some commands depend on the provided BusyBox configuration.

## Source reference

[`lab2_driver.c`](../driver/lab2_driver.c), specifically `lab2_init()` and `lab2_exit()`.
