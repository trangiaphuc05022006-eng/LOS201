# lab_3 — Debugging & Profiling for Embedded Applications

Apply debugging and profiling tools in an embedded Linux ARM environment: Valgrind (memory leak detection), GDB remote debugging, `perf` (CPU profiling), and `strace` (system call tracing). Three intentionally buggy C applications serve as targets.

---

## Objectives

- Detect and analyze memory leaks using Valgrind.
- Debug an ARM cross-compiled application with GDB + gdbserver (remote).
- Profile CPU hotspots using `perf record` / `perf report`.
- Trace system calls using `strace`.
- Measure and plot a latency histogram using gnuplot.

---

## Directory Structure

```
lab_3/
├── apps/
│   ├── buggy_app.c          # App with 3 intentional bugs: out-of-bounds, use-after-free, integer overflow
│   ├── buggy_app            # x86 binary (pre-built)
│   ├── leaky_app.c          # App with memory leaks (name, scores not freed)
│   ├── leaky_app            # x86 binary
│   ├── leaky_app_arm        # ARM cross-compiled binary (~478 KB)
│   ├── leaky_app_fixed      # x86 binary (leaks fixed)
│   ├── leaky_rss.c          # Variant that measures RSS memory growth
│   ├── leaky_rss_arm        # ARM cross-compiled binary
│   ├── leaky_rss_fixed_arm  # ARM binary (fixed)
│   ├── perf_target.c        # Profiling target app: heavy_compute (sqrt+log), linear_search
│   ├── perf_target          # x86 binary
│   ├── perf_target_arm      # ARM cross-compiled binary (~484 KB)
│   └── LOS201/              # Auxiliary subdirectory (contains Lab_3/)
├── rt_kernel/               # RT kernel directory (currently empty)
├── rootfs/
│   └── initramfs/           # Root filesystem for Lab 3
├── output/
│   └── initramfs_lab3.cpio.gz  # Packaged initramfs (~8.9 MB, Git LFS)
├── logs/
│   ├── gdb_session.txt          # GDB remote debug session (summary)
│   ├── gdb_session_full.txt     # Full GDB session transcript
│   ├── valgrind_report.txt      # Detailed Valgrind leak check report
│   ├── valgrind_fixed.txt       # Valgrind output after fixing the leaks
│   ├── perf.data                # Raw perf data (~60 KB, binary — do not open)
│   ├── perf.data.old            # Previous perf session data (~60 KB)
│   ├── perf_report.txt          # perf report output (text)
│   ├── perf_stat.txt            # perf stat output
│   ├── strace_arm.txt           # strace on ARM (via QEMU)
│   ├── strace_full.txt          # Full strace on host
│   ├── strace_summary.txt       # System call count summary
│   ├── hist_standard.txt        # Raw latency histogram data
│   ├── latency_histogram.png    # Latency distribution chart (PNG image — do not open via git)
│   ├── plot_latency.gnuplot     # gnuplot script to generate the chart
│   ├── qemu_session1.txt        # QEMU session log 1
│   └── qemu_session2.txt        # QEMU session log 2
└── submit/
    └── <official submission folder>/   # Official submission package under lab_3/submit/
        ├── apps/                # Submitted app binaries
        ├── bao_cao/             # Submitted report
        ├── logs/                # Submitted logs
        └── output/              # Submitted output
```

---

## Requirements

| Tool | Notes |
|---|---|
| `arm-linux-gnueabi-gcc` or `arm-linux-gnueabihf-gcc` | Cross-compile apps for ARM |
| `Valgrind` | Run on host or inside QEMU |
| `GDB` + `gdbserver` | Remote debug — gdbserver runs in QEMU, GDB runs on the host |
| `perf` | CPU profiling (requires `linux-tools-$(uname -r)` or built from kernel source) |
| `strace` | System call tracing |
| `gnuplot` | Plot the latency histogram (see `logs/plot_latency.gnuplot`) |
| `qemu-system-arm` | Run ARM binaries and gdbserver |
| `git-lfs` | Required to pull `initramfs_lab3.cpio.gz` |

---

## Build & Run

**Build apps on host (x86):**

```bash
cd lab_3/apps
gcc -g -o buggy_app buggy_app.c
gcc -g -o leaky_app leaky_app.c
gcc -g -o perf_target perf_target.c -lm
```

**Cross-compile for ARM:**

```bash
arm-linux-gnueabi-gcc -g -o leaky_app_arm leaky_app.c
arm-linux-gnueabi-gcc -g -o perf_target_arm perf_target.c -lm
```

**Valgrind (memory leak detection):**

```bash
valgrind --leak-check=full --track-origins=yes ./leaky_app
```

**perf (CPU profiling):**

```bash
perf record -g ./perf_target
perf report     # view results (see logs/perf_report.txt for a sample output)
perf stat ./perf_target
```

**strace (system call tracing):**

```bash
strace -c ./leaky_app                       # summary
strace -o strace_full.txt ./leaky_app       # detailed
```

**GDB remote debug (via QEMU):**

```bash
# Inside QEMU (gdbserver side)
gdbserver :1234 /leaky_app_arm

# On the host (GDB side)
arm-linux-gnueabi-gdb leaky_app_arm
(gdb) target remote :1234
```

> See `logs/gdb_session.txt` and `logs/gdb_session_full.txt` for the actual session transcript.

**Plot latency histogram:**

```bash
gnuplot logs/plot_latency.gnuplot
```

---

## Results / Output

| File | Content |
|---|---|
| `logs/valgrind_report.txt` | Leak detected in `leaky_app` (name and scores not freed) |
| `logs/valgrind_fixed.txt` | Clean Valgrind output after fixing leaks |
| `logs/perf_report.txt` | Overhead: `__ieee754_log_fma` 48%, `heavy_compute` 37% |
| `logs/perf_stat.txt` | CPU cycles, instructions, cache miss statistics |
| `logs/strace_summary.txt` | System call count summary |
| `logs/gdb_session_full.txt` | Full GDB remote session transcript |
| `logs/latency_histogram.png` | Latency distribution chart (PNG) |
| `output/initramfs_lab3.cpio.gz` | Packaged initramfs (~8.9 MB, Git LFS) |

---

## Notes

- `perf_target.c`: `heavy_compute` runs `sqrt()` + `log()` over 100,000 elements × 50 iterations — it dominates CPU time according to `perf_report.txt`.
- `buggy_app.c` has 3 intentional bugs (out-of-bounds, use-after-free, integer overflow) for debugging practice.
- `leaky_app.c` intentionally does not free `name` and `scores` in `Record` for Valgrind practice.
- `rt_kernel/` is currently empty — it may relate to an RT-preempt patch and is expected to be populated later.

---

[← Back to main README](../README.md)
