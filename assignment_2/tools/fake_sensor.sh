#!/bin/bash
# fake_sensor.sh - host-only helper: feeds CSV lines into a FIFO
# so sms_app_x86 can run on a laptop that has no /dev/sms_sensor.
FIFO=/tmp/fake_sensor
rm -f "$FIFO"
mkfifo "$FIFO"
while true; do
  echo "$(date +%s%3N),$((RANDOM%31+15)).0,$((RANDOM%61+30)).0"
  sleep 0.5
done > "$FIFO"