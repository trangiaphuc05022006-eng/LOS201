#!/bin/sh
# ==============================================================================
# File: test_asgn1.sh
# Author: Tran Gia Phuc - SE204417
# Description: Automated test script for ASSIGNMENT-01 Task 1 (asgn1_driver)
#              Testing character device read/write and 4 ioctl commands in QEMU
# ==============================================================================

MODULE_KO="/lib/modules/5.15.0/asgn1_driver.ko"
DEV_NODE="/dev/asgn1"
MAJOR=241
IOCTL_BIN="/usr/bin/ioctl_app"

echo "============================================================"
echo "  ASSIGNMENT 01 - TASK 1: IOCTL DRIVER TEST SUITE"
echo "  Student: Tran Gia Phuc - SE204417"
echo "============================================================"

# 1. Load kernel module and create device node
echo ""
echo "[STEP 1] Loading asgn1_driver.ko and verifying /dev/asgn1 (major=$MAJOR)..."
if lsmod | grep -q "asgn1_driver"; then
    rmmod asgn1_driver
fi

insmod "$MODULE_KO"
if [ $? -ne 0 ]; then
    echo "  [FAIL] insmod $MODULE_KO failed!"
    exit 1
fi

[ -c "$DEV_NODE" ] || mknod "$DEV_NODE" c $MAJOR 0
chmod 666 "$DEV_NODE"
ls -l "$DEV_NODE"
lsmod | grep "asgn1_driver"

# 2. Test ioctl Command 4: ASGN1_GET_VERSION
echo ""
echo "[STEP 2] Testing ioctl Command 4: ASGN1_GET_VERSION..."
$IOCTL_BIN version

# 3. Test Default Mode (Mode 1 - Echo mode) Write & Read
echo ""
echo "[STEP 3] Testing Write/Read in Default Mode (Mode 1 - Echo Mode)..."
echo "Hello_Echo_Mode_SE204417" > "$DEV_NODE"
READ_ECHO=$(cat "$DEV_NODE")
echo "  -> Data written : Hello_Echo_Mode_SE204417"
echo "  -> Data read    : $READ_ECHO"
if [ "$READ_ECHO" = "Hello_Echo_Mode_SE204417" ]; then
    echo "  [PASS] Echo Mode preserved original string."
else
    echo "  [FAIL] Echo Mode mismatch!"
fi

# 4. Test ioctl Command 3: ASGN1_SET_MODE -> Mode 0 (Kernel mode)
echo ""
echo "[STEP 4] Testing ioctl Command 3: ASGN1_SET_MODE -> Mode 0 (Kernel Mode)..."
$IOCTL_BIN mode 0
echo "Data_From_Userspace" > "$DEV_NODE"
READ_KERNEL=$(cat "$DEV_NODE")
echo "  -> Data written : Data_From_Userspace"
echo "  -> Data read    : $READ_KERNEL"
if [ "$READ_KERNEL" = "[KERNEL] Data_From_Userspace" ]; then
    echo "  [PASS] Kernel Mode prepended '[KERNEL] ' prefix accurately."
else
    echo "  [FAIL] Kernel Mode prefix mismatch!"
fi

# Switch back to Mode 1 (Echo mode) and verify
echo ""
echo "[STEP 5] Switching back to Mode 1 (Echo Mode) via ASGN1_SET_MODE..."
$IOCTL_BIN mode 1
echo "Back_To_Echo_Mode" > "$DEV_NODE"
READ_BACK=$(cat "$DEV_NODE")
echo "  -> Data read    : $READ_BACK"

# 5. Test ioctl Command 2: ASGN1_GET_STATS
echo ""
echo "[STEP 6] Testing ioctl Command 2: ASGN1_GET_STATS (before reset)..."
$IOCTL_BIN stats
echo "  -> Checking /proc/asgn1_info:"
cat /proc/asgn1_info

# 6. Test ioctl Command 1: ASGN1_RESET_BUFFER
echo ""
echo "[STEP 7] Testing ioctl Command 1: ASGN1_RESET_BUFFER..."
$IOCTL_BIN reset
READ_AFTER_RESET=$(cat "$DEV_NODE")
echo "  -> Output of 'cat /dev/asgn1' after reset: [$READ_AFTER_RESET]"
if [ -z "$READ_AFTER_RESET" ]; then
    echo "  [PASS] Buffer is completely empty after ASGN1_RESET_BUFFER."
else
    echo "  [FAIL] Buffer is NOT empty after reset!"
fi
echo "  -> Checking stats after reset (note: cat just incremented read_count on empty buffer or returned EOF):"
$IOCTL_BIN stats

# 7. Test invalid ioctl command handling (-ENOTTY)
echo ""
echo "[STEP 8] Testing invalid ioctl command handling (-ENOTTY)..."
$IOCTL_BIN invalid

# 8. Unload module and verify cleanup in dmesg
echo ""
echo "[STEP 9] Unloading asgn1_driver module and checking dmesg..."
rmmod asgn1_driver
if [ $? -eq 0 ]; then
    echo "  [PASS] rmmod asgn1_driver succeeded."
else
    echo "  [FAIL] rmmod asgn1_driver failed!"
fi

echo ""
echo "[STEP 10] Kernel log (dmesg) for asgn1_driver:"
dmesg | grep "asgn1_driver"

echo ""
echo "============================================================"
echo "  TASK 1 TEST SUITE COMPLETED SUCCESSFULLY!"
echo "============================================================"
