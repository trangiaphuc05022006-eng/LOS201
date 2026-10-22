#include <linux/module.h>
#include <linux/kernel.h>        /* kstrtoint */
#include <linux/fs.h>
#include <linux/miscdevice.h>    /* struct miscdevice, misc_register */
#include <linux/uaccess.h>       /* copy_to_user, copy_from_user */
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DEVICE_NAME "counter"                     /* becomes /dev/counter */

static atomic_t counter     = ATOMIC_INIT(0);     /* the value users read and set */
static atomic_t read_total  = ATOMIC_INIT(0);     /* reads that returned data */
static atomic_t write_total = ATOMIC_INIT(0);     /* successful writes */
static struct proc_dir_entry *proc_entry;

/* counter_read - read the current value and increment the counter
 * Return: number of bytes read, 0 at end of data, -EINVAL if len too small,
 *         -EFAULT if copy_to_user fails
 */
static ssize_t counter_read(struct file *file, char __user *buf,
                            size_t len, loff_t *ppos)
{
    char tmp[16];
    int n, val;

    /* 1. *ppos > 0: this open file already got its data -> return 0 (end of data), NO increment */
    if (*ppos > 0) {
        return 0;   /* already read: EOF */
    }

    /* 2. len smaller than the longest possible output (11 bytes)? return -EINVAL, before touching the counter */
    if (len < 11) { 
        return -EINVAL;   /* not enough space for the output */
    }
   
    /* 3. val = current value, and add 1 to the counter */
    val = atomic_fetch_add(1, &counter);   /* get current value and increment */
    
    /* 4. count it in read_total */
    atomic_inc(&read_total);   /* increment total reads */
    
    /* 5. n = snprintf of val plus a newline into tmp */
    n = snprintf(tmp, sizeof(tmp), "%d\n", val);   /* format the value with newline */
    
    /* 6. copy tmp to the user; on failure return -EFAULT */
    if (copy_to_user(buf, tmp, n)) { 
        return -EFAULT;   /* failed to copy to user */
    }
    printk(KERN_INFO "counter_driver: read returned %d\n", val);
    *ppos += n;   /* advance the file position */
    return n;     /* return number of bytes read */
}

/* counter_write - write a new value to the counter
 * @file:   the open file instance
 * @buf:    user-space memory the data is copied from
 * @len:    number of bytes the user wants to write
 * @ppos:   where this writer currently is in the data
 *
 * Return: number of bytes written, -EINVAL if len too small or invalid value,
 *         -EFAULT if copy_from_user fails
 */
static ssize_t counter_write(struct file *file, const char __user *buf,
                             size_t len, loff_t *ppos)
{
    char kbuf[16];
    int val;

    /* 1. len too big for kbuf? return -EINVAL */
    if (len >= sizeof(kbuf)) { 
        return -EINVAL;
    }

    /* 2. copy_from_user into kbuf (-EFAULT on failure), then put '\0' at the end */
    if (copy_from_user(kbuf, buf, len)) { 
        return -EFAULT;
    }
    kbuf[len] = '\0';

    /* 3. kstrtoint(kbuf, 10, &val); any error -> -EINVAL */
    if (kstrtoint(kbuf, 10, &val)) {
        return -EINVAL;   /* if conversion fails, return -EINVAL */
    }

    /* 4. val < 0 -> -EINVAL */
    if (val < 0) {
        return -EINVAL;
    }
    /* 5. atomic_set the counter; count it in write_total */
    atomic_set(&counter, val);
    printk(KERN_INFO "counter_driver: write set counter to %d\n", val);
    atomic_inc(&write_total);
    
    /* 6. return len */
    return len;
}

/* counter_proc_show - show the current value and totals in /proc/counter_info
 * @m:  the seq_file to write to
 * @v:  ignored
 *
 * Return: 0 on success
 */
static int counter_proc_show(struct seq_file *m, void *v)
{
    /* three labelled lines using atomic_read: current value, total reads, total writes */
    seq_printf(m, "current value: %d\n", atomic_read(&counter));
    seq_printf(m, "total reads: %d\n", atomic_read(&read_total));
    seq_printf(m, "total writes: %d\n", atomic_read(&write_total));
    return 0;
}

static const struct file_operations counter_fops = {
    .owner = THIS_MODULE,
    .read = counter_read,   
    .write = counter_write,
};

static struct miscdevice counter_misc = {
    .minor = MISC_DYNAMIC_MINOR,        /* let the kernel choose: MISC_DYNAMIC_MINOR */
    .name  = "counter",
    .fops  = &counter_fops,             /* address of the table above */
    .mode  = 0666,                      /* anyone may open /dev/counter */
};

/* counter_init - initialize the counter module
 * Return: 0 on success, negative error code on failure
 */
static int __init counter_init(void)
{   int ret;

    /* 1. misc_register(&counter_misc); on failure log and return the error */
    ret = misc_register(&counter_misc);
    if(ret) {  
        printk(KERN_ERR "counter_driver: Failed to register misc device\n");
        return ret;  /* or the actual error code returned by misc_register */
    }

    /* 2. create /proc/counter_info ; on failure undo step 1, return -ENOMEM */
    proc_entry = proc_create_single("counter_info", 0444, NULL, counter_proc_show);
    if (!proc_entry) {
        misc_deregister(&counter_misc);
        printk(KERN_ERR "counter_driver: Failed to create /proc/counter_info\n");
        return -ENOMEM;
    }
    /* 3. log "loaded", return 0 */
    printk(KERN_INFO "counter_driver: module loaded\n");
    return 0;
}

/* counter_exit - cleanup the counter module
 */
static void __exit counter_exit(void)
{
    /* undo in REVERSE order: proc entry first, then misc_deregister; log "unloaded" */
    remove_proc_entry("counter_info", NULL);
    misc_deregister(&counter_misc);
    printk(KERN_INFO "counter_driver: module unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tran Hoang Khoi - SE203155");
MODULE_DESCRIPTION("COUNTER: misc character device with an auto-incrementing counter");