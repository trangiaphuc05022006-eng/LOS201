# LAB-02 Architecture and Data Flow

## Scope

This lab packages an out-of-tree character-device kernel module in a BusyBox-based ARM initramfs. The project targets QEMU's Versatile Express Cortex-A9 board with a Linux 5.15 kernel. The device is named `lab2`, with a fixed major number of `240` and minor number `0`.

```mermaid
flowchart LR
    U[Userspace: echo / cat] --> D[/dev/lab2]
    D --> V[Linux VFS]
    V --> F[lab2_fops]
    F --> B[Shared 1024-byte buffer]
    P[cat /proc/lab2_info] --> Q[procfs seq_file callback]
    S[cat /sys/class/lab2_class/lab2/*] --> A[sysfs show callbacks]
    B --> Q
    B --> A
```

## Boundaries

- **Data interface:** `/dev/lab2` exposes `open`, `read`, `write`, and `release` through `struct file_operations`.
- **Observability interface:** `/proc/lab2_info` summarizes the driver's state. Three read-only device attributes appear under `/sys/class/lab2_class/lab2/`.
- **Boot integration:** `/etc/init.d/rcS` inside the initramfs attempts to load `lab2_driver.ko` and create `/dev/lab2`.
- **Test clients:** `test_driver.sh` exercises data exchange; `test_procfs.sh` queries virtual-filesystem status.

These are interfaces of one module, not three independent drivers. The procfs and sysfs files reflect the same underlying driver state. This distinction matters when interpreting an `open_count` increase after reading the character device.

## Source references

[`lab2_driver.c`](../driver/lab2_driver.c), [`rcS`](../rootfs/initramfs/etc/init.d/rcS), and the [main LAB-02 README](../README.md).
