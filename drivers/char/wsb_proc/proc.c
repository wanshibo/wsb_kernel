#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#define DRIVER_NAME "io_driver"
#define PROC_DIR	"io_driver"
#define PROC_FILE	"ioinfo"

typedef struct proc_info
{
	struct proc_dir_entry *proc_root;
	struct proc_dir_entry *proc_entry;
}PROC_SYS_T;

PROC_SYS_T *gp_proc;
ssize_t io_driver_read(struct file *filp, char __user *buf, size_t size, loff_t *fops)
{
	return 0;
}
ssize_t io_driver_write(struct file *filp, const char __user *buf, size_t size, loff_t *fpos)
{
	return 0;
}

static struct file_operations io_driver_fops = {
	.owner = THIS_MODULE,
	.read  = io_driver_read,
	.write = io_driver_write,
};

static ssize_t proc_read(struct file *filp, char __user *buf, size_t size, loff_t *fops)
{
	printk("This is sample proc read interface!\n");
	return 0;
}
static ssize_t proc_write(struct file *filp, const char __user *buf, size_t size, loff_t *fpos)
{
	printk("Write something through sample proc wirte interface!\n");
	return 0;
}

static struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.read  = proc_read,
	.write = proc_write,
};

static int io_driver_proc_create(PROC_SYS_T *proc)
{
	if (!strlen(PROC_DIR))
	{
		proc->proc_root = NULL;
	}
	else
	{
		proc->proc_root = proc_mkdir(PROC_DIR, NULL);
		if (!proc->proc_root)
			return -ENOMEM;
	}
	proc->proc_entry = proc_create(PROC_FILE, 0, proc->proc_root, &proc_fops);
	if (!proc->proc_entry)
		return -ENOMEM;
	
	return 0;	
}

static void io_driver_proc_delete(PROC_SYS_T *proc)
{
	if (proc->proc_entry)
		remove_proc_entry(PROC_FILE, proc->proc_root);
	if (proc->proc_root)
		remove_proc_entry(PROC_DIR, proc->proc_root);
}

static struct miscdevice io_driver = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DRIVER_NAME,
	.fops  = &io_driver_fops,
};


static __init int io_driver_init(void)
{
	int ret_val;
	PROC_SYS_T io_driver_proc;
	gp_proc = &io_driver_proc;
	io_driver_proc_create(&io_driver_proc);
	ret_val = misc_register(&io_driver);
	return 0;
}
static void io_driver_exit(void)
{
	io_driver_proc_delete(gp_proc);
	misc_deregister(&io_driver);
}
module_init(io_driver_init);
module_exit(io_driver_exit);
MODULE_AUTHOR("wsb <andrinux@163.com>");
MODULE_LICENSE("GPL");