# lab_2 — Character Device Driver for QEMU ARM

Implements the `lab2_driver` Linux kernel module — a simple character device driver — cross-compiled for ARM Cortex-A9 (hard-float ABI), packaged into an initramfs, and tested inside QEMU ARM.

---

## Objectives

- Write a character device driver using `cdev`, `device_create`, a `/proc` interface, and mutex locking.
- Cross-compile with `arm-linux-gnueabihf-` (hard-float ABI, `-march=armv7-a -mtune=cortex-a9`).
- Package the driver into an initramfs together with the root filesystem from Lab 1.
- Load and test the driver inside QEMU ARM.

---

## Directory Structure

```
lab_2/
├── driver/
│   ├── lab2_driver.c        # Character device driver (Major 240, "lab2_class", 1024 B buffer, /proc entry)
│   ├── Makefile             # Build script (ARCH=arm, arm-linux-gnueabihf-, -march=armv7-a -mtune=cortex-a9)
│   ├── lab2_driver.ko       # Pre-built kernel module (~140 KB, binary file)
│   ├── lab2_driver.o        # Object file (~112 KB)
│   ├── lab2_driver.mod      # Module descriptor
│   ├── lab2_driver.mod.c    # Module metadata (auto-generated)
│   ├── lab2_driver.mod.o    # Compiled metadata
│   ├── Module.symvers       # Symbol versions
│   ├── modules.order        # Module load order
│   └── .*.cmd               # Kconfig command files (auto-generated)
├── configs/                 # Configuration directory (currently empty)
├── mtd/                     # MTD-related files (currently empty)
├── rootfs/
│   └── initramfs/           # Root filesystem for Lab 2
└── output/
    └── initramfs_lab2.cpio.gz  # Packaged initramfs (~8.1 MB, Git LFS)
```

---

## Requirements

| Tool | Notes |
|---|---|
| `arm-linux-gnueabihf-gcc` | Hard-float cross-compiler (different from `arm-linux-gnueabi-` used in Lab 1) |
| Linux kernel 5.15 headers | `KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15` (per Makefile) |
| `make` | Build the kernel module |
| `qemu-system-arm` | Run the initramfs in QEMU (requires `zImage` and `vexpress-v2p-ca9.dtb` from `lab_1/output/`) |
| `git-lfs` | Required to pull `initramfs_lab2.cpio.gz` |

---

## Build & Run

**Build kernel module (on the Linux host):**

```bash
cd lab_2/driver
make        # produces lab2_driver.ko
make clean
```

> The Makefile automatically uses `arm-linux-gnueabihf-` and the flags `-march=armv7-a -mtune=cortex-a9`.

**Boot in QEMU with the Lab 2 initramfs:**

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -kernel lab_1/output/zImage \
  -dtb lab_1/output/vexpress-v2p-ca9.dtb \
  -initrd lab_2/output/initramfs_lab2.cpio.gz \
  -append "console=ttyAMA0 root=/dev/ram rdinit=/linuxrc" \
  -nographic
```

> The QEMU command above is inferred from the artifact file names (see Lab 1 for `zImage` and `.dtb`).

**Load and test driver inside QEMU (as root):**

```sh
insmod /lib/modules/5.15.0/lab2_driver.ko
mknod /dev/lab2 c 240 0
echo "hello" > /dev/lab2
cat /dev/lab2
cat /proc/lab2     # read stats via /proc interface
rmmod lab2_driver
```

---

## Results / Output

| File | Content |
|---|---|
| `driver/lab2_driver.ko` | Kernel module cross-compiled for ARM |
| `output/initramfs_lab2.cpio.gz` | Complete initramfs containing the driver and root filesystem (~8.1 MB, Git LFS) |

---

## Notes

- Lab 2 uses `arm-linux-gnueabihf-` (hard-float) instead of `arm-linux-gnueabi-` from Lab 1 / Assignment 1 — use the correct toolchain.
- `driver/lab2_driver.c` registers module name `lab2`, class `lab2_class`, major number 240, buffer size 1024 bytes, and a `/proc` entry for reading stats.
- The `configs/` and `mtd/` directories are currently empty and may be populated later.

---

[← Back to main README](../README.md)
