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
#include <linux/workqueue.h>

#define DRIVER_NAME "io_driver_wq"


static struct work_struct wk;
static struct workqueue_struct *wq;
static struct completion done;

typedef struct io_driver
{
	struct work_struct wk;
	struct workqueue_struct *wq;
	struct completion wk_done;
}WORK_T;

WORK_T gp_wk;

void io_driver_wk_proc(struct work_struct *wk)
{
	printk("in work!\n");
	complete(&gp_wk.wk_done);
}

static void io_driver_wk_init(void)
{
	gp_wk.wq = create_workqueue("aaaa");
	INIT_WORK(&gp_wk.wk, io_driver_wk_proc);
	init_completion(&gp_wk.wk_done);
}
static void io_driver_wk_exit(void)
{
	flush_workqueue(gp_wk.wq);
	destroy_workqueue(gp_wk.wq);
}
ssize_t io_driver_read(struct file *filp, char __user *buf, size_t size, loff_t *fops)
{
	unsigned long start_jiffies;
	unsigned long end_jiffies;
	unsigned int  msec = 0;
	
	printk("wait write.........\n");
	start_jiffies = jiffies;
	
	wait_for_completion_interruptible(&gp_wk.wk_done);

	end_jiffies = jiffies;
	msec = jiffies_to_msecs(end_jiffies - start_jiffies);
	printk("1writed msec = %dms\n", msec);
	
	return 0;
}
ssize_t io_driver_write(struct file *filp, const char __user *buf, size_t size, loff_t *fpos)
{
	queue_work(gp_wk.wq, &gp_wk.wk);
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
	io_driver_wk_init();
	ret_val = misc_register(&io_driver);
	
	return 0;
}
static void io_driver_exit(void)
{
	io_driver_wk_exit();
	misc_deregister(&io_driver);
}
module_init(io_driver_init);
module_exit(io_driver_exit);
MODULE_AUTHOR("wsb <andrinux@163.com>");
MODULE_LICENSE("GPL");