# assignment_1 — Linux Kernel Device Driver (ioctl + misc)

Two Linux kernel modules for ARM: a character device driver with `ioctl` interface, and a misc device driver (counter). Built and tested inside a QEMU ARM environment.

---

## Objectives

- Understand the Linux kernel character device driver mechanism (register, file_operations, ioctl).
- Implement custom `ioctl` commands: `RESET_BUFFER`, `GET_STATS`, `SET_MODE`, `GET_VERSION`.
- Write a simple misc device driver (counter).
- Test drivers via shell scripts inside QEMU (insmod, mknod, read/write through `/dev`).

---

## Directory Structure

```
assignment_1/
├── task1_ioctl/
│   ├── asgn1_driver.c      # Character device driver (Major 241, 1024 B buffer, two modes)
│   ├── asgn1_ioctl.h       # Header: magic number, struct asgn1_stats, 4 ioctl commands
│   ├── asgn1_ctl.c         # Userspace control tool source
│   ├── asgn1_ctl           # Userspace control tool binary (pre-built, ARM)
│   ├── asgn1_driver.ko     # Pre-built kernel module (Git LFS — binary file)
│   ├── check               # Binary test helper (ARM)
│   ├── check_output.txt    # Sample output from check
│   ├── test_asgn1.sh       # Functional test shell script (runs inside QEMU)
│   ├── Makefile            # Build script (ARCH=arm, arm-linux-gnueabi-)
│   └── .vscode/            # Editor settings (not related to build)
├── task2_misc/
│   ├── counter_driver.c    # Misc device driver (counter)
│   ├── test_counter.sh     # Shell script for counter driver testing
│   └── Makefile            # Build script (ARCH=arm, arm-linux-gnueabi-)
├── initramfs_asgn1/
│   └── usr/                # usr/ directory tree for initramfs (contains pre-built binaries)
├── logs/
│   ├── dmesg_full.txt          # Kernel log (dmesg) when running both drivers
│   ├── session_both.txt        # Full terminal session inside QEMU
│   ├── test_asgn1_output.txt   # Output of test_asgn1.sh
│   ├── test_asgn1_output_30pass.txt  # Output with 30 pass cases
│   └── test_counter_output.txt # Output of test_counter.sh
├── bao_cao/
│   └── ASN01_Report.pdf    # Assignment report (binary — do not open via git)
└── report/                 # Reserved folder (currently empty)
```

---

## Requirements

| Tool | Notes |
|---|---|
| `arm-linux-gnueabi-gcc` | Cross-compiler (`CROSS_COMPILE=arm-linux-gnueabi-` in Makefile) |
| Linux kernel 5.15 headers | `KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15` (per Makefile) |
| `make` | Build kernel modules |
| `qemu-system-arm` | Run the driver inside QEMU (see Lab 1 for the boot image) |

---

## Build & Run

**Build kernel modules (on the Linux host):**

```bash
# Task 1 — ioctl driver
cd assignment_1/task1_ioctl
make        # produces asgn1_driver.ko
make clean

# Task 2 — misc counter driver
cd assignment_1/task2_misc
make        # produces counter_driver.ko
make clean
```

> `KDIR` must point to the built Linux 5.15 kernel tree (see `lab_1/`).

**Run inside QEMU (as root):**

```sh
# Load driver
insmod /lib/modules/5.15.0/asgn1_driver.ko
mknod /dev/asgn1 c 241 0

# Run automated tests
sh /test_asgn1.sh

# Unload driver
rmmod asgn1_driver
```

> `test_asgn1.sh` handles insmod/rmmod, creates the device node, and reports PASS/FAIL for each test case.

---

## Results / Output

| File | Content |
|---|---|
| `logs/test_asgn1_output_30pass.txt` | Full output — all 30 test cases passed |
| `logs/test_counter_output.txt` | Counter driver test output |
| `logs/dmesg_full.txt` | Kernel log capturing insmod/rmmod of both drivers |
| `logs/session_both.txt` | Complete terminal session (QEMU) |
| `bao_cao/ASN01_Report.pdf` | Assignment report (PDF) |

---

## Notes

- `asgn1_driver` has two modes: **KERNEL mode** (prepends `[KERNEL]` to stored data) and **ECHO mode** (stores data unchanged).
- `read_count` also counts the final read that returns 0 bytes (EOF), so a single `cat` on a non-empty buffer increments `read_count` by 2.
- `RESET_BUFFER` (`ioctl`) clears the buffer and the read/write counters, but does **not** reset `open_count`.

---

[← Back to main README](../README.md)
