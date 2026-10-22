# LOS201 — Linux Operating System (Embedded Linux)

This repository contains coursework assignments and labs for **LOS201 – Linux Operating System** (Linux kernel programming and embedded systems).

> ⚠️ **This repository is updated incrementally** — some folders may be incomplete or missing reports.

---

## Directory Structure

```
LOS201/
├── assignment_1/    # Assignment 1: Linux kernel character device driver (ioctl + misc)
├── assignment_2/    # Assignment 2: Sensor Monitoring System (kernel module + multi-threaded userspace app)
├── lab_1/           # Lab 1: Cross-compile Linux 5.15 + U-Boot 2022.04 + BusyBox 1.35; boot on QEMU ARM
├── lab_2/           # Lab 2: Character device driver cross-compiled for QEMU ARM vexpress-ca9
└── lab_3/           # Lab 3: Debugging & profiling (Valgrind, GDB, perf, strace)
```

---

## Table of Contents

| Folder | Content | Status |
|---|---|---|
| [assignment_1](assignment_1/README.md) | Character device driver with `ioctl`; misc device (counter); functional testing via shell scripts in QEMU | Done |
| [assignment_2](assignment_2/README.md) | Sensor Monitoring System: `sms_sensor` kernel module + multi-threaded userspace app (`pthread`, POSIX `mqueue`, circular buffer); performance analysis with `perf`/`valgrind`/`strace` | Done |
| [lab_1](lab_1/README.md) | Cross-compile Linux 5.15 + U-Boot 2022.04 + BusyBox 1.35.0 for ARM vexpress-v2p-ca9; package initramfs; boot on QEMU | Done |
| [lab_2](lab_2/README.md) | `lab2_driver` (simple character device, Major 240) cross-compiled with `arm-linux-gnueabihf-`; packaged into initramfs; loaded on the kernel built in Lab 1 | Done |
| [lab_3](lab_3/README.md) | Embedded application debugging and profiling: Valgrind (memory leak), GDB remote debug (ARM), `perf`, `strace`; three intentionally buggy sample apps | Done |
| `lab_4`, ... | Upcoming labs | Planned |

---

## Requirements

| Tool | Version / Notes |
|---|---|
| `arm-linux-gnueabi-gcc` | Cross-compiler used in `assignment_1` and `assignment_2` (Makefile uses `arm-linux-gnueabi-`) |
| `arm-linux-gnueabihf-gcc` | Cross-compiler used in `lab_2` (hard-float ABI; `-march=armv7-a -mtune=cortex-a9`) |
| Linux kernel headers | Version **5.15** — required when building kernel modules |
| `make` | Build kernel modules and userspace apps |
| `qemu-system-arm` | Run ARM vexpress-v2p-ca9 images on the host machine |
| BusyBox | **1.35.0** (source in `lab_1/`) |
| U-Boot | **2022.04** (source in `lab_1/`) |
| `Valgrind` | Used in Lab 3 and Assignment 2 |
| `GDB` / `gdbserver` | Remote ARM debugging in Lab 3 |
| `perf` | CPU profiling in Assignment 2 and Lab 3 |
| `strace` | System call tracing in Assignment 2 and Lab 3 |
| `git-lfs` | Manages large binary files (`.ko`, `.img`, `.bin`, `*.cpio.gz`) |

---

## Installation & Clone

```bash
# 1. Install Git LFS (required — this repo uses LFS for large binary files)
git lfs install

# 2. Clone the repository
git clone https://github.com/trangiaphuc05022006-eng/LOS201.git
cd LOS201

# 3. Pull LFS files to local
git lfs pull
```

> **Note:** If you skip `git lfs install`, files such as `.ko`, `*.cpio.gz`, `zImage`, and `u-boot` will only be LFS pointer files and will not be usable.

---

## Working Conventions

| Convention | Description |
|---|---|
| **Main branch** | `main` — always stable; do not push directly |
| **Merge** | All changes must go through a **Pull Request**; review required before merging |
| **Branch naming** | `<type>/<short-description>` — e.g., `feat/asgn2-circular-buffer`, `fix/lab2-makefile` |
| **Commit message** | English, format `<type>: <summary>` — e.g., `feat: add ioctl handler for RESET`, `docs: update lab_2 README` |

---

## Author

**trangiaphuc05022006-eng**

---

## Notice

> The content in this repository is produced for **educational purposes** within the LOS201 course. Do not copy for academic submission.
