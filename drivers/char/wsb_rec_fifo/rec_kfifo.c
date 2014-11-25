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
#include <asm/uaccess.h>

#define FIFO_DEBUG(fmt, arg...)  printk("wsb_fifo:"fmt,##arg) 
#define DRIVER_NAME "wsb_rec_fifo"

static wait_queue_head_t write_wait;


#define FIFO_SIZE 	16
static spinlock_t fifo_lock;
#if 0
#define DYNAMIC_FIFO
#endif

#ifdef DYNAMIC_FIFO
	struct kfifo_rec_ptr_1 test;
#else
typedef STRUCT_KFIFO_REC_1(FIFO_SIZE) REC_FIFO;
	static REC_FIFO test;
#endif


static ssize_t io_driver_read(struct file *filp, char __user *buf, size_t size, loff_t *fops)
{
	char ret_val = 0;
	unsigned int copied = 0;
	//unsigned int actual_size = 0;

	wait_event_interruptible(write_wait, kfifo_is_empty(&test) != 1);
	//actual_size = kfifo_peek_len(&test);
	//FIFO_DEBUG("actual_size = %d\n", actual_size);
	//actual_size = min(actual_size, size);
	//FIFO_DEBUG("actual_size = %d\n", actual_size);
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
	wake_up_interruptible(&write_wait);
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


static void rec_test(void)
{
	int rec_size = 0;
	int e_size = 0;
	int in_size = 0;
	int fifo_aval = 0;
	unsigned char buf[] = {0x02, 0x04, 0x06, 0x08};
	unsigned char buf1[] = {0x02, 0x04, 0x06, 0x08};
	in_size = kfifo_in(&test, buf, sizeof(buf));
	FIFO_DEBUG("in_size = %d\n", in_size);
	//e_size   = kfifo_esize(&test);
	//FIFO_DEBUG("e_size = %d\n", e_size);
	//rec_size = kfifo_recsize(&test);
	//FIFO_DEBUG("rec_size = %d\n", rec_size);
	fifo_aval = kfifo_avail(&test);
	FIFO_DEBUG("fifo_aval = %d\n", fifo_aval);

	in_size = kfifo_in(&test, buf1, sizeof(buf));
	FIFO_DEBUG("in_size = %d\n", in_size);

	//e_size   = kfifo_esize(&test);
	//FIFO_DEBUG("e_size = %d\n", e_size);
	//rec_size = kfifo_recsize(&test);
	//FIFO_DEBUG("rec_size = %d\n", rec_size);
	fifo_aval = kfifo_avail(&test);
	FIFO_DEBUG("fifo_aval = %d\n", fifo_aval);
	
/*
	int size = 0;
	int num = 190;
	in_size = kfifo_in(&test, &num, sizeof(num));
	FIFO_DEBUG("in_size = %d\n", in_size);
	e_size   = kfifo_esize(&test);
	rec_size = kfifo_recsize(&test);
	FIFO_DEBUG("rec_size = %d\n", rec_size);
	fifo_aval = kfifo_avail(&test);
	FIFO_DEBUG("fifo_aval = %d\n", fifo_aval);
	size = kfifo_size(&test);
	FIFO_DEBUG("size = %d\n", size);

	in_size = kfifo_in(&test, &num, sizeof(num));
	FIFO_DEBUG("in_size = %d\n", in_size);
	e_size   = kfifo_esize(&test);
	rec_size = kfifo_recsize(&test);
	FIFO_DEBUG("rec_size = %d\n", rec_size);
	fifo_aval = kfifo_avail(&test);
	FIFO_DEBUG("fifo_aval = %d\n", fifo_aval);
	size = kfifo_size(&test);
	FIFO_DEBUG("size = %d\n", size);
*/
}

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
	rec_test();
	
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