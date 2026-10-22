# lab_2 — Character Device Driver cho QEMU ARM

Viết Linux kernel module `lab2_driver` — một simple character device driver — cross-compiled cho ARM Cortex-A9 (hard-float ABI), đóng gói vào initramfs và chạy trong QEMU ARM.

---

## Mục tiêu

- Viết character device driver với `cdev`, `device_create`, `/proc` interface và mutex.
- Cross-compile với `arm-linux-gnueabihf-` (hard-float ABI, `-march=armv7-a -mtune=cortex-a9`).
- Đóng gói driver vào initramfs cùng với root filesystem từ Lab 1.
- Load và kiểm thử driver bên trong QEMU ARM.

---

## Cấu trúc thư mục

```
lab_2/
├── driver/
│   ├── lab2_driver.c        # Character device driver (Major 240, "lab2_class", buffer 1024 B, /proc entry)
│   ├── Makefile             # Build module (ARCH=arm, arm-linux-gnueabihf-, -march=armv7-a -mtune=cortex-a9)
│   ├── lab2_driver.ko       # Kernel module đã build (~140 KB, file nhị phân)
│   ├── lab2_driver.o        # Object file (~112 KB)
│   ├── lab2_driver.mod      # Module descriptor
│   ├── lab2_driver.mod.c    # Module metadata (auto-generated)
│   ├── lab2_driver.mod.o    # Compiled metadata
│   ├── Module.symvers       # Symbol versions
│   ├── modules.order        # Module load order
│   └── .*.cmd               # Kconfig command files (auto-generated)
├── configs/                 # Thư mục cấu hình (hiện trống)
├── mtd/                     # MTD-related (hiện trống)
├── rootfs/
│   └── initramfs/           # Root filesystem cho Lab 2
└── output/
    └── initramfs_lab2.cpio.gz  # initramfs đóng gói (~8.1 MB, Git LFS)
```

---

## Yêu cầu

| Công cụ | Ghi chú |
|---|---|
| `arm-linux-gnueabihf-gcc` | Cross-compiler hard-float (khác Lab 1 dùng `arm-linux-gnueabi-`) |
| Linux kernel 5.15 headers | `KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15` (theo Makefile) |
| `make` | Build kernel module |
| `qemu-system-arm` | Chạy initramfs trong QEMU (dùng `zImage` và `vexpress-v2p-ca9.dtb` từ `lab_1/output/`) |
| `git-lfs` | Cần để pull `initramfs_lab2.cpio.gz` |

---

## Build & Run

**Build kernel module (trên máy host Linux):**

```bash
cd lab_2/driver
make        # tạo lab2_driver.ko
make clean
```

> Makefile tự động dùng `arm-linux-gnueabihf-` và flag `-march=armv7-a -mtune=cortex-a9`.

**Boot trong QEMU với initramfs Lab 2:**

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -kernel lab_1/output/zImage \
  -dtb lab_1/output/vexpress-v2p-ca9.dtb \
  -initrd lab_2/output/initramfs_lab2.cpio.gz \
  -append "console=ttyAMA0 root=/dev/ram rdinit=/linuxrc" \
  -nographic
```

> Lệnh QEMU trên là suy luận từ tên file artifact (xem Lab 1 để có `zImage` và `.dtb`).

**Load và test driver trong QEMU (với tư cách root):**

```sh
insmod /lib/modules/5.15.0/lab2_driver.ko
mknod /dev/lab2 c 240 0
echo "hello" > /dev/lab2
cat /dev/lab2
cat /proc/lab2     # đọc stats qua /proc interface
rmmod lab2_driver
```

---

## Kết quả / Output

| File | Nội dung |
|---|---|
| `driver/lab2_driver.ko` | Kernel module đã cross-compile cho ARM |
| `output/initramfs_lab2.cpio.gz` | initramfs hoàn chỉnh chứa driver và rootfs (~8.1 MB, Git LFS) |

---

## Ghi chú

- Lab 2 dùng `arm-linux-gnueabihf-` (hard-float) thay vì `arm-linux-gnueabi-` của Lab 1/Assignment 1 — cần đúng toolchain.
- `driver/lab2_driver.c` có module tên `lab2`, class `lab2_class`, major number 240, buffer 1024 bytes, và `/proc` entry để đọc stats.
- Thư mục `configs/` và `mtd/` hiện trống — có thể sẽ bổ sung sau.

---

[← Về README chính](../README.md)
