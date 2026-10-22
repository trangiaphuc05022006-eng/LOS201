# lab_1 — Build Kernel + U-Boot + initramfs cho QEMU ARM

Cross-compile toàn bộ software stack cho nền tảng ARM vexpress-v2p-ca9: Linux kernel 5.15, U-Boot 2022.04, BusyBox 1.35.0, và đóng gói initramfs để boot trong QEMU.

---

## Mục tiêu

- Cross-compile Linux kernel 5.15 cho ARM Cortex-A9 (vexpress-v2p-ca9).
- Build U-Boot 2022.04 làm bootloader.
- Build BusyBox 1.35.0 để tạo root filesystem tối giản.
- Đóng gói initramfs (`initramfs.cpio.gz`) và boot hệ thống trong `qemu-system-arm`.

---

## Cấu trúc thư mục

```
lab_1/
├── kernel/
│   ├── linux-5.15/          # Kernel source (trong .gitignore — không commit)
│   └── linux-5.15.tar.xz    # Kernel tarball (~116 MB, file nén — không mở)
├── uboot/
│   ├── u-boot-2022.04/      # U-Boot source
│   └── u-boot-2022.04.tar.bz2  # U-Boot tarball (~17 MB, file nén — không mở)
├── busybox-1.35.0/          # BusyBox source (untracked)
├── busybox-1.35.0.tar.bz2   # BusyBox tarball (~2.4 MB, Git LFS)
├── initramfs.cpio.gz        # initramfs đóng gói (~8 MB, Git LFS)
├── rootfs/                  # Root filesystem thủ công (untracked)
│   ├── bin/, sbin/, usr/    # BusyBox symlinks
│   ├── dev/, proc/, sys/    # Mount points
│   ├── etc/                 # Cấu hình cơ bản
│   ├── lib/                 # Thư viện
│   ├── linuxrc              # Init script
│   └── initramfs.cpio.gz    # Placeholder (266 B)
└── output/
    ├── zImage               # Kernel image ARM (~4.9 MB, Git LFS)
    ├── u-boot               # U-Boot binary (~4.2 MB, Git LFS)
    └── vexpress-v2p-ca9.dtb # Device tree blob (~14 KB, Git LFS)
```

> **Lưu ý:** `lab_1/kernel/linux-5.15/` có trong `.gitignore` và không được commit. Các thư mục `busybox-1.35.0/` và `rootfs/` là untracked (lớn).

---

## Yêu cầu

| Công cụ | Ghi chú |
|---|---|
| `arm-linux-gnueabi-gcc` | Cross-compiler cho kernel và U-Boot |
| `make` | Build kernel, U-Boot, BusyBox |
| `qemu-system-arm` | Chạy image ARM |
| `cpio`, `gzip` | Đóng gói initramfs |
| `git-lfs` | Cần thiết để pull file `zImage`, `u-boot`, `*.dtb`, `*.cpio.gz` |

---

## Build & Run

**Giải nén source (nếu chưa có thư mục):**

```bash
tar -xf lab_1/kernel/linux-5.15.tar.xz   -C lab_1/kernel/
tar -xf lab_1/uboot/u-boot-2022.04.tar.bz2 -C lab_1/uboot/
tar -xf lab_1/busybox-1.35.0.tar.bz2
```

**Cross-compile Linux kernel:**

```bash
# <lệnh build kernel — xem config và script trong lab_1/kernel/ nếu có>
```

> ⚠️ Chưa tìm thấy build script hoặc `.config` được lưu trong thư mục này. Xem báo cáo hoặc hỏi tác giả để biết defconfig đã dùng.

**Boot trong QEMU:**

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -kernel lab_1/output/zImage \
  -dtb lab_1/output/vexpress-v2p-ca9.dtb \
  -initrd lab_1/initramfs.cpio.gz \
  -append "console=ttyAMA0 root=/dev/ram rdinit=/linuxrc" \
  -nographic
```

> Lệnh QEMU trên là suy luận từ tên file artifact. Kiểm tra lại với log hoặc script thực tế nếu có.

---

## Kết quả / Output

| File | Nội dung |
|---|---|
| `output/zImage` | Kernel image ARM đã compile (Git LFS) |
| `output/u-boot` | U-Boot bootloader binary (Git LFS) |
| `output/vexpress-v2p-ca9.dtb` | Device tree cho vexpress-v2p-ca9 (Git LFS) |
| `initramfs.cpio.gz` | Root filesystem đóng gói (Git LFS) |

---

## Ghi chú

- Thư mục `lab_1/kernel/linux-5.15/` bị loại khỏi git (`.gitignore`) vì quá lớn. Cần giải nén từ tarball trước khi build.
- `busybox-1.35.0/` và `rootfs/` là untracked — không bị commit vào repo.
- Các file nhị phân trong `output/` được quản lý bởi Git LFS (theo `.gitattributes`).

---

[← Về README chính](../README.md)
