# LAB-02 Functional Verification Plan

This plan describes checks that can be repeated inside a booted QEMU guest. It is **not** a claim that all of them have been run and passed. Save the resulting terminal output if the course requires empirical evidence.

| Check | Action | Expected property |
|---|---|---|
| Module | `lsmod \| grep lab2` | Module name is present after load |
| Node | `ls -l /dev/lab2` | Character type, major 240, minor 0 |
| Normal write | `printf 'abc' > /dev/lab2` | Device accepts three bytes |
| Normal read | `cat /dev/lab2` | Shows most recently stored data |
| Replacement | Write `xy` after `abc` | Next fresh read shows `xy`, not `abcxy` |
| Length | Read sysfs `buffer_len` | Reports 2 after the `xy` write |
| procfs | `cat /proc/lab2_info` | Driver status fields are available |
| sysfs | Read all three attributes | Text values reflect shared state |
| EOF | Read twice on same open descriptor | Later read returns 0 after all data consumed |
| Cleanup | `rmmod lab2_driver` | Module unloads; its procfs/sysfs entries disappear |

## A repeatable mini-session

```sh
lsmod | grep lab2
printf 'abc' > /dev/lab2
cat /dev/lab2
printf 'xy' > /dev/lab2
cat /dev/lab2
cat /proc/lab2_info
cat /sys/class/lab2_class/lab2/buffer_len
```

Always run these inside QEMU, not on the Ubuntu host. Capture `dmesg` entries when investigating failures. Do not invoke `insmod` if the startup script already loaded the module; unload first if you intentionally want a fresh load.

## Source references

[`test_driver.sh`](../rootfs/initramfs/usr/bin/test_driver.sh), [`test_procfs.sh`](../rootfs/initramfs/usr/bin/test_procfs.sh), and [`lab2_driver.c`](../driver/lab2_driver.c).
