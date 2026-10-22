# assignment_2 — Sensor Monitoring System (SMS)

A complete Sensor Monitoring System consisting of: a kernel module that simulates a sensor device (`sms_sensor`), a multi-threaded userspace application that reads and processes sensor data, and system performance measurements.

---

## Objectives

- Write a Linux kernel module as a misc device that simulates a temperature/humidity sensor (`sms_sensor_driver`).
- Build a multi-threaded userspace application using POSIX `pthread`, POSIX message queues (`mqueue`), and a circular buffer.
- Measure and analyze system performance using `perf`, `strace`, and `valgrind`.

---

## Directory Structure

```
assignment_2/
├── driver/
│   ├── sms_sensor_driver.c  # Kernel module: misc device, generates random sensor data (linux/random.h)
│   ├── sms_sensor.h         # Header: ioctl command and sensor data struct
│   ├── test_ioctl.c         # Userspace program for ioctl testing
│   ├── Makefile             # Build script (ARCH=arm, arm-linux-gnueabi-, KDIR=~/embedded_lab1/kernel/linux-5.15)
│   └── lab2_driver.ko       # Build artifact — binary file, do not open
├── app/
│   ├── sms_app.c            # Main program: thread management, signal handler, CLI options
│   ├── sms_common.h         # Shared constants and data types
│   ├── sensor_thread.c/h    # Thread that reads data from /dev/sms_sensor
│   ├── processor_thread.c/h # Thread that processes data and computes jitter
│   ├── logger_thread.c/h    # Thread that writes logs to file/stdout
│   ├── alert_thread.c/h     # Thread that raises alerts when thresholds are exceeded
│   ├── circular_buffer.c/h  # Thread-safe circular buffer data structure
│   ├── jitter.c/h           # Measures inter-read latency (jitter)
│   └── test_cb.c            # Unit tests for the circular buffer
├── rootfs/
│   └── etc/                 # System configuration (mounted in QEMU)
├── tools/
│   ├── fake_sensor.sh       # Script to simulate fake sensor data
│   └── measure.sh           # Script to snapshot VmRSS/smaps of sms_app over 60 seconds
├── logs/
│   ├── perf_sms.txt             # perf stat results for sms_app
│   ├── perf_sms_callgraph.txt   # Call graph from perf report (large file)
│   ├── strace_sms.txt           # System call trace of sms_app
│   └── valgrind_sms.txt         # Memory leak check with Valgrind
└── report/
    └── Group2_Assignment2.pdf   # Group report (binary — do not open via git)
```

---

## Requirements

| Tool | Notes |
|---|---|
| `arm-linux-gnueabi-gcc` | Cross-compiler for the kernel module and userspace app |
| Linux kernel 5.15 headers | `KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15` (per Makefile) |
| `make` | Build module and app |
| `qemu-system-arm` | Run the system in QEMU (use the image from `lab_1/`) |
| `perf` | CPU profiling |
| `strace` | System call tracing |
| `Valgrind` | Memory leak detection |

---

## Build & Run

**Build kernel module:**

```bash
cd assignment_2/driver
make        # produces sms_sensor_driver.ko
make clean
```

**Build userspace app:**

```bash
# <build command for userspace app — check for a Makefile or build script>
```

> ⚠️ No Makefile was found for `app/`. See `sms_app.c` for manual compilation; required flags include `-lpthread -lm -lrt`.

**Run inside QEMU (as root):**

```sh
# Load driver
insmod sms_sensor_driver.ko

# Run app (e.g., for 60 seconds)
sms_app -d 60

# Measure memory in parallel
sh /measure.sh
```

**Performance measurement:**

```sh
# perf
perf stat ./sms_app -d 30

# strace
strace -o strace_sms.txt ./sms_app -d 10

# valgrind
valgrind --leak-check=full ./sms_app -d 5
```

---

## Results / Output

| File | Content |
|---|---|
| `logs/perf_sms.txt` | CPU cycles, instructions, and cache miss statistics for sms_app |
| `logs/perf_sms_callgraph.txt` | Detailed call graph (perf report) |
| `logs/strace_sms.txt` | System call list |
| `logs/valgrind_sms.txt` | Valgrind memory leak report |
| `report/Group2_Assignment2.pdf` | Group report (PDF) |

---

## Notes

- `sms_sensor_driver` uses `linux/random.h` to generate random data and `linux/ktime.h` for timestamps.
- `measure.sh` reads `/proc/$PID/smaps_rollup` when available, falling back to `/proc/$PID/smaps` on older kernels.
- The circular buffer requires thread safety — see `circular_buffer.c` for the locking mechanism.

---

[← Back to main README](../README.md)
