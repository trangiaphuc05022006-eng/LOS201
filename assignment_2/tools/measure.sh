#!/bin/sh
# measure.sh - run sms_app for 60 s and snapshot its memory at T=5, 30, 55 s.
# VmRSS was unreliable on this kernel (it stayed at 4 kB), so each snapshot
# also reads smaps_rollup / smaps, which walk the page tables.

sms_app -d 60 > /tmp/run.txt 2>&1 &
PID=$!

snap() {
    echo "--- T=$1 ---"
    grep -E 'VmSize|VmRSS' /proc/$PID/status
    if [ -r /proc/$PID/smaps_rollup ]; then
        grep -E '^(Rss|Pss)' /proc/$PID/smaps_rollup
    else
        echo "smaps total Rss (kB): $(grep '^Rss:' /proc/$PID/smaps | awk '{s+=$2} END {print s}')"
    fi
}

sleep 5
snap 5s

sleep 25
snap 30s
echo "--- maps ---"
cat /proc/$PID/maps

sleep 25
snap 55s

wait $PID
echo "--- finished ---"
cat /tmp/run.txt
cat /proc/sms_stats