# LOS201 — Linux Operating System (Embedded Linux)

Repo lưu bài làm assignments và labs của môn học **LOS201 – Linux Operating System** (lập trình nhân Linux và hệ thống nhúng).

> ⚠️ **Repo đang được cập nhật dần** — một số thư mục chưa hoàn chỉnh hoặc chưa có báo cáo.

---

## Cấu trúc thư mục

```
LOS201/
├── assignment_1/    # Assignment 1: Linux kernel character device driver (ioctl + misc)
├── assignment_2/    # Assignment 2: Sensor Monitoring System (kernel module + userspace app đa luồng)
├── lab_1/           # Lab 1: Cross-compile Linux 5.15 + U-Boot 2022.04 + BusyBox 1.35; boot trên QEMU ARM
├── lab_2/           # Lab 2: Character device driver cross-compiled cho QEMU ARM vexpress-ca9
└── lab_3/           # Lab 3: Debugging & profiling (Valgrind, GDB, perf, strace)
```

---

## Bảng nội dung

| Thư mục | Nội dung | Trạng thái |
|---|---|---|
| [assignment_1](assignment_1/README.md) | Viết character device driver với `ioctl`; thêm misc device (counter); kiểm thử bằng shell script trong QEMU | ✅ Hoàn thành |
| [assignment_2](assignment_2/README.md) | Sensor Monitoring System: kernel module `sms_sensor` + userspace app đa luồng (`pthread`, POSIX `mqueue`, circular buffer); đo hiệu năng bằng `perf`/`valgrind`/`strace` | ✅ Hoàn thành |
| [lab_1](lab_1/README.md) | Cross-compile Linux 5.15 + U-Boot 2022.04 + BusyBox 1.35.0 cho ARM vexpress-v2p-ca9; đóng gói initramfs; boot thử trên QEMU | ✅ Hoàn thành |
| [lab_2](lab_2/README.md) | Viết `lab2_driver` (simple character device, Major 240) cross-compiled với `arm-linux-gnueabihf-`; đóng gói vào initramfs; load vào kernel đã build ở Lab 1 | ✅ Hoàn thành |
| [lab_3](lab_3/README.md) | Debug và profiling ứng dụng nhúng: Valgrind (memory leak), GDB remote debug (ARM), `perf`, `strace`; ba app mẫu có bug | ✅ Hoàn thành |
| `lab_4`, ... | Các lab tiếp theo | 🗓️ Planned |

---

## Yêu cầu môi trường

| Công cụ | Phiên bản / Ghi chú |
|---|---|
| `arm-linux-gnueabi-gcc` | Cross-compiler dùng trong `assignment_1`, `assignment_2` (Makefile dùng `arm-linux-gnueabi-`) |
| `arm-linux-gnueabihf-gcc` | Cross-compiler dùng trong `lab_2` (hard-float ABI; `-march=armv7-a -mtune=cortex-a9`) |
| Linux kernel headers | Phiên bản **5.15** — cần khi build kernel module |
| `make` | Build kernel module và userspace app |
| `qemu-system-arm` | Chạy ảnh ARM vexpress-v2p-ca9 trên máy host |
| BusyBox | **1.35.0** (source trong `lab_1/`) |
| U-Boot | **2022.04** (source trong `lab_1/`) |
| `Valgrind` | Dùng trong Lab 3 và Assignment 2 |
| `GDB` / `gdbserver` | Remote debug ARM trong Lab 3 |
| `perf` | CPU profiling trong Assignment 2 và Lab 3 |
| `strace` | Trace system call trong Assignment 2 và Lab 3 |
| `git-lfs` | Quản lý file nhị phân lớn (`.ko`, `.img`, `.bin`, `*.cpio.gz`) |
| Distro Linux host | `<tên distro và phiên bản>` |

---

## Cài đặt & Clone

```bash
# 1. Cài Git LFS (bắt buộc — repo dùng LFS cho file nhị phân lớn)
git lfs install

# 2. Clone repo
git clone https://github.com/trangiaphuc05022006-eng/LOS201.git
cd LOS201

# 3. Kéo các file LFS về local
git lfs pull
```

> **Lưu ý:** Nếu bỏ qua bước `git lfs install`, các file `.ko`, `*.cpio.gz`, `zImage`, `u-boot` sẽ chỉ là con trỏ LFS và không dùng được.

---

## Quy ước làm việc

| Quy ước | Mô tả |
|---|---|
| **Nhánh chính** | `main` — luôn ổn định, không push trực tiếp |
| **Merge** | Tất cả thay đổi phải qua **Pull Request**; cần review trước khi merge |
| **Đặt tên nhánh** | `<loại>/<mô-tả-ngắn>` — ví dụ: `feat/asgn2-circular-buffer`, `fix/lab2-makefile` |
| **Commit message** | Tiếng Anh, dạng `<type>: <summary>` — ví dụ: `feat: add ioctl handler for RESET`, `docs: update lab_2 README` |

---

## Tác giả

**trangiaphuc05022006-eng**

---

## Lưu ý

> Nội dung trong repo này được tạo ra phục vụ **mục đích học tập** trong khuôn khổ môn học LOS201. Không sao chép để nộp bài.
