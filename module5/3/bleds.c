#include <linux/module.h>
#include <linux/configfs.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>
#include <linux/vt_kern.h>
#include <linux/timer.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yan Pristinskiy)");
MODULE_DESCRIPTION("Blinks keyboard LED");

#define PERMS 0660

#define BLINK_DELAY_MS 100          
#define RESTORE_LEDS  0xFF

static struct kobject *sfs_kobject;
static int fds; 

static struct timer_list timer;
static struct tty_driver* driver;
static int _kbledstatus = RESTORE_LEDS;
static int light_mode = 0;
static bool blinking = false;

static inline struct tty_struct* get_active_tty(void)
{
    if (!vc_cons[fg_console].d)
        return NULL;
    return vc_cons[fg_console].d->port.tty;
}

static void set_kbd_leds(int value)
{
    struct tty_struct *tty = get_active_tty();
    if (!driver || !tty)
        return;

    if (!driver->ops || !driver->ops->ioctl)
        return;

    (driver->ops->ioctl)(tty, KDSETLED, value);
}

static ssize_t sfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t sfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

static struct kobj_attribute sfs_attribute = __ATTR(mask, PERMS, sfs_show, sfs_store);

static void timer_func(struct timer_list *ptr);
static void start_bleds(void);
static void stop_bleds(void);

static ssize_t sfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", fds);
}

static ssize_t sfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int mode, mask = 0;

    if (kstrtoint(buf, 10, &mode) != 0)
        return -EINVAL;

    if (mode < 0 || mode > 3)
        return -EINVAL;

    fds = mode;

    switch (mode) {
    case 0:  mask = 0;        break; // off
    case 1:  mask = 0x02;     break; // NumLock
    case 2:  mask = 0x04;     break; // CapsLock
    case 3:  mask = 0x02|0x04;break; // Num + Caps
    }

    light_mode = mask;

    if (mode == 0)
        stop_bleds();
    else
        start_bleds();

    return count;

}

static void timer_func(struct timer_list *ptr)
{
    if (!blinking || light_mode == 0) {
        _kbledstatus = RESTORE_LEDS;
        set_kbd_leds(RESTORE_LEDS);
        return;
    }

    if (_kbledstatus == light_mode)
        _kbledstatus = RESTORE_LEDS;
    else
        _kbledstatus = light_mode;

    set_kbd_leds(_kbledstatus);

    mod_timer(&timer, jiffies + msecs_to_jiffies(BLINK_DELAY_MS));
}

static void start_bleds(void)
{
    struct tty_struct *tty;

    tty = get_active_tty();
    if (!tty) {
        printk(KERN_ERR "bleds: no active tty console\n");
        return;
    }

    driver = tty->driver;
    if (!driver) {
        printk(KERN_ERR "bleds: tty driver is NULL\n");
        return;
    }

    if (!blinking) {
        printk(KERN_INFO "bleds: start blinking, mask=%d\n", light_mode);
        blinking = true;
    }

    _kbledstatus = RESTORE_LEDS;
    mod_timer(&timer, jiffies + msecs_to_jiffies(BLINK_DELAY_MS));
}

static void stop_bleds(void)
{
    blinking = false;
    del_timer_sync(&timer);
    set_kbd_leds(RESTORE_LEDS);
    _kbledstatus = RESTORE_LEDS;
    printk(KERN_INFO "bleds: stopped blinking\n");
}

static int __init bleds_init(void)
{
    int error = 0;

    printk(KERN_INFO "bleds: module init\n");

    sfs_kobject = kobject_create_and_add("bleds", kernel_kobj);
    if (!sfs_kobject)
        return -ENOMEM;

    error = sysfs_create_file(sfs_kobject, &sfs_attribute.attr);
    if (error) {
        printk(KERN_WARNING "bleds: failed to create sysfs file /sys/kernel/bleds/mask\n");
        kobject_put(sfs_kobject);
        return error;
    }

    timer_setup(&timer, timer_func, 0);

    fds = 0;
    light_mode = 0;
    blinking = false;
    _kbledstatus = RESTORE_LEDS;

    printk(KERN_INFO "bleds: use echo 1..3 > /sys/kernel/bleds/mask, echo 0 to stop\n");
    return 0;
}

static void __exit bleds_exit(void)
{
    stop_bleds();

    if (sfs_kobject)
        kobject_put(sfs_kobject);

    printk(KERN_INFO "bleds: module exit\n");
}

module_init(bleds_init);
module_exit(bleds_exit);
