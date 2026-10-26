# Troubleshooting and Scope Boundaries

## Common failures

**`insmod: File exists` or module already loaded.** The BusyBox `rcS` script tries to load the module during boot. Check `lsmod | grep lab2` before running a test script that also invokes `insmod`; unload the module first if safe.

**`insmod: Invalid module format`.** Compare `uname -r` in the guest with `modinfo lab2_driver.ko | grep vermagic` on a host that has `modinfo`. Compile against the correct prepared kernel tree and architecture.

**`/dev/lab2` missing.** Verify the module loaded, inspect its major number, and check whether `devtmpfs` is mounted. In the teaching guest, create the node with `mknod /dev/lab2 c 240 0` only after confirming the registration.

**`/proc/lab2_info` or sysfs attributes missing.** These are created after the character device; their creation failures log warnings but do not necessarily fail the whole module load. Check `dmesg` and use the exact paths defined in source.

**Assembler reports unsupported `isb`.** This specific error occurs in the repository's saved build transcript. Re-check cross-toolchain selection and Kbuild flags for the ARM kernel configuration. Do not claim this error is resolved solely because a module binary exists elsewhere in the repository.

## Boundaries of the submission

The original LAB-02 assignment mentions additional MTD/JFFS2 topics, but the checked-in LAB-02 source principally demonstrates character-device operations, mutex coordination, procfs/sysfs, and QEMU integration. This guide does not claim a separate MTD/JFFS2 implementation or test results without repository evidence.

## Safe collaboration

Keep documentation changes in `lab_2/docs/` and `lab_2/README.md`. Do not overwrite kernel images, driver `.c` files, Makefiles, or another team member's branch merely to accumulate commits. Review the file list of every pull request before merging.

## Source references

[`lab2_driver.c`](../driver/lab2_driver.c), [`rcS`](../rootfs/initramfs/etc/init.d/rcS), [`build.log`](../driver/build.log), and the [main LAB-02 README](../README.md).
