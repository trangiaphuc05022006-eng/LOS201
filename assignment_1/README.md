# assignment_1 — Linux Kernel Device Driver (ioctl + misc)

Viết hai Linux kernel module cho ARM: một character device driver với giao tiếp `ioctl`, và một misc device driver dạng counter. Chạy và kiểm thử trong môi trường QEMU ARM.

---

## Mục tiêu

- Hiểu cơ chế character device driver trong Linux kernel (register, file_operations, ioctl).
- Triển khai các lệnh `ioctl` tùy chỉnh: `RESET_BUFFER`, `GET_STATS`, `SET_MODE`, `GET_VERSION`.
- Viết misc device driver đơn giản (counter).
- Kiểm thử driver bằng shell script bên trong QEMU (insmod, mknod, đọc/ghi qua `/dev`).

---

## Cấu trúc thư mục

```
assignment_1/
├── task1_ioctl/
│   ├── asgn1_driver.c      # Character device driver (Major 241, buffer 1024 B, hai mode)
│   ├── asgn1_ioctl.h       # Header: magic number, struct asgn1_stats, 4 ioctl commands
│   ├── asgn1_ctl.c         # Userspace control tool
│   ├── asgn1_ctl           # Binary userspace tool (đã build, ARM)
│   ├── asgn1_driver.ko     # Kernel module đã build (Git LFS — file nhị phân)
│   ├── check               # Binary test helper (ARM)
│   ├── check_output.txt    # Output mẫu của check
│   ├── test_asgn1.sh       # Shell script kiểm thử chức năng (chạy trong QEMU)
│   ├── Makefile            # Build module (ARCH=arm, arm-linux-gnueabi-)
│   └── .vscode/            # Cài đặt editor (không liên quan đến build)
├── task2_misc/
│   ├── counter_driver.c    # Misc device driver (counter)
│   ├── test_counter.sh     # Shell script kiểm thử counter
│   └── Makefile            # Build module (ARCH=arm, arm-linux-gnueabi-)
├── initramfs_asgn1/
│   └── usr/                # Cây thư mục usr/ cho initramfs (chứa binary đã build)
├── logs/
│   ├── dmesg_full.txt          # Kernel log (dmesg) khi chạy cả hai driver
│   ├── session_both.txt        # Toàn bộ session terminal trong QEMU
│   ├── test_asgn1_output.txt   # Output test_asgn1.sh
│   ├── test_asgn1_output_30pass.txt  # Output test với 30 pass cases
│   └── test_counter_output.txt # Output test_counter.sh
├── bao_cao/
│   └── ASN01_Report.pdf    # Báo cáo (file nhị phân — không mở bằng git)
└── report/                 # Thư mục dự phòng (hiện trống)
```

---

## Yêu cầu

| Công cụ | Ghi chú |
|---|---|
| `arm-linux-gnueabi-gcc` | Cross-compiler (Makefile dùng `CROSS_COMPILE=arm-linux-gnueabi-`) |
| Linux kernel 5.15 headers | `KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15` (theo Makefile) |
| `make` | Build kernel module |
| `qemu-system-arm` | Chạy driver bên trong QEMU (xem Lab 1 để có image) |

---

## Build & Run

**Build kernel module (trên máy host Linux):**

```bash
# Task 1 — ioctl driver
cd assignment_1/task1_ioctl
make        # tạo asgn1_driver.ko
make clean

# Task 2 — misc counter driver
cd assignment_1/task2_misc
make        # tạo counter_driver.ko
make clean
```

> Yêu cầu `KDIR` trỏ đúng vào kernel 5.15 đã build (xem `lab_1/`).

**Chạy trong QEMU (với tư cách root):**

```sh
# Load driver
insmod /lib/modules/5.15.0/asgn1_driver.ko
mknod /dev/asgn1 c 241 0

# Kiểm thử tự động
sh /test_asgn1.sh

# Gỡ driver
rmmod asgn1_driver
```

> Script `test_asgn1.sh` tự xử lý insmod/rmmod, tạo device node, và báo cáo PASS/FAIL.

---

## Kết quả / Output

| File | Nội dung |
|---|---|
| `logs/test_asgn1_output_30pass.txt` | 30 test cases PASS — output đầy đủ |
| `logs/test_counter_output.txt` | Output kiểm thử counter driver |
| `logs/dmesg_full.txt` | Kernel log ghi nhận insmod/rmmod cả hai driver |
| `logs/session_both.txt` | Session terminal đầy đủ (QEMU) |
| `bao_cao/ASN01_Report.pdf` | Báo cáo PDF |

---

## Ghi chú

- `asgn1_driver` có hai mode: **KERNEL mode** (thêm tiền tố `[KERNEL]` vào dữ liệu lưu) và **ECHO mode** (lưu nguyên).
- `read_count` đếm cả lần đọc cuối trả về 0 byte (EOF), nên một lần `cat` một buffer không rỗng sẽ tăng `read_count` lên 2.
- `RESET_BUFFER` (`ioctl`) xóa buffer và các counter đọc/ghi, nhưng **không** xóa `open_count`.

---

[← Về README chính](../README.md)
