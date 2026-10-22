# lab_1 — Build Kernel + U-Boot + initramfs for QEMU ARM

Cross-compile the full software stack for the ARM vexpress-v2p-ca9 platform: Linux kernel 5.15, U-Boot 2022.04, BusyBox 1.35.0, and package an initramfs to boot in QEMU.

---

## Objectives

- Cross-compile Linux kernel 5.15 for ARM Cortex-A9 (vexpress-v2p-ca9).
- Build U-Boot 2022.04 as the bootloader.
- Build BusyBox 1.35.0 to create a minimal root filesystem.
- Package an initramfs (`initramfs.cpio.gz`) and boot the system in `qemu-system-arm`.

---

## Directory Structure

```
lab_1/
├── kernel/
│   ├── linux-5.15/          # Kernel source (excluded by .gitignore — not committed)
│   └── linux-5.15.tar.xz    # Kernel tarball (~116 MB, archive — do not open)
├── uboot/
│   ├── u-boot-2022.04/      # U-Boot source
│   └── u-boot-2022.04.tar.bz2  # U-Boot tarball (~17 MB, archive — do not open)
├── busybox-1.35.0/          # BusyBox source (untracked)
├── busybox-1.35.0.tar.bz2   # BusyBox tarball (~2.4 MB, Git LFS)
├── initramfs.cpio.gz        # Packaged initramfs (~8 MB, Git LFS)
├── rootfs/                  # Manually constructed root filesystem (untracked)
│   ├── bin/, sbin/, usr/    # BusyBox symlinks
│   ├── dev/, proc/, sys/    # Mount points
│   ├── etc/                 # Basic configuration
│   ├── lib/                 # Libraries
│   ├── linuxrc              # Init script
│   └── initramfs.cpio.gz    # Placeholder (266 B)
└── output/
    ├── zImage               # ARM kernel image (~4.9 MB, Git LFS)
    ├── u-boot               # U-Boot binary (~4.2 MB, Git LFS)
    └── vexpress-v2p-ca9.dtb # Device tree blob (~14 KB, Git LFS)
```

> **Note:** `lab_1/kernel/linux-5.15/` is listed in `.gitignore` and is not committed. The `busybox-1.35.0/` and `rootfs/` directories are untracked (large).

---

## Requirements

| Tool | Notes |
|---|---|
| `arm-linux-gnueabi-gcc` | Cross-compiler for kernel and U-Boot |
| `make` | Build kernel, U-Boot, and BusyBox |
| `qemu-system-arm` | Run the ARM image |
| `cpio`, `gzip` | Package the initramfs |
| `git-lfs` | Required to pull `zImage`, `u-boot`, `*.dtb`, and `*.cpio.gz` |

---

## Build & Run

**Extract sources (if directories do not exist yet):**

```bash
tar -xf lab_1/kernel/linux-5.15.tar.xz   -C lab_1/kernel/
tar -xf lab_1/uboot/u-boot-2022.04.tar.bz2 -C lab_1/uboot/
tar -xf lab_1/busybox-1.35.0.tar.bz2
```

**Cross-compile Linux kernel:**

```bash
# <kernel build command — check for a config or build script in lab_1/kernel/>
```

> ⚠️ No build script or saved `.config` was found in this directory. Refer to the course report or contact the author to obtain the defconfig used.

**Boot in QEMU:**

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -kernel lab_1/output/zImage \
  -dtb lab_1/output/vexpress-v2p-ca9.dtb \
  -initrd lab_1/initramfs.cpio.gz \
  -append "console=ttyAMA0 root=/dev/ram rdinit=/linuxrc" \
  -nographic
```

> The QEMU command above is inferred from the artifact file names. Verify against actual logs or scripts if available.

---

## Results / Output

| File | Content |
|---|---|
| `output/zImage` | Compiled ARM kernel image (Git LFS) |
| `output/u-boot` | U-Boot bootloader binary (Git LFS) |
| `output/vexpress-v2p-ca9.dtb` | Device tree for vexpress-v2p-ca9 (Git LFS) |
| `initramfs.cpio.gz` | Packaged root filesystem (Git LFS) |

---

## Notes

- `lab_1/kernel/linux-5.15/` is excluded from git (`.gitignore`) due to its size. Extract it from the tarball before building.
- `busybox-1.35.0/` and `rootfs/` are untracked — they are never committed to the repository.
- Binary files in `output/` are managed by Git LFS (as specified in `.gitattributes`).

---

[← Back to main README](../README.md)
