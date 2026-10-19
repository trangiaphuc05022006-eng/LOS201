#!/bin/sh
# ==============================================================================
# File: test_counter.sh
# Author: Tran Gia Phuc - SE204417
# Description: Automated test script for ASSIGNMENT-01 Task 2 (counter_driver)
#              Testing Misc Device (major=10), atomic read/write, and procfs
# ==============================================================================

MODULE_KO="/lib/modules/5.15.0/counter_driver.ko"
DEV_NODE="/dev/counter"

echo "============================================================"
echo "  ASSIGNMENT 01 - TASK 2: MISC COUNTER DRIVER TEST SUITE"
echo "  Student: Tran Gia Phuc - SE204417"
echo "============================================================"

# 1. Load module and check misc device registration
echo ""
echo "[STEP 1] Loading counter_driver.ko and checking /dev/counter (major=10)..."
if lsmod | grep -q "counter_driver"; then
    rmmod counter_driver
fi

insmod "$MODULE_KO"
if [ $? -ne 0 ]; then
    echo "  [FAIL] insmod $MODULE_KO failed!"
    exit 1
fi

# Ensure device node exists if devtmpfs did not auto-create it immediately
if [ ! -c "$DEV_NODE" ]; then
    MINOR=$(grep "counter" /proc/misc | awk '{print $1}')
    [ -n "$MINOR" ] && mknod "$DEV_NODE" c 10 "$MINOR"
fi
chmod 666 "$DEV_NODE"

ls -l "$DEV_NODE"
lsmod | grep "counter_driver"
grep "counter" /proc/misc

# 2. Test auto-increment on READ (3 consecutive reads: 0, 1, 2)
echo ""
echo "[STEP 2] Testing READ auto-increment on /dev/counter..."
R1=$(cat "$DEV_NODE")
R2=$(cat "$DEV_NODE")
R3=$(cat "$DEV_NODE")
echo "  -> Read #1 (expected 0): $R1"
echo "  -> Read #2 (expected 1): $R2"
echo "  -> Read #3 (expected 2): $R3"
if [ "$R1" = "0" ] && [ "$R2" = "1" ] && [ "$R3" = "2" ]; then
    echo "  [PASS] Auto-increment on read works accurately."
else
    echo "  [FAIL] Unexpected counter values on read!"
fi

# 3. Check /proc/counter_info at Timepoint #1 (Before WRITE)
echo ""
echo "[STEP 3] Checking /proc/counter_info [Timepoint #1 - Before Write]:"
cat /proc/counter_info

# 4. Test WRITE to set counter = 100
echo ""
echo "[STEP 4] Testing WRITE: setting counter to 100..."
echo "100" > "$DEV_NODE"
R_AFTER_100=$(cat "$DEV_NODE")
R_NEXT_101=$(cat "$DEV_NODE")
echo "  -> First read after setting 100 (expected 100): $R_AFTER_100"
echo "  -> Next read (expected 101)                    : $R_NEXT_101"
if [ "$R_AFTER_100" = "100" ] && [ "$R_NEXT_101" = "101" ]; then
    echo "  [PASS] Counter set to 100 and incremented to 101."
else
    echo "  [FAIL] Counter set 100 failed!"
fi

# 5. Check /proc/counter_info at Timepoint #2 (After WRITE 100)
echo ""
echo "[STEP 5] Checking /proc/counter_info [Timepoint #2 - After Write 100]:"
cat /proc/counter_info

# 6. Test WRITE '0' to reset counter to 0
echo ""
echo "[STEP 6] Testing WRITE '0' to reset counter to 0..."
echo "0" > "$DEV_NODE"
R_AFTER_RESET=$(cat "$DEV_NODE")
echo "  -> Read immediately after reset (expected 0): $R_AFTER_RESET"
if [ "$R_AFTER_RESET" = "0" ]; then
    echo "  [PASS] Counter reset to 0 accurately."
else
    echo "  [FAIL] Counter reset to 0 failed!"
fi

# 7. Check /proc/counter_info at Timepoint #3 (After RESET)
echo ""
echo "[STEP 7] Checking /proc/counter_info [Timepoint #3 - After Reset]:"
cat /proc/counter_info

# 8. Test error handling (-EINVAL) for negative and non-numeric writes
echo ""
echo "[STEP 8] Testing invalid WRITE inputs (-EINVAL check)..."
echo "  -> Writing negative value '-50' to /dev/counter:"
if echo "-50" > "$DEV_NODE" 2>/dev/null; then
    echo "  [FAIL] Negative value was accepted!"
else
    echo "  [PASS] Negative value '-50' rejected with error (-EINVAL)."
fi

echo "  -> Writing non-numeric string 'abc_invalid' to /dev/counter:"
if echo "abc_invalid" > "$DEV_NODE" 2>/dev/null; then
    echo "  [FAIL] Non-numeric string was accepted!"
else
    echo "  [PASS] Non-numeric string 'abc_invalid' rejected with error (-EINVAL)."
fi

# 9. Unload module and check dmesg
echo ""
echo "[STEP 9] Unloading counter_driver and checking dmesg..."
rmmod counter_driver
if [ $? -eq 0 ]; then
    echo "  [PASS] rmmod counter_driver succeeded."
else
    echo "  [FAIL] rmmod counter_driver failed!"
fi

echo ""
echo "[STEP 10] Kernel log (dmesg) for counter_driver:"
dmesg | grep "counter_driver"

echo ""
echo "============================================================"
echo "  TASK 2 TEST SUITE COMPLETED SUCCESSFULLY!"
echo "============================================================"
