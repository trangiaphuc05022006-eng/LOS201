# Lab 01 — Booting an Embedded Linux System on ARMv7

Set up a minimal ARM Linux boot stack using **Linux kernel 5.15**, **U-Boot 2022.04**, **BusyBox 1.35.0**, an `initramfs`, and the QEMU `vexpress-a9` machine (Cortex-A9).

## Course information

| Item | Details |
|---|---|
| **Course** | LOS201 |
| **Class** | IC2008 |

## Team members

| No. | Full name | Student ID |
|---:|---|---|
| 1 | Trần Gia Phúc | SE204417 |
| 2 | Trần Hoàng Khôi | SE203155 |
| 3 | Trần Nguyễn Việt Dũng | SE205180 |
| 4 | Thân Trọng Khang | SE203290 |
| 5 | Nguyễn Huy Khánh | SE204207 |

## Objectives

1. Configure and cross-compile an ARM Linux kernel and device tree.
2. Explore U-Boot as the bootloader and the role of kernel boot arguments.
3. Build a minimal BusyBox-based root filesystem.
4. Package that filesystem as an initramfs and boot it in QEMU.
5. Understand the transition from the bootloader/kernel to the first userspace process.

## Checked-in artifacts

```text
lab_1/
├── README.md
├── configs/
│   ├── kernel.config           # Saved Linux kernel configuration
│   └── busybox.config          # Saved BusyBox configuration
├── busybox-1.35.0.tar.bz2      # BusyBox source archive
├── initramfs.cpio.gz           # Packaged root filesystem
├── output/
│   ├── zImage                 # ARM kernel image
│   ├── vexpress-v2p-ca9.dtb   # Device tree
│   ├── u-boot                 # Bootloader executable
│   └── initramfs.cpio.gz     # Another checked-in initramfs copy
└── uboot/
    ├── .config                # U-Boot configuration
    ├── u-boot                 # Built executable
    ├── u-boot.bin             # Bootloader binary
    ├── u-boot-2022.04.tar.bz2 # Source archive
    └── u-boot-2022.04/       # Checked-in U-Boot source tree
```

**Not in this GitHub snapshot:** the Linux 5.15 source/build tree and the unpacked BusyBox source/rootfs working directories. Do not assume that cloning the repository alone is enough to rebuild these components.

## Requirements

On a Linux host, install the ARM cross-toolchain, `qemu-system-arm`, `make`, `cpio`, `gzip`, and the libraries needed to build Linux/U-Boot/BusyBox. The course instructions use the ARM hard-float compiler prefix `arm-linux-gnueabihf-` for the Cortex-A9 target.

## Boot the prebuilt image

From the **LOS201 repository root**:

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -cpu cortex-a9 \
  -m 512M \
  -nographic \
  -kernel lab_1/output/zImage \
  -dtb lab_1/output/vexpress-v2p-ca9.dtb \
  -initrd lab_1/output/initramfs.cpio.gz \
  -append 'console=ttyAMA0,115200 root=/dev/ram0 rw rdinit=/linuxrc'
```

This is a **documented boot example**, not an independently re-executed test. The `rdinit` path must match the executable init entry point inside the chosen archive. If your rootfs uses `/init` or `/sbin/init` instead, adjust the boot command to match. To exit QEMU in `-nographic` mode, press **Ctrl+A**, then **X**.

## Rebuild outline

Retrieve the Linux 5.15 source separately (it is not committed), then restore the versioned configuration:

```bash
# Run within your extracted linux-5.15 source directory
cp /path/to/LOS201/lab_1/configs/kernel.config .config
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- olddefconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j"$(nproc)" zImage dtbs
```

To rebuild BusyBox, extract `busybox-1.35.0.tar.bz2`, load `configs/busybox.config` as `.config`, use the same compatible ARM toolchain, and follow the course lab's steps for creating the rootfs and init scripts. U-Boot has its own source archive and saved `.config` in `uboot/`; its build is distinct from booting the kernel directly with QEMU's `-kernel` option.

## Results and verification

The repository contains the resulting `zImage`, `vexpress-v2p-ca9.dtb`, `u-boot`, and `initramfs.cpio.gz`, together with configuration snapshots. These artifacts document the build output; no Lab 01 terminal boot log is included here, so the boot sequence is not claimed to have been independently reproduced as part of this README review.

Useful commands after entering the Linux shell include `uname -a`, `cat /proc/cmdline`, `mount`, and `dmesg`.

## Technical takeaways

- **Device Tree:** describes the emulated hardware to the ARM kernel.
- **Boot arguments:** select the serial console and userspace initialization path.
- **BusyBox/initramfs:** provide a compact early userspace environment.
- **Cross-compilation:** produces ARM executables on a non-ARM Linux host.

[← Back to repository](../README.md)
