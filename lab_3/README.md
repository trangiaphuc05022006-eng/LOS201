# lab_3 — Debugging & Profiling ứng dụng nhúng

Sử dụng các công cụ debugging và profiling trên môi trường Linux nhúng ARM: Valgrind (memory leak), GDB remote debug, `perf` (CPU profiling), `strace` (system call tracing). Ba ứng dụng C mẫu có lỗi cố ý được dùng làm target.

---

## Mục tiêu

- Phát hiện và phân tích memory leak bằng Valgrind.
- Debug ứng dụng ARM cross-compiled bằng GDB + gdbserver (remote).
- Profile CPU hotspot bằng `perf record` / `perf report`.
- Trace system call bằng `strace`.
- Đo và vẽ biểu đồ latency histogram bằng gnuplot.

---

## Cấu trúc thư mục

```
lab_3/
├── apps/
│   ├── buggy_app.c          # App có 3 lỗi cố ý: out-of-bounds, use-after-free, integer overflow
│   ├── buggy_app            # Binary x86 (đã build)
│   ├── leaky_app.c          # App có memory leak (name, scores không được free)
│   ├── leaky_app            # Binary x86
│   ├── leaky_app_arm        # Binary ARM cross-compiled (~478 KB)
│   ├── leaky_app_fixed      # Binary x86 (đã sửa leak)
│   ├── leaky_rss.c          # Biến thể đo RSS memory growth
│   ├── leaky_rss_arm        # Binary ARM cross-compiled
│   ├── leaky_rss_fixed_arm  # Binary ARM (đã sửa)
│   ├── perf_target.c        # App profiling: heavy_compute (sqrt+log), linear_search
│   ├── perf_target          # Binary x86
│   ├── perf_target_arm      # Binary ARM cross-compiled (~484 KB)
│   └── LOS201/              # Thư mục phụ (chứa Lab_3/)
├── rt_kernel/               # RT kernel (hiện trống)
├── rootfs/
│   └── initramfs/           # Root filesystem cho Lab 3
├── output/
│   └── initramfs_lab3.cpio.gz  # initramfs đóng gói (~8.9 MB, Git LFS)
├── logs/
│   ├── gdb_session.txt          # GDB remote debug session (tóm tắt)
│   ├── gdb_session_full.txt     # GDB session đầy đủ
│   ├── valgrind_report.txt      # Báo cáo Valgrind chi tiết (leak check)
│   ├── valgrind_fixed.txt       # Valgrind sau khi sửa leak
│   ├── perf.data                # Raw perf data (~60 KB, file nhị phân — không mở)
│   ├── perf.data.old            # perf data session trước (~60 KB)
│   ├── perf_report.txt          # Kết quả perf report (text)
│   ├── perf_stat.txt            # Kết quả perf stat
│   ├── strace_arm.txt           # strace trên ARM (qua QEMU)
│   ├── strace_full.txt          # strace đầy đủ trên host
│   ├── strace_summary.txt       # Tóm tắt system call count
│   ├── hist_standard.txt        # Latency histogram (dữ liệu thô)
│   ├── latency_histogram.png    # Biểu đồ latency (file ảnh — không mở bằng git)
│   ├── plot_latency.gnuplot     # Script gnuplot vẽ biểu đồ
│   ├── qemu_session1.txt        # Session QEMU lần 1
│   └── qemu_session2.txt        # Session QEMU lần 2
└── submit/
    └── SE205180_TranNguyenVietDung_Lab03/
        ├── apps/                # Bản app nộp chính thức
        ├── bao_cao/             # Báo cáo
        ├── logs/                # Log nộp chính thức
        └── output/              # Output nộp chính thức
```

---

## Yêu cầu

| Công cụ | Ghi chú |
|---|---|
| `arm-linux-gnueabi-gcc` hoặc `arm-linux-gnueabihf-gcc` | Cross-compile app cho ARM |
| `Valgrind` | Chạy trên host hoặc trong QEMU |
| `GDB` + `gdbserver` | Remote debug — gdbserver chạy trong QEMU, GDB chạy trên host |
| `perf` | CPU profiling (cần `linux-tools-$(uname -r)` hoặc build từ kernel source) |
| `strace` | System call tracing |
| `gnuplot` | Vẽ latency histogram (xem `logs/plot_latency.gnuplot`) |
| `qemu-system-arm` | Chạy binary ARM và gdbserver |
| `git-lfs` | Cần để pull `initramfs_lab3.cpio.gz` |

---

## Build & Run

**Build app trên host (x86):**

```bash
cd lab_3/apps
gcc -g -o buggy_app buggy_app.c
gcc -g -o leaky_app leaky_app.c
gcc -g -o perf_target perf_target.c -lm
```

**Cross-compile cho ARM:**

```bash
arm-linux-gnueabi-gcc -g -o leaky_app_arm leaky_app.c
arm-linux-gnueabi-gcc -g -o perf_target_arm perf_target.c -lm
```

**Valgrind (memory leak):**

```bash
valgrind --leak-check=full --track-origins=yes ./leaky_app
```

**perf (CPU profiling):**

```bash
perf record -g ./perf_target
perf report     # xem kết quả (xem logs/perf_report.txt để có output mẫu)
perf stat ./perf_target
```

**strace (system call trace):**

```bash
strace -c ./leaky_app         # tóm tắt
strace -o strace_full.txt ./leaky_app  # chi tiết
```

**GDB remote debug (qua QEMU):**

```bash
# Trong QEMU (gdbserver)
gdbserver :1234 /leaky_app_arm

# Trên host (GDB)
arm-linux-gnueabi-gdb leaky_app_arm
(gdb) target remote :1234
```

> Xem `logs/gdb_session.txt` và `logs/gdb_session_full.txt` để biết chi tiết session thực tế.

**Vẽ latency histogram:**

```bash
gnuplot logs/plot_latency.gnuplot
```

---

## Kết quả / Output

| File | Nội dung |
|---|---|
| `logs/valgrind_report.txt` | Phát hiện leak trong `leaky_app` (name, scores không free) |
| `logs/valgrind_fixed.txt` | Valgrind clean sau khi sửa |
| `logs/perf_report.txt` | Overhead: `__ieee754_log_fma` 48%, `heavy_compute` 37% |
| `logs/perf_stat.txt` | CPU cycles, instructions, cache miss |
| `logs/strace_summary.txt` | Thống kê system call count |
| `logs/gdb_session_full.txt` | GDB remote session đầy đủ |
| `logs/latency_histogram.png` | Biểu đồ phân phối latency (PNG) |
| `output/initramfs_lab3.cpio.gz` | initramfs (~8.9 MB, Git LFS) |

---

## Ghi chú

- `perf_target.c`: hàm `heavy_compute` dùng `sqrt()` + `log()` với 100 000 phần tử × 50 vòng lặp — chiếm phần lớn CPU time theo `perf_report.txt`.
- `buggy_app.c` có 3 lỗi cố ý (out-of-bounds, use-after-free, integer overflow) để thực hành debug.
- `leaky_app.c` cố ý không free `name` và `scores` trong `Record` để thực hành Valgrind.
- `rt_kernel/` hiện trống — có thể liên quan đến RT-preempt patch, chưa có nội dung.

---

[← Về README chính](../README.md)
