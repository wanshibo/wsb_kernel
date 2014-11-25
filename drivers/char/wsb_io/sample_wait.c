#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/sched.h>
#define DRIVER_NAME "io_driver"
static wait_queue_head_t queue;
unsigned char wait_condition;
ssize_t io_driver_read(struct file *filp, char __user *buf, size_t size, loff_t *fops)
{
	printk("wait write\n");
	wait_event_interruptible(queue, wait_condition != 0);
	wait_condition = 0;
	printk("\t writed\n");
	return 0;
}
ssize_t io_driver_write(struct file *filp, const char __user *buf, size_t size, loff_t *fpos)
{
	wait_condition = 1;
	wake_up_interruptible(&queue);
	
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