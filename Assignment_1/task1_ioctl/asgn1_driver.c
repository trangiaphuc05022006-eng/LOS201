/*
 * File: asgn1_driver.c
 * Author: Tran Gia Phuc - SE204417
 * Description: ASSIGNMENT-01 Task 1 - Extended Character Device Driver
 *              supporting 4 ioctl commands, mode switching, statistics,
 *              procfs/sysfs interfaces, and mutex synchronization.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/string.h>

#include "asgn1_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tran Gia Phuc - SE204417");
MODULE_DESCRIPTION("ASSIGNMENT-01: Character Device Driver with ioctl Interface");
MODULE_VERSION("1.0");

#define DEVICE_NAME     "asgn1"
#define CLASS_NAME      "asgn1_class"
#define BUFFER_SIZE     1024
#define DRIVER_MAJOR    241     /* Nằm trong khoảng 241-254 theo yêu cầu đề bài */
#define KERNEL_PREFIX   "[KERNEL] "
#define DRIVER_VER_STR  "asgn1_driver v1.0 - SE204417"

/* ===== DATA LAYER (Tầng dữ liệu trạng thái của Driver) ===== */
static int    major_number    = DRIVER_MAJOR;
static char   device_buffer[BUFFER_SIZE];
static int    buffer_len      = 0;
static int    open_count      = 0;
static int    write_count     = 0;
static int    read_count      = 0;
static int    last_write_size = 0;
static int    current_mode    = ASGN1_MODE_ECHO; /* Mặc định là Mode 1 (Echo mode) */

static struct class  *asgn1_class  = NULL;
static struct device *asgn1_device = NULL;
static struct proc_dir_entry *proc_entry = NULL;

/*
 * Mutex bảo vệ toàn bộ thao tác đọc/ghi vào device_buffer và các biến counter
 * nhằm tránh xung đột dữ liệu (race condition) khi nhiều tiến trình truy cập.
 */
static DEFINE_MUTEX(asgn1_mutex);

/* ===== PROCFS INTERFACE (/proc/asgn1_info) ===== */

/*
 * Hàm asgn1_proc_show: Xuất thông tin trạng thái của driver ra /proc/asgn1_info
 * Parameters:
 *   @m: Con trỏ cấu trúc seq_file quản lý bộ đệm xuất
 *   @v: Con trỏ dữ liệu riêng (không sử dụng)
 * Return: 0 khi thành công
 */
static int asgn1_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&asgn1_mutex);
    seq_printf(m, "ASSIGNMENT-01 Driver Statistics\n");
    seq_printf(m, "===============================\n");
    seq_printf(m, "Driver version  : %s\n", DRIVER_VER_STR);
    seq_printf(m, "Device node     : /dev/%s (major=%d)\n", DEVICE_NAME, major_number);
    seq_printf(m, "Current mode    : %d (%s)\n", current_mode,
               (current_mode == ASGN1_MODE_KERNEL) ? "Kernel mode" : "Echo mode");
    seq_printf(m, "Open count      : %d\n", open_count);
    seq_printf(m, "Write count     : %d\n", write_count);
    seq_printf(m, "Read count      : %d\n", read_count);
    seq_printf(m, "Buffer length   : %d bytes\n", buffer_len);
    seq_printf(m, "Last write size : %d bytes\n", last_write_size);
    if (buffer_len > 0)
        seq_printf(m, "Buffer content  : [%s]\n", device_buffer);
    else
        seq_printf(m, "Buffer content  : (empty)\n");
    mutex_unlock(&asgn1_mutex);
    return 0;
}

/*
 * Hàm asgn1_proc_open: Mở file /proc/asgn1_info sử dụng single_open
 * Parameters:
 *   @inode: Con trỏ inode của file trong procfs
 *   @file:  Con trỏ cấu trúc file đang mở
 * Return: 0 nếu thành công, mã lỗi âm nếu thất bại
 */
static int asgn1_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, asgn1_proc_show, NULL);
}

static const struct proc_ops asgn1_proc_fops = {
    .proc_open    = asgn1_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* ===== FILE OPERATIONS LAYER (Tầng xử lý thao tác thiết bị) ===== */

/*
 * Hàm asgn1_open: Được gọi khi userspace thực hiện open("/dev/asgn1", ...)
 * Parameters:
 *   @inode: Thông tin siêu dữ liệu của device node
 *   @file:  Đối tượng phiên làm việc của file mở
 * Return: 0 (luôn thành công)
 */
static int asgn1_open(struct inode *inode, struct file *file)
{
    mutex_lock(&asgn1_mutex);
    open_count++;
    pr_info("asgn1_driver: device opened (open_count=%d)\n", open_count);
    mutex_unlock(&asgn1_mutex);
    return 0;
}

/*
 * Hàm asgn1_release: Được gọi khi userspace đóng file descriptor bằng close()
 * Parameters:
 *   @inode: Thông tin siêu dữ liệu của device node
 *   @file:  Đối tượng phiên làm việc của file đóng
 * Return: 0 (luôn thành công)
 */
static int asgn1_release(struct inode *inode, struct file *file)
{
    pr_info("asgn1_driver: device closed\n");
    return 0;
}

/*
 * Hàm asgn1_read: Đọc dữ liệu từ device_buffer trả về không gian người dùng
 * Parameters:
 *   @file:   Con trỏ cấu trúc file
 *   @buf:    Bộ đệm trong không gian người dùng (__user) nhận dữ liệu
 *   @len:    Số byte tối đa mà userspace yêu cầu đọc
 *   @offset: Con trỏ vị trí đọc hiện tại trong buffer
 * Return: Số byte thực tế đọc được (0 nếu chạm EOF), hoặc -EFAULT nếu lỗi vùng nhớ
 */
static ssize_t asgn1_read(struct file *file, char __user *buf,
                          size_t len, loff_t *offset)
{
    int bytes_read = 0;

    mutex_lock(&asgn1_mutex);
    if (*offset >= buffer_len) {
        mutex_unlock(&asgn1_mutex);
        return 0; /* Đã đọc hết dữ liệu (EOF) */
    }

    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    if (copy_to_user(buf, device_buffer + *offset, len)) {
        mutex_unlock(&asgn1_mutex);
        pr_err("asgn1_driver: copy_to_user failed in read\n");
        return -EFAULT;
    }

    *offset += len;
    bytes_read = len;
    read_count++;

    pr_info("asgn1_driver: read %d bytes (read_count=%d)\n",
            bytes_read, read_count);
    mutex_unlock(&asgn1_mutex);

    return bytes_read;
}

/*
 * Hàm asgn1_write: Nhận dữ liệu từ userspace và lưu vào device_buffer theo chế độ
 *                  - Mode 0 (Kernel mode): Tự động thêm tiền tố "[KERNEL] "
 *                  - Mode 1 (Echo mode)  : Lưu dữ liệu nguyên bản
 *                  Lưu ý: Ghi trực tiếp vào device_buffer (.bss) thay vì cấp phát
 *                  mảng 1024 bytes trên Kernel Stack để tránh lỗi tràn frame size.
 * Parameters:
 *   @file:   Con trỏ cấu trúc file
 *   @buf:    Bộ đệm dữ liệu nguồn từ userspace (__user)
 *   @len:    Số byte mà userspace muốn ghi
 *   @offset: Con trỏ vị trí ghi
 * Return: Số byte từ userspace đã xử lý, hoặc -EFAULT nếu lỗi copy_from_user
 */
static ssize_t asgn1_write(struct file *file, const char __user *buf,
                           size_t len, loff_t *offset)
{
    size_t copy_len = len;
    size_t prefix_len = strlen(KERNEL_PREFIX);
    size_t max_payload;

    mutex_lock(&asgn1_mutex);

    if (current_mode == ASGN1_MODE_KERNEL) {
        max_payload = (BUFFER_SIZE - 1) - prefix_len;
        if (copy_len > max_payload)
            copy_len = max_payload;

        memcpy(device_buffer, KERNEL_PREFIX, prefix_len);
        if (copy_from_user(device_buffer + prefix_len, buf, copy_len)) {
            mutex_unlock(&asgn1_mutex);
            pr_err("asgn1_driver: copy_from_user failed in write (kernel mode)\n");
            return -EFAULT;
        }
        buffer_len = prefix_len + copy_len;
    } else {
        max_payload = BUFFER_SIZE - 1;
        if (copy_len > max_payload)
            copy_len = max_payload;

        if (copy_from_user(device_buffer, buf, copy_len)) {
            mutex_unlock(&asgn1_mutex);
            pr_err("asgn1_driver: copy_from_user failed in write (echo mode)\n");
            return -EFAULT;
        }
        buffer_len = copy_len;
    }

    /* Xóa ký tự xuống dòng '\n' ở cuối (nếu ghi bằng lệnh echo) */
    if (buffer_len > 0 && device_buffer[buffer_len - 1] == '\n')
        buffer_len--;

    device_buffer[buffer_len] = '\0';
    write_count++;
    last_write_size = buffer_len;

    pr_info("asgn1_driver: write in mode=%d, stored %d bytes: [%s] (write_count=%d)\n",
            current_mode, buffer_len, device_buffer, write_count);

    mutex_unlock(&asgn1_mutex);

    return len;
}

/*
 * Hàm asgn1_ioctl: Xử lý 4 lệnh điều khiển ioctl từ không gian người dùng
 * Parameters:
 *   @file: Con trỏ cấu trúc file đang mở
 *   @cmd:  Mã lệnh ioctl 32-bit được tạo bởi macro _IO/_IOR/_IOW
 *   @arg:  Tham số đi kèm (giá trị hoặc địa chỉ con trỏ vùng nhớ userspace)
 * Return: 0 nếu thành công, -ENOTTY nếu sai lệnh, -EINVAL nếu sai tham số,
 *         -EFAULT nếu lỗi truyền dữ liệu qua ranh giới User/Kernel
 */
static long asgn1_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct asgn1_stats stats;
    int new_mode;
    char ver_buf[ASGN1_VERSION_MAX_LEN];

    /* Kiểm tra Magic Number để đảm bảo lệnh thuộc về asgn1_driver */
    if (_IOC_TYPE(cmd) != ASGN1_IOC_MAGIC) {
        pr_warn("asgn1_driver: ioctl invalid magic 0x%x (expected 0x%x)\n",
                _IOC_TYPE(cmd), ASGN1_IOC_MAGIC);
        return -ENOTTY;
    }

    switch (cmd) {
    case ASGN1_RESET_BUFFER:
        mutex_lock(&asgn1_mutex);
        memset(device_buffer, 0, BUFFER_SIZE);
        buffer_len  = 0;
        write_count = 0;
        read_count  = 0;
        mutex_unlock(&asgn1_mutex);
        pr_info("asgn1_driver: ioctl ASGN1_RESET_BUFFER executed (buffer cleared, write_count=0, read_count=0)\n");
        break;

    case ASGN1_GET_STATS:
        mutex_lock(&asgn1_mutex);
        stats.open_count      = open_count;
        stats.write_count     = write_count;
        stats.read_count      = read_count;
        stats.buffer_len      = buffer_len;
        stats.last_write_size = last_write_size;
        mutex_unlock(&asgn1_mutex);

        if (copy_to_user((struct asgn1_stats __user *)arg, &stats, sizeof(stats))) {
            pr_err("asgn1_driver: ioctl ASGN1_GET_STATS copy_to_user failed\n");
            return -EFAULT;
        }
        pr_info("asgn1_driver: ioctl ASGN1_GET_STATS executed (open=%d, write=%d, read=%d, len=%d, last_write=%d)\n",
                stats.open_count, stats.write_count, stats.read_count,
                stats.buffer_len, stats.last_write_size);
        break;

    case ASGN1_SET_MODE:
        if (copy_from_user(&new_mode, (int __user *)arg, sizeof(int))) {
            pr_err("asgn1_driver: ioctl ASGN1_SET_MODE copy_from_user failed\n");
            return -EFAULT;
        }
        if (new_mode != ASGN1_MODE_KERNEL && new_mode != ASGN1_MODE_ECHO) {
            pr_warn("asgn1_driver: ioctl ASGN1_SET_MODE invalid mode=%d\n", new_mode);
            return -EINVAL;
        }
        mutex_lock(&asgn1_mutex);
        current_mode = new_mode;
        mutex_unlock(&asgn1_mutex);
        pr_info("asgn1_driver: ioctl ASGN1_SET_MODE set to %d (%s)\n",
                current_mode,
                (current_mode == ASGN1_MODE_KERNEL) ? "Kernel mode" : "Echo mode");
        break;

    case ASGN1_GET_VERSION:
        memset(ver_buf, 0, ASGN1_VERSION_MAX_LEN);
        strscpy(ver_buf, DRIVER_VER_STR, ASGN1_VERSION_MAX_LEN);
        if (copy_to_user((char __user *)arg, ver_buf, ASGN1_VERSION_MAX_LEN)) {
            pr_err("asgn1_driver: ioctl ASGN1_GET_VERSION copy_to_user failed\n");
            return -EFAULT;
        }
        pr_info("asgn1_driver: ioctl ASGN1_GET_VERSION executed (version=\"%s\")\n",
                ver_buf);
        break;

    default:
        pr_warn("asgn1_driver: ioctl unknown command 0x%x\n", cmd);
        return -ENOTTY;
    }

    return 0;
}

/* Bảng ánh xạ các hàm xử lý vào VFS (Virtual File System) */
static const struct file_operations asgn1_fops = {
    .owner          = THIS_MODULE,
    .open           = asgn1_open,
    .release        = asgn1_release,
    .read           = asgn1_read,
    .write          = asgn1_write,
    .unlocked_ioctl = asgn1_ioctl,
};

/* ===== MODULE INIT & EXIT LAYER ===== */

/*
 * Hàm asgn1_init: Khởi tạo và đăng ký character driver khi nạp bằng insmod
 * Return: 0 nếu thành công, mã lỗi âm nếu thất bại
 */
static int __init asgn1_init(void)
{
    int ret;

    pr_info("asgn1_driver: initializing module (version: %s)\n", DRIVER_VER_STR);

    ret = register_chrdev(major_number, DEVICE_NAME, &asgn1_fops);
    if (ret < 0) {
        pr_err("asgn1_driver: register_chrdev failed with error %d\n", ret);
        return ret;
    }

    asgn1_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(asgn1_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(asgn1_class);
    }

    asgn1_device = device_create(asgn1_class, NULL,
                                 MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(asgn1_device)) {
        class_destroy(asgn1_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(asgn1_device);
    }

    proc_entry = proc_create("asgn1_info", 0444, NULL, &asgn1_proc_fops);
    if (!proc_entry)
        pr_warn("asgn1_driver: failed to create /proc/asgn1_info\n");

    pr_info("asgn1_driver: loaded successfully on /dev/%s (major=%d, default_mode=%d)\n",
            DEVICE_NAME, major_number, current_mode);
    return 0;
}

/*
 * Hàm asgn1_exit: Giải phóng toàn bộ tài nguyên khi gỡ module bằng rmmod
 */
static void __exit asgn1_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);
    device_destroy(asgn1_class, MKDEV(major_number, 0));
    class_destroy(asgn1_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    pr_info("asgn1_driver: module unloaded cleanly\n");
}

module_init(asgn1_init);
module_exit(asgn1_exit);
