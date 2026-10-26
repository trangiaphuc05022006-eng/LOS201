# BusyBox initramfs Boot Sequence

## Observed boot

The saved [`boot_log.txt`](../logs/boot_log.txt) identifies a Linux `5.15.0` ARMv7 boot on an `V2P-CA9` machine. Its recorded kernel command line includes:

```text
console=ttyAMA0,115200 rdinit=/sbin/init mem=512M
```

The kernel starts `/sbin/init`; BusyBox reads `/etc/inittab`, which contains `::sysinit:/etc/init.d/rcS`. That script mounts `/proc`, `/sys`, `/tmp`, and tries `devtmpfs` for `/dev`. It then checks for `/lib/modules/5.15.0/lab2_driver.ko` and calls `insmod` if the file is present.

```mermaid
flowchart TD
    K[Linux 5.15 boots] --> I[BusyBox /sbin/init]
    I --> T[/etc/inittab]
    T --> R[/etc/init.d/rcS]
    R --> M[Mount procfs, sysfs, tmpfs, devtmpfs]
    M --> L{Module .ko exists?}
    L -->|yes| X[insmod lab2_driver.ko]
    X --> N[Set up /dev/lab2]
    L -->|no| W[Warn and continue]
```

## Verification

The boot log includes the messages `lab2_driver: loaded, major=240` and `[ LAB-02 ] lab2_driver loaded OK`. It also records a character node with major `240` and minor `0` and initial driver statistics. These are **observed outputs** from that session, not guarantees for every local rebuild.

The general README also contains a sample QEMU invocation with a different `rdinit` path. Use the actual command-line and init configuration of the image you boot, rather than assuming the two startup paths are interchangeable.

## Source references

[`inittab`](../rootfs/initramfs/etc/inittab), [`rcS`](../rootfs/initramfs/etc/init.d/rcS), and [`boot_log.txt`](../logs/boot_log.txt).
