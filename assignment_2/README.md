# assignment_2 — Sensor Monitoring System (SMS)

Xây dựng một Sensor Monitoring System hoàn chỉnh gồm: kernel module mô phỏng cảm biến (`sms_sensor`), userspace application đa luồng đọc và xử lý dữ liệu cảm biến, và đo lường hiệu năng hệ thống.

---

## Mục tiêu

- Viết Linux kernel module dạng misc device mô phỏng cảm biến nhiệt độ/độ ẩm (`sms_sensor_driver`).
- Xây dựng userspace app đa luồng sử dụng POSIX `pthread`, POSIX message queue (`mqueue`), và circular buffer.
- Đo và phân tích hiệu năng hệ thống bằng `perf`, `strace`, `valgrind`.

---

## Cấu trúc thư mục

```
assignment_2/
├── driver/
│   ├── sms_sensor_driver.c  # Kernel module: misc device, sinh dữ liệu cảm biến ngẫu nhiên (linux/random.h)
│   ├── sms_sensor.h         # Header: ioctl command và struct dữ liệu cảm biến
│   ├── test_ioctl.c         # Chương trình userspace kiểm thử ioctl
│   ├── Makefile             # Build module (ARCH=arm, arm-linux-gnueabi-, KDIR=~/embedded_lab1/kernel/linux-5.15)
│   └── lab2_driver.ko       # (Artifact build — file nhị phân, không mở)
├── app/
│   ├── sms_app.c            # Chương trình chính: quản lý luồng, signal handler, CLI options
│   ├── sms_common.h         # Hằng số, kiểu dữ liệu dùng chung
│   ├── sensor_thread.c/h    # Luồng đọc dữ liệu từ /dev/sms_sensor
│   ├── processor_thread.c/h # Luồng xử lý dữ liệu và tính jitter
│   ├── logger_thread.c/h    # Luồng ghi log ra file/stdout
│   ├── alert_thread.c/h     # Luồng phát cảnh báo khi vượt ngưỡng
│   ├── circular_buffer.c/h  # Cấu trúc dữ liệu circular buffer (thread-safe)
│   ├── jitter.c/h           # Đo độ trễ (jitter) giữa các lần đọc cảm biến
│   └── test_cb.c            # Unit test cho circular buffer
├── rootfs/
│   └── etc/                 # Cấu hình hệ thống (mount trong QEMU)
├── tools/
│   ├── fake_sensor.sh       # Script mô phỏng dữ liệu cảm biến giả
│   └── measure.sh           # Script đo VmRSS/smaps của sms_app trong 60 giây
├── logs/
│   ├── perf_sms.txt             # Kết quả perf stat cho sms_app
│   ├── perf_sms_callgraph.txt   # Callgraph từ perf report (file lớn)
│   ├── strace_sms.txt           # System call trace của sms_app
│   └── valgrind_sms.txt         # Kiểm tra memory leak với Valgrind
└── report/
    └── Group2_Assignment2.pdf   # Báo cáo nhóm (file nhị phân — không mở)
```

---

## Yêu cầu

| Công cụ | Ghi chú |
|---|---|
| `arm-linux-gnueabi-gcc` | Cross-compiler cho kernel module và userspace app |
| Linux kernel 5.15 headers | `KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15` (theo Makefile) |
| `make` | Build module và app |
| `qemu-system-arm` | Chạy hệ thống trong QEMU (dùng image từ `lab_1/`) |
| `perf` | CPU profiling |
| `strace` | System call tracing |
| `Valgrind` | Memory leak detection |

---

## Build & Run

**Build kernel module:**

```bash
cd assignment_2/driver
make        # tạo sms_sensor_driver.ko
make clean
```

**Build userspace app:**

```bash
# <lệnh build userspace app — xem Makefile hoặc script nếu có>
```

> ⚠️ Chưa tìm thấy Makefile cho `app/`. Xem `sms_app.c` để biết cách biên dịch thủ công (cần `-lpthread -lm -lrt`).

**Chạy trong QEMU (với tư cách root):**

```sh
# Load driver
insmod sms_sensor_driver.ko

# Chạy app (ví dụ 60 giây)
sms_app -d 60

# Đo bộ nhớ song song
sh /measure.sh
```

**Đo hiệu năng:**

```sh
# perf
perf stat ./sms_app -d 30

# strace
strace -o strace_sms.txt ./sms_app -d 10

# valgrind
valgrind --leak-check=full ./sms_app -d 5
```

---

## Kết quả / Output

| File | Nội dung |
|---|---|
| `logs/perf_sms.txt` | Thống kê CPU cycles, instructions, cache miss của sms_app |
| `logs/perf_sms_callgraph.txt` | Callgraph chi tiết (perf report) |
| `logs/strace_sms.txt` | Danh sách system call |
| `logs/valgrind_sms.txt` | Báo cáo memory leak (Valgrind) |
| `report/Group2_Assignment2.pdf` | Báo cáo nhóm (PDF) |

---

## Ghi chú

- `sms_sensor_driver` dùng `linux/random.h` để sinh dữ liệu ngẫu nhiên và `linux/ktime.h` để timestamp.
- `measure.sh` đọc `/proc/$PID/smaps_rollup` khi có, fallback sang `/proc/$PID/smaps` nếu kernel không hỗ trợ.
- Circular buffer cần thread-safe — kiểm tra `circular_buffer.c` để biết cơ chế lock.

---

[← Về README chính](../README.md)
