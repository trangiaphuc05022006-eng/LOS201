# Building the ARM Kernel Module

## Build inputs

`driver/Makefile` invokes Linux's external-module build system. Its checked-in configuration is:

```make
KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-
obj-m += lab2_driver.o
```

`KDIR` is a **machine-specific absolute location via `HOME`**: it must refer to the configured and prepared kernel tree whose binary boots inside QEMU. It is not the relative path `lab_1/output/`, which stores compiled images rather than the kernel build tree.

From the repository root, run:

```bash
cd lab_2/driver
make
file lab2_driver.ko
modinfo lab2_driver.ko | grep -E 'filename|vermagic'
```

The module's `vermagic` must be compatible with the running guest kernel. A successful host compilation alone does not prove that `insmod` will accept the module in the guest.

## Diagnosing errors

The tracked [`build.log`](../driver/build.log) records an assembler error, `selected processor does not support 'isb' in ARM mode`, during an earlier build attempt. It demonstrates an architecture/assembler mismatch, **not** a successful build. Check the cross-compiler prefix, the kernel's ARM configuration, and the exact compiler flags emitted by Kbuild. Do not add arbitrary flags without checking the kernel's build rules.

Separately, the tracked guest [`boot_log.txt`](../logs/boot_log.txt) shows a successful module load. The two logs describe different executions and should not be presented as one successful continuous run.

## Source references

[`Makefile`](../driver/Makefile) and [`build.log`](../driver/build.log).
