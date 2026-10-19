/*
 * File: counter_driver.c
 * Author: Tran Gia Phuc - SE204417
 * Description: ASSIGNMENT-01 Task 2 - Miscellaneous Device Driver (Counter Device)
 *              Using miscdevice framework (major 10), atomic_t for thread-safety,
 *              auto-increment on read, kstrtoint parsing on write, and procfs stats.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tran Gia Phuc - SE204417");
MODULE_DESCRIPTION("ASSIGNMENT-01: Miscellaneous Device Driver - Counter Device");
MODULE_VERSION("1.0");

#define DEVICE_NAME     "counter"
#define PROCFS_NAME     "counter_info"

/* ===== DATA LAYER (Tầng dữ liệu an toàn đa luồng SMP) ===== */
static atomic_t counter_val   = ATOMIC_INIT(0); /* Giá trị bộ đếm hiện tại */
static atomic_t total_reads   = ATOMIC_INIT(0); /* Tổng số lần đọc thành công */
static atomic_t total_resets  = ATOMIC_INIT(0); /* Tổng số lần ghi/reset bộ đếm */

static struct proc_dir_entry *proc_entry = NULL;

/* ===== PROCFS INTERFACE (/proc/counter_info) ===== */
static int counter_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "ASSIGNMENT-01 Counter Driver Statistics\n");
    seq_printf(m, "=======================================\n");
    seq_printf(m, "Device name     : /dev/%s (Misc major: 10)\n", DEVICE_NAME);
    seq_printf(m, "Current counter : %d\n", atomic_read(&counter_val));
    seq_printf(m, "Total reads     : %d\n", atomic_read(&total_reads));
    seq_printf(m, "Total resets    : %d\n", atomic_read(&total_resets));
    return 0;
}

static int counter_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}

static const struct proc_ops counter_proc_fops = {
    .proc_open    = counter_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* ===== FILE OPERATIONS LAYER ===== */

/*
 * Hàm counter_read: Trả về giá trị hiện tại của counter dưới dạng chuỗi thập phân
 *                   và tự động tăng counter lên 1 (auto-increment on read).
 * Xử lý đúng *ppos (file offset) để lệnh cat không bị lặp vô tận.
 */
static ssize_t counter_read(struct file *file, char __user *buf,
                            size_t len, loff_t *offset)
{
    char tmp_str[32];
    int val, str_len;
    int bytes_copied;

    /* Nếu offset > 0, nghĩa là đã đọc ở lần gọi trước, trả về 0 (EOF) */
    if (*offset > 0)
        return 0;

    val = atomic_read(&counter_val);
    str_len = snprintf(tmp_str, sizeof(tmp_str), "%d\n", val);

    if (len < str_len)
        return -EINVAL;

    if (copy_to_user(buf, tmp_str, str_len)) {
        pr_err("counter_driver: copy_to_user failed in read\n");
        return -EFAULT;
    }

    *offset += str_len;
    bytes_copied = str_len;

    /* Tự động tăng counter lên 1 và tăng bộ đếm thống kê read */
    atomic_inc(&counter_val);
    atomic_inc(&total_reads);

    pr_info("counter_driver: read counter=%d, auto-incremented\n", val);
    return bytes_copied;
}

/*
 * Hàm counter_write: Nhận chuỗi ASCII từ userspace, phân tích thành số nguyên
 *                    bằng kstrtoint() để set hoặc reset giá trị counter.
 */
static ssize_t counter_write(struct file *file, const char __user *buf,
                             size_t len, loff_t *offset)
{
    char kbuf[32];
    int new_val, ret;
    size_t copy_len = len;

    if (copy_len > sizeof(kbuf) - 1)
        copy_len = sizeof(kbuf) - 1;

    if (copy_from_user(kbuf, buf, copy_len)) {
        pr_err("counter_driver: copy_from_user failed in write\n");
        return -EFAULT;
    }
    kbuf[copy_len] = '\0';

    /* Xóa ký tự xuống dòng '\n' nếu có */
    if (copy_len > 0 && kbuf[copy_len - 1] == '\n')
        kbuf[copy_len - 1] = '\0';

    /* Phân tích chuỗi thành số nguyên an toàn bằng kstrtoint */
    ret = kstrtoint(kbuf, 10, &new_val);
    if (ret < 0) {
        pr_warn("counter_driver: invalid non-numeric input: \"%s\"\n", kbuf);
        return -EINVAL; /* Không phải số hợp lệ */
    }

    if (new_val < 0) {
        pr_warn("counter_driver: negative value not allowed: %d\n", new_val);
        return -EINVAL; /* Giá trị âm */
    }

    /* Cập nhật giá trị counter và tăng số lần write/reset */
    atomic_set(&counter_val, new_val);
    atomic_inc(&total_resets);

    pr_info("counter_driver: counter set to %d via write\n", new_val);
    return len;
}

static const struct file_operations counter_fops = {
    .owner   = THIS_MODULE,
    .read    = counter_read,
    .write   = counter_write,
};

/* Đăng ký miscdevice cấu trúc chuẩn của kernel */
static struct miscdevice counter_misc_device = {
    .minor   = MISC_DYNAMIC_MINOR, /* Kernel tự cấp minor number */
    .name    = DEVICE_NAME,        /* Tạo tự động /dev/counter */
    .fops    = &counter_fops,
};

/* ===== MODULE INIT & EXIT ===== */
static int __init counter_init(void)
{
    int ret;

    pr_info("counter_driver: initializing miscellaneous device driver\n");

    /* Đăng ký misc device (thay thế register_chrdev phức tạp) */
    ret = misc_register(&counter_misc_device);
    if (ret) {
        pr_err("counter_driver: misc_register failed with error %d\n", ret);
        return ret;
    }

    /* Tạo file thông tin /proc/counter_info */
    proc_entry = proc_create(PROCFS_NAME, 0444, NULL, &counter_proc_fops);
    if (!proc_entry)
        pr_warn("counter_driver: failed to create /proc/%s\n", PROCFS_NAME);

    pr_info("counter_driver: loaded successfully on /dev/%s (minor=%d)\n",
            DEVICE_NAME, counter_misc_device.minor);
    return 0;
}

static void __exit counter_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);
    misc_deregister(&counter_misc_device);
    pr_info("counter_driver: module unloaded cleanly\n");
}

module_init(counter_init);
module_exit(counter_exit);
