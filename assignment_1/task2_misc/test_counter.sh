#!/bin/sh
# test_counter.sh - functional test for counter_driver (runs inside QEMU, as root)
#
# Rules this script relies on (they are design decisions, state them in the report):
#   - a read that returns data gives the current value and then adds 1
#   - the second read() that cat makes (end of data) does NOT add 1,
#     so cat gives 0, 1, 2, ... without skipping a number
#   - "total reads" counts reads that returned data, "total writes" counts accepted writes
#   - a rejected write (not a number, negative, too long) changes nothing and is not counted
#
# Counters start at 0 right after insmod, so the expected numbers below are exact.

DEV=/dev/counter
PROC=/proc/counter_info
MODULE=/lib/modules/5.15.0/counter_driver.ko
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

# proc_of <label>: print the value of one line of /proc/counter_info,
# e.g. proc_of "total reads"
proc_of() {
    sed -n "s/^$1: *//p" $PROC
}

# rejected <text>: try to write <text> to the device, print yes if the write failed.
# dd is used on purpose: it reports a failed write() with a non-zero exit status.
rejected() {
    printf '%s\n' "$1" | dd of=$DEV 2>/dev/null
    [ $? -ne 0 ] && echo yes || echo no
}

# module_loaded: prints yes or no
module_loaded() {
    lsmod | grep -q counter_driver && echo yes || echo no
}
# ---- end helpers ----

dmesg -n 1 2>/dev/null          # keep kernel messages off the console; dmesg still stores them

echo "=== 1. load module, check the device ==="
insmod $MODULE || { echo "insmod failed, stopping"; exit 1; }
check "module is loaded" "yes" "$(module_loaded)"
ls -l $DEV
check "$DEV is a character device" "yes" "$([ -c $DEV ] && echo yes || echo no)"
check_has "major number is 10 (MISC_MAJOR)" " 10," "$(ls -l $DEV)"
check "$PROC exists" "yes" "$([ -e $PROC ] && echo yes || echo no)"

echo "=== 2. /proc/counter_info BEFORE any read or write ==="
cat $PROC
check "current value" "0" "$(proc_of 'current value')"
check "total reads"   "0" "$(proc_of 'total reads')"
check "total writes"  "0" "$(proc_of 'total writes')"

echo "=== 3. three reads with cat: 0, 1, 2 (no skipped numbers) ==="
A=$(cat $DEV)
B=$(cat $DEV)
C=$(cat $DEV)
echo "cat gave: $A $B $C"
check "first cat"  "0" "$A"
check "second cat" "1" "$B"
check "third cat"  "2" "$C"
check "total reads counts data reads only (not the end-of-data call)" "3" "$(proc_of 'total reads')"

echo "=== 4. write 100 ==="
printf '100\n' > $DEV
echo "--- /proc/counter_info AFTER the write ---"
cat $PROC
check "current value after write"  "100" "$(proc_of 'current value')"
check "total writes after write"   "1"   "$(proc_of 'total writes')"
D=$(cat $DEV)
E=$(cat $DEV)
echo "cat gave: $D $E"
check "read returns the value that was set" "100" "$D"
check "and then keeps counting"             "101" "$E"

echo "=== 5. write 0 resets the counter ==="
printf '0\n' > $DEV
echo "--- /proc/counter_info AFTER the reset ---"
cat $PROC
check "current value after reset" "0" "$(proc_of 'current value')"
check "total reads"               "5" "$(proc_of 'total reads')"
check "total writes"              "2" "$(proc_of 'total writes')"
F=$(cat $DEV)
check "first read after reset" "0" "$F"
check "current value after that read" "1" "$(proc_of 'current value')"

echo "=== 6. invalid writes are rejected with EINVAL ==="
check "write of -5 (negative) fails"        "yes" "$(rejected -5)"
check "write of abc (not a number) fails"   "yes" "$(rejected abc)"
check "write of 99999999999 (overflow) fails" "yes" "$(rejected 99999999999)"
check "write of a 20-digit number (too long) fails" "yes" "$(rejected 12345678901234567890)"
ERR=$(printf '%s\n' -5 | dd of=$DEV 2>&1)
echo "message from dd: $ERR"
check_has "the error reported to user space is EINVAL" "Invalid argument" "$ERR"
check "rejected writes changed nothing (value)"  "1" "$(proc_of 'current value')"
check "rejected writes were not counted"         "2" "$(proc_of 'total writes')"

echo "=== 7. unload and check cleanup ==="
rmmod counter_driver
check "module is gone" "no" "$(module_loaded)"
check "$PROC is gone" "no" "$([ -e $PROC ] && echo yes || echo no)"
check "$DEV is gone"  "no" "$([ -e $DEV ] && echo yes || echo no)"
DMESG=$(dmesg)
check_has "dmesg shows the unload line" "module unloaded" "$DMESG"
echo "--- dmesg (counter_driver lines) ---"
echo "$DMESG" | grep counter_driver

echo "=== SUMMARY: $PASS passed, $FAIL failed ==="
[ "$FAIL" -eq 0 ]
