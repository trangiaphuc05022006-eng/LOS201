/* ---- includes: kernel features the driver uses ---- */
#include <linux/module.h>    /* module_init, MODULE_LICENSE, ... */
#include <linux/fs.h>        /* file_operations, register_chrdev */
#include <linux/uaccess.h>   /* copy_to_user, copy_from_user */
#include <linux/device.h>    /* class_create, device_create */
#include <linux/mutex.h>     /* the lock */
#include <linux/string.h>    /* strlen, memcpy, memset, strscpy */
#include "asgn1_ioctl.h"     /* my header: ioctl commands, stats struct, mode names */

/* ---- constants ---- */
#define DEVICE_NAME    "asgn1"
#define CLASS_NAME     "asgn1_class"
#define BUFFER_SIZE    1024
#define DRIVER_MAJOR   250                          /* allowed range: 241 to 254 */
#define DRIVER_VERSION "asgn1_driver v1.0 - SE203155"   /* must fit in 64 bytes incl. the '\0' */
#define LOG_PREFIX     "asgn1_driver: "             /* in front of every log line */
#define KERNEL_PREFIX  "[KERNEL]"                   /* added to stored data in kernel mode */
#define ASGN1_MODE_KERNEL 0
#define ASGN1_MODE_ECHO   1

/* ---- global variables ---- */
static char buffer[BUFFER_SIZE];          /* the data buffer */
static int buffer_len = 0;                /* number of bytes stored in the buffer */
static int open_count = 0;                /* number of times the device was opened */
static int read_count = 0;                /* number of read() calls (EOF call included) */
static int write_count = 0;               /* number of successful write() calls */
static int last_write_size = 0;           /* bytes the user sent in the last write */
static int current_mode = ASGN1_MODE_ECHO; /* starts in echo mode */

static struct class  *asgn1_class  = NULL;  /* the device class */
static struct device *asgn1_device = NULL;  /* the device */
static DEFINE_MUTEX(asgn1_mutex);           /* protects the buffer, counters and mode */


/*
 * asgn1_open - called when the device is opened
 * @inode: the inode of the device
 * @file:  the open file instance
 *
 * Counts the open under the lock and writes a line to the kernel log.
 *
 * Return: 0 (always succeeds)
 */
static int asgn1_open(struct inode *inode, struct file *file)
{
    int count;

    mutex_lock(&asgn1_mutex);
    open_count++;
    count = open_count;                 /* local copy, so the log is not racy */
    mutex_unlock(&asgn1_mutex);

    printk(KERN_INFO LOG_PREFIX "Device opened %d times\n", count);
    return 0;
}

/*
 * asgn1_release - called when the device is closed
 * @inode: the inode of the device
 * @file:  the open file instance
 *
 * Only writes a line to the kernel log. open_count is a running total, so a
 * close does not change it.
 *
 * Return: 0 (always succeeds)
 */
static int asgn1_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO LOG_PREFIX "Device closed\n");
    return 0;
}

/*
 * asgn1_write - store user data in the buffer, according to the current mode
 * @file:   the open file instance
 * @buf:    user-space memory holding the data to write
 * @len:    number of bytes the user sent
 * @offset: unused, every write replaces the whole buffer
 *
 * Echo mode stores the data as it is. Kernel mode stores "[KERNEL]" first.
 * Data that does not fit is cut. A successful write counts in write_count
 * and sets last_write_size to the bytes the user sent.
 *
 * Return: bytes accepted from the user (not buffer_len), or -EFAULT
 */
static ssize_t asgn1_write(struct file *file, const char __user *buf,
                           size_t len, loff_t *offset)
{
    size_t head_len;
    size_t room;

    mutex_lock(&asgn1_mutex);

    /* 0 in echo mode, 8 in kernel mode (strlen("[KERNEL]")) */
    head_len = (current_mode == ASGN1_MODE_KERNEL) ? strlen(KERNEL_PREFIX) : 0;
    room = BUFFER_SIZE - 1 - head_len;         /* 1 byte kept for '\0' */

    if (len > room) {
        printk(KERN_WARNING LOG_PREFIX "write too large, truncating\n");
        len = room;                            /* only accept what still fits */
    }

    memcpy(buffer, KERNEL_PREFIX, head_len);   /* kernel-to-kernel copy, 0 bytes in echo mode */
    if (copy_from_user(buffer + head_len, buf, len)) {
        mutex_unlock(&asgn1_mutex);            /* never return while holding the lock */
        return -EFAULT;
    }

    buffer_len = head_len + len;               /* what is stored, not what was sent */
    buffer[buffer_len] = '\0';

    write_count++;                             /* only reached if the copy succeeded */
    last_write_size = len;

    printk(KERN_INFO LOG_PREFIX "received %zu bytes, mode=%d\n",
           len, current_mode);                 /* no buffer contents in dmesg */
    mutex_unlock(&asgn1_mutex);

    return len;                                /* bytes consumed from the user */
}

/*
 * asgn1_read - copy stored data to the user, counting every call
 * @file:   the open file instance
 * @buf:    user-space memory the data is copied to
 * @len:    number of bytes the user asked for
 * @offset: where this reader currently is in the data
 *
 * Every call counts in read_count, including the final one that reports
 * end of data.
 *
 * Return: bytes copied; 0 at end of data; -EFAULT if the copy fails
 */
static ssize_t asgn1_read(struct file *file, char __user *buf,
                          size_t len, loff_t *offset)
{
    int sent;

    mutex_lock(&asgn1_mutex);
    read_count++;

    if (*offset >= buffer_len) {        /* nothing left: end of data */
        mutex_unlock(&asgn1_mutex);
        return 0;
    }

    if (len > buffer_len - *offset)     /* fewer bytes left than asked for */
        len = buffer_len - *offset;

    if (copy_to_user(buf, buffer + *offset, len)) {
        printk(KERN_ERR LOG_PREFIX "copy_to_user failed\n");
        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
    }

    *offset += len;
    sent = len;
    printk(KERN_INFO LOG_PREFIX "sent %d bytes to user\n", sent);
    mutex_unlock(&asgn1_mutex);
    return sent;
}

/*
 * asgn1_ioctl - handle the four control commands
 * @file: the open file instance
 * @cmd:  the command number (from asgn1_ioctl.h)
 * @arg:  user-space address of the command's data, if it has any
 *
 * RESET_BUFFER empties the buffer and zeroes read_count and write_count
 * (open_count is left alone). GET_STATS returns a snapshot of the counters.
 * SET_MODE selects echo (1) or kernel (0) mode. GET_VERSION returns the
 * version string in a 64-byte buffer. Unknown commands return -ENOTTY.
 *
 * Return: 0 on success, -EFAULT on a failed copy, -EINVAL for a bad mode,
 *         -ENOTTY for an unknown command
 */
static long asgn1_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct asgn1_stats stats;
    char version[ASGN1_VERSION_LEN] = {0};     /* zero-filled: no stack data leaks */
    int new_mode;

    switch (cmd) {
    case ASGN1_RESET_BUFFER:
        mutex_lock(&asgn1_mutex);
        memset(buffer, 0, sizeof(buffer));
        buffer_len = 0;
        read_count = 0;
        write_count = 0;
        last_write_size = 0;
        mutex_unlock(&asgn1_mutex);
        printk(KERN_INFO LOG_PREFIX "ioctl RESET_BUFFER: buffer and counters cleared\n");
        break;

    case ASGN1_GET_STATS:
        mutex_lock(&asgn1_mutex);              /* snapshot: all five from the same moment */
        stats.open_count = open_count;
        stats.read_count = read_count;
        stats.write_count = write_count;
        stats.buffer_len = buffer_len;
        stats.last_write_size = last_write_size;
        mutex_unlock(&asgn1_mutex);

        if (copy_to_user((struct asgn1_stats __user *)arg, &stats, sizeof(stats)))
            return -EFAULT;
        printk(KERN_INFO LOG_PREFIX "ioctl GET_STATS: stats sent to user\n");
        break;

    case ASGN1_SET_MODE:
        if (copy_from_user(&new_mode, (int __user *)arg, sizeof(new_mode)))
            return -EFAULT;
        if (new_mode != ASGN1_MODE_KERNEL && new_mode != ASGN1_MODE_ECHO)
            return -EINVAL;                    /* only 0 and 1 are valid modes */

        mutex_lock(&asgn1_mutex);
        current_mode = new_mode;
        mutex_unlock(&asgn1_mutex);
        printk(KERN_INFO LOG_PREFIX "ioctl SET_MODE: mode set to %d\n", new_mode);
        break;

    case ASGN1_GET_VERSION:
        strscpy(version, DRIVER_VERSION, sizeof(version));   /* never longer than 64 */
        if (copy_to_user((char __user *)arg, version, sizeof(version)))
            return -EFAULT;
        printk(KERN_INFO LOG_PREFIX "ioctl GET_VERSION: version sent to user\n");
        break;

    default:
        printk(KERN_WARNING LOG_PREFIX "ioctl: unknown command 0x%x\n", cmd);
        return -ENOTTY;
    }
    return 0;
}


static struct file_operations asgn1_fops = {
    .owner          = THIS_MODULE,
    .open           = asgn1_open,
    .release        = asgn1_release,
    .read           = asgn1_read,
    .write          = asgn1_write,
    .unlocked_ioctl = asgn1_ioctl,      /* not .ioctl */
};

/*
 * asgn1_init - runs on insmod: registers the driver and creates /dev/asgn1
 *
 * Return: 0 on success, a negative error code on failure (earlier steps are
 *         undone before returning)
 */
static int __init asgn1_init(void)
{
    int ret;

    pr_info(LOG_PREFIX "initializing module\n");

    ret = register_chrdev(DRIVER_MAJOR, DEVICE_NAME, &asgn1_fops);
    if (ret < 0) {
        pr_err(LOG_PREFIX "register_chrdev failed: %d\n", ret);
        return ret;
    }

    asgn1_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(asgn1_class)) {
        unregister_chrdev(DRIVER_MAJOR, DEVICE_NAME);
        return PTR_ERR(asgn1_class);
    }

    asgn1_device = device_create(asgn1_class, NULL,
                                 MKDEV(DRIVER_MAJOR, 0), NULL, DEVICE_NAME);
    if (IS_ERR(asgn1_device)) {
        class_destroy(asgn1_class);
        unregister_chrdev(DRIVER_MAJOR, DEVICE_NAME);
        return PTR_ERR(asgn1_device);
    }

    pr_info(LOG_PREFIX "loaded, major=%d\n", DRIVER_MAJOR);
    return 0;
}

/*
 * asgn1_exit - runs on rmmod: undoes asgn1_init in reverse order
 */
static void __exit asgn1_exit(void)
{
    device_destroy(asgn1_class, MKDEV(DRIVER_MAJOR, 0));
    class_destroy(asgn1_class);
    unregister_chrdev(DRIVER_MAJOR, DEVICE_NAME);
    pr_info(LOG_PREFIX "unloaded, cleanup done\n");
}

module_init(asgn1_init);
module_exit(asgn1_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tran Hoang Khoi - SE203155");        
MODULE_DESCRIPTION("ASGN1: character driver with an ioctl interface");