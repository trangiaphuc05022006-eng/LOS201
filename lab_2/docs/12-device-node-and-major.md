# Device Nodes, Major and Minor Numbers

## Interpreting /dev/lab2

The source registers major number `240` and creates a device with `MKDEV(240, 0)`. A working character node appears as a `c`-type special file:

```text
crw-rw-rw- ... 240, 0 ... /dev/lab2
```

This is the representation seen in the tracked [`boot_log.txt`](../logs/boot_log.txt). The number pair identifies the kernel device to which system calls are routed; it is not a user-defined filename convention.

## Boot-time creation

The checked-in `/etc/init.d/rcS` first tries to mount `devtmpfs` and then, after successful `insmod`, runs `mknod /dev/lab2 c 240 0` and `chmod 666 /dev/lab2`. Depending on guest device management, the node might already exist when `mknod` runs. That is why an operator should inspect the node instead of assuming a second `mknod` always succeeds.

## Manual recovery

If the module is loaded but the node is absent, a root shell inside the guest can attempt:

```sh
[ -c /dev/lab2 ] || mknod /dev/lab2 c 240 0
ls -l /dev/lab2
```

Do not create the node before verifying the driver's registered major number. For the lab's default source, that number is 240; production drivers often use dynamic allocation instead.

## Safety note

The startup script uses `chmod 666`, permitting reads and writes by any user in this isolated teaching guest. That permission is convenient for a lab but should not be copied uncritically into production systems.

## Source references

[`lab2_driver.c`](../driver/lab2_driver.c), [`rcS`](../rootfs/initramfs/etc/init.d/rcS), and [`boot_log.txt`](../logs/boot_log.txt).
