#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kfifo.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#define DRIVER_NAME "io_driver_wq"
static wait_queue_head_t queue;
static unsigned char wait_condition;
static struct kfifo data_fifo;
#define fifo_size 16
static struct work_struct wk;
static struct workqueue_struct *wq;
static spinlock_t fifo_lock;

static void data_wk_proc(struct work_struct *wk)
{
	wait_condition = 1;
	wake_up_interruptible(&queue);
}


ssize_t io_driver_read(struct file *filp, char __user *buf, size_t size, loff_t *fops)
{
	int ret_val = 0;
	unsigned int copied = 0;
	wait_event_interruptible(queue, wait_condition != 0);
	//wait_event_interruptible_timeout(queue, wait_condition != 0, 3*HZ);
	wait_condition = 0;
	if (kfifo_is_empty(&data_fifo))
	{
		printk("fifo is empty!\n");
		return 0;
	}
	ret_val = kfifo_to_user(&data_fifo, (void *)buf, size, &copied);
	if (ret_val != 0)
	{
		printk("get data from fifo error!\n");
	}
	{
		printk("get %d byte from fifo\n", copied);
	}
	return copied;
}

ssize_t io_driver_write(struct file *filp, const char __user *buf, size_t size, loff_t *fpos)
{
	int ret_val = 0;
	unsigned int copied = 0;
	//if (!kfifo_is_empty(&data_fifo))
	/*if (kfifo_is_full(&data_fifo))
	{
		printk("fifo is full!\n");
		return -1;
	}*/
	ret_val = kfifo_from_user(&data_fifo, buf, size, &copied);
	if (ret_val != 0)
	{
		printk("put data to fifo error!\n");
	}
	else
	{
		if ((copied != 0) && (!kfifo_is_empty(&data_fifo)))
			queue_work(wq, &wk);
		printk("put %d byte to fifo\n", copied);
	}
	return copied;
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
	ret_val = kfifo_alloc(&data_fifo, fifo_size, GFP_KERNEL);
	if (ret_val != 0)
	{
		printk("alloc kfifo error!\n");
		goto fifo_err;
	}

	wq = create_workqueue("data_wq");
	INIT_WORK(&wk, data_wk_proc);
	
	ret_val = misc_register(&io_driver);
	if (ret_val != 0)
	{
		printk("register misc device err!\n");
		goto misc_err;	
	}
	
	return 0;
fifo_err:
	return -EFAULT;
misc_err:
	kfifo_free(&data_fifo);
	return -EFAULT;
}
static void io_driver_exit(void)
{
	destroy_workqueue(wq);
	kfifo_free(&data_fifo);
	misc_deregister(&io_driver);
}
module_init(io_driver_init);
module_exit(io_driver_exit);
MODULE_AUTHOR("wsb <andrinux@163.com>");
MODULE_LICENSE("GPL");