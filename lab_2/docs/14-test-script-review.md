# Reviewing the Two Bundled Test Scripts

## test_driver.sh

This script calls `insmod`, checks `lsmod`, creates the character node if needed, writes a sample line, reads the buffer, repeats three write/read pairs, and prints selected `dmesg` lines. Its final `=== Test PASSED ===` line is an **unconditional echo**, not a computed assertion summary. Treat it as a demonstration script rather than proof of complete error-free testing.

## test_procfs.sh

This script also calls `insmod`, then displays `/proc/lab2_info` before and after a write and reads `buffer_len`, `open_count`, and `last_data` from the sysfs class device. The printed values are useful evidence, but the script does not rigorously validate them against expected results before reporting its final message.

## The preloaded-module trap

`rcS` attempts `insmod /lib/modules/5.15.0/lab2_driver.ko` during boot. When the module is already loaded, a second `insmod` can fail with a file-exists error; `test_driver.sh` exits early on that failure. The lab README therefore recommends:

```sh
lsmod | grep lab2
rmmod lab2_driver
sh /usr/bin/test_driver.sh
rmmod lab2_driver
sh /usr/bin/test_procfs.sh
```

Only run the `rmmod` command if the module is currently loaded and no process is using it. Both scripts are present in the initramfs tree; their exact location in the live guest can be checked with `ls /usr/bin/test_*.sh`.

## Source references

[`test_driver.sh`](../rootfs/initramfs/usr/bin/test_driver.sh), [`test_procfs.sh`](../rootfs/initramfs/usr/bin/test_procfs.sh), and [`rcS`](../rootfs/initramfs/etc/init.d/rcS).
