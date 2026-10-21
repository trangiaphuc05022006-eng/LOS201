#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "sms_sensor.h"

#define DRIVER_NAME "sms_sensor"

// Biến lưu trữ trạng thái thống kê
static int read_count = 0;
static int last_temp_int = 0, last_temp_dec = 0;
static int last_hum_int = 0, last_hum_dec = 0;
static int sample_interval_ms = 100; // Mặc định 100ms
static ktime_t start_time;

// Đọc dữ liệu từ /dev/sms_sensor
static ssize_t sms_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[128];
    int len;
    u32 rand_val;
    u64 timestamp_ms;

    if (*ppos > 0) return 0; // Tránh lặp vô tận khi user dùng cat

    // Lấy timestamp hiện tại
    timestamp_ms = ktime_to_ms(ktime_get());

    // Sinh nhiệt độ (15.0 - 45.0) và độ ẩm (30.0 - 90.0)
    get_random_bytes(&rand_val, sizeof(rand_val));
    last_temp_int = 15 + (rand_val % 31);
    last_temp_dec = (rand_val >> 8) % 10; // 1 chữ số thập phân

    get_random_bytes(&rand_val, sizeof(rand_val));
    last_hum_int = 30 + (rand_val % 61);
    last_hum_dec = (rand_val >> 8) % 10;

    // Format thành chuỗi CSV
    len = snprintf(kbuf, sizeof(kbuf), "%llu,%d.%d,%d.%d\n",
                   timestamp_ms, last_temp_int, last_temp_dec, last_hum_int, last_hum_dec);

    if (copy_to_user(buf, kbuf, len)) return -EFAULT;

    read_count++;
    *ppos = len;
    return len;
}

// Xử lý ioctl
static long sms_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int new_interval;
    switch (cmd) {
        case SMS_SET_INTERVAL:
            if (copy_from_user(&new_interval, (int __user *)arg, sizeof(new_interval)))
                return -EFAULT;
            sample_interval_ms = new_interval;
            pr_info("sms_sensor: Interval set to %d ms\n", sample_interval_ms);
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static const struct file_operations sms_fops = {
    .owner = THIS_MODULE,
    .read = sms_read,
    .unlocked_ioctl = sms_ioctl,
};

static struct miscdevice sms_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DRIVER_NAME,
    .fops = &sms_fops,
};

// Hiển thị /proc/sms_stats
static int sms_proc_show(struct seq_file *m, void *v)
{
    u64 uptime_sec = ktime_to_ms(ktime_sub(ktime_get(), start_time)) / 1000;
    seq_printf(m, "read_count: %d\n", read_count);
    seq_printf(m, "last_temperature: %d.%d\n", last_temp_int, last_temp_dec);
    seq_printf(m, "last_humidity: %d.%d\n", last_hum_int, last_hum_dec);
    seq_printf(m, "uptime: %llu seconds\n", uptime_sec);
    return 0;
}

static int sms_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, sms_proc_show, NULL);
}

static const struct proc_ops sms_proc_ops = {
    .proc_open = sms_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init sms_sensor_init(void)
{
    int ret;
    start_time = ktime_get();
    
    ret = misc_register(&sms_miscdev);
    if (ret) {
        pr_err("sms_sensor: Failed to register misc device\n");
        return ret;
    }

    if (!proc_create("sms_stats", 0444, NULL, &sms_proc_ops)) {
        misc_deregister(&sms_miscdev);
        pr_err("sms_sensor: Failed to create proc entry\n");
        return -ENOMEM;
    }

    pr_info("sms_sensor: Module loaded\n");
    return 0;
}

static void __exit sms_sensor_exit(void)
{
    remove_proc_entry("sms_stats", NULL);
    misc_deregister(&sms_miscdev);
    pr_info("sms_sensor: Module unloaded\n");
}

module_init(sms_sensor_init);
module_exit(sms_sensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Trần Nguyễn Việt Dũng");
MODULE_DESCRIPTION("Sensor Monitoring System - Dummy Kernel Driver");
