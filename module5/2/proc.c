#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#define PROC_FILENAME      "proc"
#define PROC_FILE_MODE     0644
#define MAX_BUFFER_SIZE    256

static size_t len, temp;                 
static char *msg;
static DEFINE_MUTEX(proc_mutex);

MODULE_LICENSE("CustomLicence");
MODULE_AUTHOR("Yan Pristinskiy");
MODULE_DESCRIPTION("Proc module");

static ssize_t read_proc(struct file *filp, char __user *buf, size_t count, loff_t *offp);
static ssize_t write_proc(struct file *filp, const char __user *buf, size_t count, loff_t *offp);
static void create_proc_entry(void);

static ssize_t read_proc(struct file *filp, char __user *buf, size_t count, loff_t *offp) 
{
    ssize_t bytes_to_read;
    
    mutex_lock(&proc_mutex);
    
    if (count > temp) {
        count = temp;
    }
    
    bytes_to_read = count;
    temp = temp - bytes_to_read;
    
    if (copy_to_user(buf, msg, bytes_to_read)) {
        printk(KERN_WARNING "PROC failed to copy data into userspace\n");
        mutex_unlock(&proc_mutex);
        return -EFAULT;
    }
    
    if (bytes_to_read == 0) {
        temp = len;
    }
    
    mutex_unlock(&proc_mutex);
    return bytes_to_read;
}

static ssize_t write_proc(struct file *filp, const char __user *buf, size_t count, loff_t *offp) 
{
    size_t bytes_to_copy;
    
    mutex_lock(&proc_mutex);
    
    bytes_to_copy = min(count, (size_t)MAX_BUFFER_SIZE);
    
    if (copy_from_user(msg, buf, bytes_to_copy)) {
        printk(KERN_WARNING  "PROC failed to copy data from userspace\n");
        mutex_unlock(&proc_mutex);
        return -EFAULT;
    }
    
    len = bytes_to_copy;
    temp = len;
    
    mutex_unlock(&proc_mutex);
    return bytes_to_copy;
}

static const struct proc_ops proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};


static void create_proc_entry(void) 
{
    proc_create(PROC_FILENAME, PROC_FILE_MODE, NULL, &proc_fops);
    
    msg = kmalloc(MAX_BUFFER_SIZE, GFP_KERNEL);
    if (!msg) {
        printk(KERN_ERR "PROC failed to allocate memory\n");
        return;
    }
    
    memset(msg, 0, MAX_BUFFER_SIZE);
}

static int __init proc_init(void) 
{
    create_proc_entry();  
    if (!msg) {
        printk(KERN_ERR "PROC initialization failed\n");
        return -ENOMEM;
    }
        printk(KERN_INFO "PROC successfully initialized!\n");
    return 0;
}

static void __exit proc_cleanup(void) 
{
    remove_proc_entry(PROC_FILENAME, NULL);
    
    if (msg) {
        kfree(msg);
        msg = NULL;
    }
    
    printk(KERN_INFO "PROC successfully cleared!\n");
}

module_init(proc_init);
module_exit(proc_cleanup);