#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yan Pristinskiy");
MODULE_DESCRIPTION("Chardev userspace exchange");

#define DEVICE_NAME "chardev"
#define BUF_LEN     128
#define TEXT_LEN    64

static int major;
static struct class *cls;

static atomic_t already_open = ATOMIC_INIT(0);
static atomic_t open_counter = ATOMIC_INIT(0);

static char text[TEXT_LEN];    
static char msg[BUF_LEN];      

static int device_open(struct inode *inode, struct file *file)
{
    if (atomic_xchg(&already_open, 1))
        return -EBUSY;

    atomic_inc(&open_counter);
    try_module_get(THIS_MODULE);
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    atomic_set(&already_open, 0);
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset)
{
    size_t to_copy = min(length, (size_t)(TEXT_LEN - 1));

    memset(text, 0, sizeof(text));

    if (copy_from_user(text, buffer, to_copy))
        return -EFAULT;

    if (to_copy > 0 && text[to_copy - 1] == '\n')
        text[to_copy - 1] = '\0';
    else
        text[to_copy] = '\0';

    *offset = 0;

    return to_copy;
}

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
    int len;

    if (*offset == 0) {
        len = scnprintf(msg, sizeof(msg), "open_count=%d; text: %s\n", atomic_read(&open_counter), text);
    } else {
        len = strlen(msg);
    }

    if (*offset >= len)
        return 0;

    if (length > (len - *offset))
        length = len - *offset;

    if (copy_to_user(buffer, msg + *offset, length))
        return -EFAULT;

    *offset += length;
    return length;
}

static const struct file_operations chardev_fops = {
    .owner   = THIS_MODULE,
    .read    = device_read,
    .write   = device_write,
    .open    = device_open,
    .release = device_release,
};

static int __init chardev_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &chardev_fops);
    if (major < 0) {
        pr_err("chardev: register_chrdev failed: %d\n", major);
        return major;
    }

    cls = class_create(DEVICE_NAME);
    if (IS_ERR(cls)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(cls);
    }

    if (IS_ERR(device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME))) {
        class_destroy(cls);
        unregister_chrdev(major, DEVICE_NAME);
        return -EINVAL;
    }

    pr_info("chardev: created /dev/%s with major=%d\n", DEVICE_NAME, major);
    return 0;
}

static void __exit chardev_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("chardev: removed /dev/%s\n", DEVICE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);
