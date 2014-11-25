#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/timer.h>

#define DRIVER_NAME "io_driver"
static struct completion done;
static struct timespec time;
static struct timer_list timer;

ssize_t io_driver_read(struct file *filp, char __user *buf, size_t size, loff_t *fops)
{
	unsigned long start_jiffies;
	unsigned long end_jiffies;
	unsigned int  msec;
	
	printk("wait write\n");
	start_jiffies = jiffies;
	wait_for_completion(&done);
	add_timer(&timer);
	end_jiffies = jiffies;
	msec = jiffies_to_msecs(end_jiffies - start_jiffies);
	jiffies_to_timespec(end_jiffies - start_jiffies, &time);
	
	printk("writed msec = %dms\n", msec);
	printk("takes %ds, takes %ldns\n", (unsigned int)time.tv_sec, time.tv_nsec);
	
	return 0;
}
ssize_t io_driver_write(struct file *filp, const char __user *buf, size_t size, loff_t *fpos)
{
	complete(&done);
	return 0;
}

static struct file_operations io_driver_fops = {
	.owner = THIS_MODULE,
	.read  = io_driver_read,
	.write = io_driver_write,
};
static struct miscdevice io_driver = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DRIVER_NAME,
	.fops  = &io_driver_fops,
};

static void timer_proc(unsigned long arg)
{
	struct completion *done = (struct completion *)arg;
	timer.expires = jiffies + 3*HZ;
	//add_timer(&timer);
	complete(done);
}
static __init int io_driver_init(void)
{
	int ret_val;
	init_completion(&done);
	init_timer(&timer);
	timer.expires = jiffies + 3*HZ;
	timer.data = (long)(&done);
	timer.function = timer_proc;

	ret_val = misc_register(&io_driver);
	
	return 0;
}
static void io_driver_exit(void)
{
	del_timer_sync(&timer);
	misc_deregister(&io_driver);
}
module_init(io_driver_init);
module_exit(io_driver_exit);
MODULE_AUTHOR("wsb <andrinux@163.com>");
MODULE_LICENSE("GPL");