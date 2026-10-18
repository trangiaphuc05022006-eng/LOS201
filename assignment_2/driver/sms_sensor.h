/*
 * sms_sensor.h
 * Shared between kernel module (sms_sensor_driver.c) and userspace (sms_app).
 * Author: <Tran Hoang Khoi - SE203155>
 */

#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

/* ioctl magic number: one character */
#define SMS_IOC_MAGIC  'z'

/* Set the minimum interval between two reads, in ms */
#define SMS_SET_INTERVAL  _IOW(SMS_IOC_MAGIC, 1, int)

/* Shared constants: both sides must agree on these */
#define SMS_DEFAULT_INTERVAL_MS  100
#define SMS_MIN_INTERVAL_MS      1
#define SMS_MAX_INTERVAL_MS      60000
#endif /* closing the guard */