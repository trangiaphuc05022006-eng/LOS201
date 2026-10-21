/*
 * sms_sensor_driver.c - simulated temperature/humidity sensor
 * Author: Tran Hoang Khoi - SE203155
 * Date:   2026-10-17
 */
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include "sms_sensor.h"
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DEVICE_NAME "sms_sensor"

static DEFINE_MUTEX(sms_mutex);
static int           interval_ms = SMS_DEFAULT_INTERVAL_MS;
static unsigned long read_count;
static int           last_temp;     /* tenths of degC */
static int           last_hum;      /* tenths of % */
static u64           last_read_ms;  /* time of the previous read */
static struct proc_dir_entry *proc_entry;
static time64_t               load_sec;

static long sms_ioctl(struct file *f, unsigned int cmd, unsigned long arg);

static ssize_t sms_read(struct file *f, char __user *buf,
                        size_t len, loff_t *off)
{
    char line[64];
    int  n;
    u64  now;

    /* TODO 3: random values: temp in 150..450, hum in 300..900
     *         (get_random_u32() % range + offset) */
    int temp = get_random_u32() % (450 - 150 + 1) + 150;
    int hum = get_random_u32() % (900 - 300 + 1) + 300;


    /* TODO 1: lock sms_mutex, but let Ctrl+C interrupt the wait.
     *         (mutex_lock_interruptible; if it fails -> return -EINTR) */
    if (mutex_lock_interruptible(&sms_mutex)) {
          return -EINTR;
    }

    /* TODO 2: enforce the interval. now = time in ms. If less than
     *         interval_ms has passed since last_read_ms, sleep the
     *         remaining time with msleep_interruptible().
     *         If it returns non-zero (interrupted): unlock, return -EINTR. */
    now = ktime_to_ms(ktime_get());  /* convert ns to ms */
    if (now - last_read_ms < interval_ms) { 
            u64 remaining = interval_ms - (now - last_read_ms);
        if( msleep_interruptible(remaining)) {
            mutex_unlock(&sms_mutex);
            return -EINTR;
        }
        now = ktime_to_ms(ktime_get());  /* update now after sleep */
    }


    /* TODO 4: build the CSV line with scnprintf into line[]:
     *         "<ms>,<t/10>.<t%10>,<h/10>.<h%10>\n" */
    n = scnprintf(line, sizeof(line), "%llu,%d.%d,%d.%d\n", now, temp / 10, temp % 10, hum / 10, hum % 10);

    /* TODO 5: update last_temp, last_hum, last_read_ms, read_count,
     *         then unlock */
    last_temp = temp; 
    last_hum = hum; 
    last_read_ms = now;
    read_count++; 
    mutex_unlock(&sms_mutex);

    /* TODO 6: copy at most len bytes to the user buffer. */
    if (len < n) {
        n = len;
    }
    if (copy_to_user(buf, line, n)) {
        return -EFAULT;
    }

    return n;
}

static const struct file_operations sms_fops = {
    .owner = THIS_MODULE,
    .read = sms_read,
    .unlocked_ioctl = sms_ioctl,
};

static struct miscdevice sms_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &sms_fops,
};

static int sms_proc_show(struct seq_file *m, void *v)
{
    /* TODO A: lock sms_mutex (interruptible; on failure return -EINTR) */
    if (mutex_lock_interruptible(&sms_mutex)) {
        return -EINTR;
    }
    /* TODO B: seq_printf these lines:
     *   read_count, last temperature and humidity (as x.y, like the CSV),
     *   uptime in seconds = ktime_get_seconds() - load_sec  (print with %lld) */
    seq_printf(m, "Read count: %lu\n", read_count);
    seq_printf(m, "last temperature: %d.%d\n", last_temp / 10, last_temp % 10);
    seq_printf(m, "last humidity: %d.%d\n", last_hum / 10, last_hum % 10);
    seq_printf(m, "uptime: %lld\n", (long long)(ktime_get_seconds() - load_sec));
    /* TODO C: unlock */
    mutex_unlock(&sms_mutex); 
    return 0;
}

static int sms_proc_open(struct inode *i, struct file *f)
{
    return single_open(f, sms_proc_show, NULL);
}

static const struct proc_ops sms_proc_ops = {
    .proc_open    = sms_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static long sms_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    int val;

    /* TODO D: if the magic number in cmd is not SMS_IOC_MAGIC -> -ENOTTY
     *         (hint: _IOC_TYPE(cmd)) */
    if (_IOC_TYPE (cmd) != SMS_IOC_MAGIC) { 
        return -ENOTTY; 
    }

    switch (cmd) {
    case SMS_SET_INTERVAL:
        /* TODO E: copy an int from the user pointer (int __user *)arg;
         *         -EFAULT if it fails */
        if(copy_from_user(&val, (int __user *)arg, sizeof(int))) {
            return -EFAULT;
        }
        /* TODO F: outside SMS_MIN_INTERVAL_MS..SMS_MAX_INTERVAL_MS -> -EINVAL */
        if(val < SMS_MIN_INTERVAL_MS || val > SMS_MAX_INTERVAL_MS) {
            return -EINVAL;
        }
        /* TODO G: set interval_ms under the mutex; pr_info the new value */
        if (mutex_lock_interruptible(&sms_mutex)) {
            return -EINTR;
        }
        interval_ms = val;
        mutex_unlock(&sms_mutex);
        pr_info("New interval_ms: %d\n", val);
        return 0;
    default:
        return -ENOTTY;
    }
}

static int __init sms_init(void)
{
    int ret;

    ret = misc_register(&sms_misc);
    if (ret) {
        pr_err("Failed to register misc device\n");
        return ret;
    }

    load_sec = ktime_get_seconds();
    proc_entry = proc_create("sms_stats", 0444, NULL, &sms_proc_ops);
    if (!proc_entry) {
        misc_deregister(&sms_misc);
        return -ENOMEM;
    }

    pr_info("SMS sensor driver loaded\n");
    return 0;
}

static void __exit sms_exit(void)
{
    proc_remove(proc_entry);
    misc_deregister(&sms_misc);
    pr_info("SMS sensor driver unloaded\n");
}

module_init(sms_init);
module_exit(sms_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tran Hoang Khoi - SE203155");
MODULE_DESCRIPTION("SMS simulated sensor driver");