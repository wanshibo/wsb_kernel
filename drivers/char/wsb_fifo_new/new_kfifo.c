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

#define FIFO_DEBUG(fmt, arg...)  printk("wsb_fifo:"fmt,##arg) 
#define DRIVER_NAME "wsb_fifo"

static wait_queue_head_t write_wait;
static unsigned char wait_condition;

#define FIFO_SIZE 	16
static spinlock_t fifo_lock;
#if 0
#define DYNAMIC_FIFO
#endif

#ifdef DYNAMIC_FIFO
	static struct kfifo test;
#else
	static DECLARE_KFIFO(test, unsigned char, FIFO_SIZE);
#endif


static ssize_t io_driver_read(struct file *filp, char __user *buf, size_t size, loff_t *fops)
{
	int ret_val = 0;
	unsigned int copied = 0;
	wait_event_interruptible(write_wait, wait_condition!= 0);
	wait_condition = 0;
	ret_val = kfifo_to_user(&test, buf, size, &copied);
	if (ret_val != 0)
		return -1;
	return copied;
}

static ssize_t io_driver_write(struct file *filp, const char __user *buf, size_t size, loff_t *fpos)
{
	int ret_val = 0;
	unsigned int copied = 0;

	ret_val = kfifo_from_user(&test, buf, size, &copied);
	if (ret_val != 0)
		return -1;
	wait_condition = 1;
	wake_up_interruptible(&write_wait);
	return copied;
}

static void put_data_in_fifo(void)
{
	unsigned char data;
	unsigned char buf[10];
	unsigned char buf_count = 0;
	unsigned char idx;

	//put 5 data into fifo
	for (idx = 0; idx < 5; idx++)
		kfifo_put(&test, &idx);
	FIFO_DEBUG("put data in fifo!\n");
	FIFO_DEBUG("the length fo fifo is %d\n", kfifo_len(&test));

	//get most of (sizeof(buf)) data out from fifo
	buf_count = kfifo_out(&test, buf, sizeof(buf));
	FIFO_DEBUG("get %d data from fifo\n", buf_count);
	for (idx = 0; idx < buf_count; idx++)
		FIFO_DEBUG("buf[%d]=%d\n", idx, buf[idx]);
	
	//put 4 data into fifo
	for (idx = 0; idx < 7; idx++)
		buf[idx] = idx+1;
	buf_count = kfifo_in(&test, buf, idx);
	//buf_count = kfifo_in(&test, buf, sizeof(buf)); //!!!note:kfifo_in put sizeof(buf) data into fifo
		FIFO_DEBUG("put %d data into fifo\n", buf_count);
	
	//get the rest of data out
	while(kfifo_get(&test, &data))
	{
		FIFO_DEBUG("data = %d\n", data);
	}
}
/*
static void get_data_from_fifo(void)
{
	while(kfifo_out(fifo, buf, n))
}
*/
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
	init_waitqueue_head(&write_wait);
#ifdef DYNAMIC_FIFO
	ret_val = kfifo_alloc(&test, FIFO_SIZE, GFP_KERNEL);
	if (ret_val != 0)
	{
		FIFO_DEBUG("kfifo alloc fail!\n");	
		goto fifo_err;
	}
#else
	INIT_KFIFO(test);
#endif


	put_data_in_fifo();
	
	ret_val = misc_register(&io_driver);
	if (ret_val != 0)
	{
		FIFO_DEBUG("register misc device fail!\n");
		goto misc_err;	
	}
	
	return 0;
fifo_err:
	return -EFAULT;
misc_err:
#ifdef DYNAMIC_FIFO
	kfifo_free(&test);
#endif
	return -EFAULT;
}
static void io_driver_exit(void)
{
#ifdef DYNAMIC_FIFO
	kfifo_free(&test);
#endif
	misc_deregister(&io_driver);
}
module_init(io_driver_init);
module_exit(io_driver_exit);
MODULE_AUTHOR("wsb <andrinux@163.com>");
MODULE_LICENSE("GPL");