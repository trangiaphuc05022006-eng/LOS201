#!/bin/sh
# test_asgn1.sh - functional test for asgn1_driver (runs inside QEMU, as root)
#
# Rules this script relies on (they are design decisions, state them in the report):
#   - read_count counts every read() call, including the final end-of-data call:
#     one cat of a non-empty buffer adds 2, one cat of an empty buffer adds 1
#   - write_count counts successful writes only
#   - last_write_size is the number of bytes the USER sent, not the stored count
#   - RESET clears the buffer, read_count, write_count and last_write_size,
#     but NOT open_count
#   - open_count goes up on every open (printf >, cat, asgn1_ctl), so it is
#     shown and sanity-checked, never compared with an exact number
#
# Counters start at 0 right after insmod, so the expected numbers below are exact.
 
DEV=/dev/asgn1
MAJOR=241
MODULE=/lib/modules/5.15.0/asgn1_driver.ko
PASS=0
FAIL=0
 
# ---- helpers ----
# check <description> <expected> <actual>: exact match
check() {
    if [ "$3" = "$2" ]; then
        echo "  PASS: $1"
        PASS=$((PASS + 1))
    else
        echo "  FAIL: $1 (expected '$2', got '$3')"
        FAIL=$((FAIL + 1))
    fi
}
 
# check_has <description> <text that must appear> <actual>: substring match
check_has() {
    case "$3" in
        *"$2"*) echo "  PASS: $1"; PASS=$((PASS + 1)) ;;
        *)      echo "  FAIL: $1 (expected to contain '$2', got '$3')"; FAIL=$((FAIL + 1)) ;;
    esac
}
 
# stat_of <field>: print one field of "asgn1_ctl stats", e.g. stat_of write_count
stat_of() {
    asgn1_ctl stats | sed -n "s/^$1 *: *//p"
}
 
# module_loaded: prints yes or no
module_loaded() {
    lsmod | grep -q asgn1_driver && echo yes || echo no
}
# ---- end helpers ----
 
dmesg -n 1 2>/dev/null          # keep kernel messages off the console; dmesg still stores them
 
echo "=== 1. load module and create the device node ==="
insmod $MODULE || { echo "insmod failed, stopping"; exit 1; }
check "module is loaded" "yes" "$(module_loaded)"
[ -c $DEV ] || mknod $DEV c $MAJOR 0
ls -l $DEV
check "$DEV is a character device" "yes" "$([ -c $DEV ] && echo yes || echo no)"
 
echo "=== 2. GET_VERSION ==="
V=$(asgn1_ctl version)
echo "$V"
check_has "version string returned" "asgn1_driver" "$V"
 
echo "=== 3. echo mode (default): data stored unchanged ==="
printf 'hi' > $DEV
OUT=$(cat $DEV)
echo "read back: [$OUT]"
check "echo mode stores 'hi' unchanged" "hi" "$OUT"
asgn1_ctl stats
check "write_count"     "1" "$(stat_of write_count)"
check "buffer_len"      "2" "$(stat_of buffer_len)"
check "last_write_size" "2" "$(stat_of last_write_size)"
check "read_count (one cat = data call + end-of-data call)" "2" "$(stat_of read_count)"
 
echo "=== 4. kernel mode: prefix added ==="
asgn1_ctl mode 0
printf 'hi' > $DEV
OUT=$(cat $DEV)
echo "read back: [$OUT]"
check "kernel mode stores [KERNEL]hi" "[KERNEL]hi" "$OUT"
asgn1_ctl stats
check "write_count (counters accumulate)" "2" "$(stat_of write_count)"
check "buffer_len is the STORED size"     "10" "$(stat_of buffer_len)"
check "last_write_size is the USER size"  "2" "$(stat_of last_write_size)"
check "read_count"                        "4" "$(stat_of read_count)"
 
echo "=== 5. oversized write is cut to fit (kernel mode) ==="
# The driver accepts only what fits and returns that smaller count (1015 = 1023 - 8).
# dd treats a short write as "not finished" and sends the remaining 985 bytes in a
# second write(), which replaces the buffer again (every write replaces it).
dd if=/dev/zero of=$DEV bs=2000 count=1 2>/dev/null
asgn1_ctl stats
check "write_count: dd needed two write() calls (2 + 2)"   "4"   "$(stat_of write_count)"
check "buffer_len after the second write (8 + 985)"        "993" "$(stat_of buffer_len)"
check "last_write_size = second write (2000 - 1015)"       "985" "$(stat_of last_write_size)"
check_has "driver logged the truncation" "write too large" "$(dmesg)"

echo "=== 6. RESET_BUFFER ==="
printf 'hello' > $DEV
check "buffer filled before reset ([KERNEL]hello = 13)" "13" "$(stat_of buffer_len)"
asgn1_ctl reset
OUT=$(cat $DEV)
echo "read back after reset: [$OUT]"
check "buffer is empty after reset" "" "$OUT"
asgn1_ctl stats
check "write_count reset"     "0" "$(stat_of write_count)"
check "buffer_len reset"      "0" "$(stat_of buffer_len)"
check "last_write_size reset" "0" "$(stat_of last_write_size)"
check "read_count reset, then one cat of an empty buffer = 1" "1" "$(stat_of read_count)"
OC=$(stat_of open_count)
echo "open_count after reset: $OC (RESET must not clear it)"
check "open_count survived reset" "yes" "$([ "$OC" -gt 0 ] && echo yes || echo no)"
 
echo "=== 7. error cases ==="
ERR=$(asgn1_ctl mode 5 2>&1)
echo "$ERR"
check_has "mode 5 rejected with EINVAL" "Invalid argument" "$ERR"
printf 'hi' > $DEV
check "rejected mode left kernel mode unchanged" "[KERNEL]hi" "$(cat $DEV)"
ERR=$(asgn1_ctl bogus 2>&1)
echo "$ERR"
check_has "unknown ioctl rejected with ENOTTY" "Inappropriate ioctl" "$ERR"
asgn1_ctl mode 1
printf 'hi' > $DEV
check "SET_MODE 1 returns to echo mode" "hi" "$(cat $DEV)"
 
echo "=== 8. unload and check cleanup ==="
rmmod asgn1_driver
check "module is gone" "no" "$(module_loaded)"
DMESG=$(dmesg)
check_has "dmesg shows the cleanup line" "unloaded, cleanup done" "$DMESG"
echo "--- dmesg (asgn1_driver lines) ---"
echo "$DMESG" | grep asgn1_driver
ls -l $DEV 2>/dev/null || echo "$DEV is gone"
 
echo "=== SUMMARY: $PASS passed, $FAIL failed ==="
[ "$FAIL" -eq 0 ]