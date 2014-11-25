#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/sched.h>
#define DRIVER_NAME "io_driver_async"
static wait_queue_head_t queue;
static unsigned char wait_condition;
static struct fasync_struct *async_irq;

static ssize_t io_driver_read(struct file *filp, char __user *buf, size_t size, loff_t *fops)
{
	printk("wait write\n");
	wait_event_interruptible(queue, wait_condition != 0);
	wait_condition = 0;
	printk("\t writed\n");
	return 0;
}
static ssize_t io_driver_write(struct file *filp, const char __user *buf, size_t size, loff_t *fpos)
{
	wait_condition = 1;
	wake_up_interruptible(&queue);
	if (async_irq)
		kill_fasync(&async_irq, SIGIO, POLL_IN);
	return 0;
}


static int io_driver_async(int fd, struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode, &async_irq);
}

static int io_driver_close(struct inode *inde, struct file *filp)
{
	io_driver_async(-1, filp, 0);
	return 0;
}

static struct file_operations io_driver_fops = {
	.owner = THIS_MODULE,
	.read  = io_driver_read,
	.write = io_driver_write,
	.fasync = io_driver_async,
	.release = io_driver_close,
};
static struct miscdevice io_driver = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DRIVER_NAME,
	.fops  = &io_driver_fops,
};

static __init int io_driver_init(void)
{
	int ret_val;
	init_waitqueue_head(&queue);
	
	ret_val = misc_register(&io_driver);
	
	return 0;
}
static void io_driver_exit(void)
{
	misc_deregister(&io_driver);
}
module_init(io_driver_init);
module_exit(io_driver_exit);
MODULE_AUTHOR("wsb <andrinux@163.com>");
MODULE_LICENSE("GPL");
