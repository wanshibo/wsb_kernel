/**
 *brief  :This file is aih DVD player motor and some extern interrupt driver.
 *Data   :2014-10-30
 *Author :Wsb
 *Version:0.1
**/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <mach/regs-gpio.h>
#include <asm/io.h>
#include <mach/gpio.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <asm-generic/siginfo.h>
//#include <asm-generic/signal.h>
#include <linux/kthread.h>  
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <plat/gpio-cfg.h>
//#include <mach/irqs.h>
#include <linux/irq.h>
//#include <plat/irqs.h>
#include <asm/gpio.h>
#include <mach/irqs-exynos4.h>
#include "dvd_player.h"


#define DEBUG_LOG_ENABLE 
#ifdef  DEBUG_LOG_ENABLE
#define DVD_LOG(fmt, arg...) printk(KERN_ERR   "[dvd]"fmt, ##arg)
#else
#define DVD_LOG(fmt, arg...) printk(KERN_DEBUG "[dvd]"fmt, ##arg)
#endif

struct dvd_player aih_dvd;


DVD_GPIO DiscEnterExitButton = {
	.gpio_name = "DiscEnterExitButton",
	.gpio_num  = EXYNOS4_GPX0(2),
	.irq = IRQ_EINT(2),
};
DVD_GPIO DiscEnterSignal = {
	.gpio_name = "DiscEnterSignal",
	.gpio_num  = EXYNOS4_GPX2(6),
	.irq = IRQ_EINT(22),
};
DVD_GPIO DiscExitSignal = {
	.gpio_name = "DiscExitSignal",
	.gpio_num  = EXYNOS4_GPX3(2),
	.irq = IRQ_EINT(26),
};
DVD_GPIO DiscInsertSignal = {
	.gpio_name = "DiscInsertSignal",
	.gpio_num  = EXYNOS4_GPX3(3),
	.irq = IRQ_EINT(27),
};

DVD_GPIO MotorPWM1H = {
	.gpio_name = "MotorPWM1H",
	.gpio_num  = EXYNOS4212_GPV0(0),
};

DVD_GPIO MotorPWM1L = {
	.gpio_name = "MotorPWM1L",
	.gpio_num  = EXYNOS4212_GPV0(1),
};

DVD_GPIO MotorPWM2H = {
	.gpio_name = "MotorPWM2H",
	.gpio_num  = EXYNOS4212_GPV0(2),
};

DVD_GPIO MotorPWM2L = {
	.gpio_name = "MotorPWM2L",
	.gpio_num  = EXYNOS4212_GPV0(3),
};




static int dev_async(int fd, struct file *filp, int mode)
{
	struct dvd_player *dvd_dev = filp->private_data;
	return fasync_helper(fd, filp, mode, &dvd_dev->async_irq);
}


static int UpdateExternalSignalStatus(void)
{
	char DiscOpenSignalStatus = 0;
	char DiscCloseSignalStatus = 0;
	char DiscInsertSignalStatus = 0;
	int SignalStatus = 0;
	
	s3c_gpio_cfgpin(DiscEnterSignal.gpio_num, S3C_GPIO_SFN(S3C_GPIO_INPUT));
	DiscOpenSignalStatus = gpio_get_value(DiscEnterSignal.gpio_num);
	s3c_gpio_cfgpin(DiscEnterSignal.gpio_num, S3C_GPIO_SFN(0xf));


	s3c_gpio_cfgpin(DiscExitSignal.gpio_num, S3C_GPIO_SFN(S3C_GPIO_INPUT));
	DiscCloseSignalStatus = gpio_get_value(DiscExitSignal.gpio_num);
	s3c_gpio_cfgpin(DiscExitSignal.gpio_num, S3C_GPIO_SFN(0xf));


	s3c_gpio_cfgpin(DiscInsertSignal.gpio_num, S3C_GPIO_SFN(S3C_GPIO_INPUT));
	DiscInsertSignalStatus = gpio_get_value(DiscInsertSignal.gpio_num);
	s3c_gpio_cfgpin(DiscInsertSignal.gpio_num, S3C_GPIO_SFN(0xf));
	
	aih_dvd.DiscEnterSignalStatus= !!DiscOpenSignalStatus;
	aih_dvd.DiscExitSignalStatus= !!DiscCloseSignalStatus;
	aih_dvd.DiscInsertSignalStatus= !!DiscInsertSignalStatus;
	
	SignalStatus = ((!!DiscOpenSignalStatus << 0) | (!!DiscCloseSignalStatus << 1) | (!!DiscInsertSignalStatus << 2));
	DVD_LOG("DiscOpenSignalStatus = %d\n", !!DiscOpenSignalStatus);
	DVD_LOG("DiscCloseSignalStatus = %d\n", !!DiscCloseSignalStatus);
	DVD_LOG("DiscInsertSignalStatus = %d\n", !!DiscInsertSignalStatus);
	DVD_LOG("SignalStatus = %d\n", SignalStatus);

	return SignalStatus;
}





static ssize_t dev_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int copied_size;
	int group_val = 0;
	/*
	int idx, bit_val;
	for (idx = 0; idx < EXYNOS4212_GPIO_V0_NR; idx++)
	{
		bit_val = gpio_get_value(EXYNOS4212_GPV0(idx));
		group_val |= (!!(bit_val) << idx);
	}
	*/
	group_val = UpdateExternalSignalStatus();
	copied_size = copy_to_user(buf, &group_val, sizeof(group_val));
	return 0;
}

static ssize_t dev_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	//if (aih_dvd.async_irq)
		//kill_fasync(&aih_dvd.async_irq, SIGIO, POLL_IN);
	return 0;
}

static int dev_open(struct inode *inde, struct file *filp)
{
	filp->private_data = (void *)&aih_dvd;
	return 0;
}

static int dev_release(struct inode *inde, struct file *filp)
{
	dev_async(-1, filp, 0);
	return 0;
}

static void MotorTurnForward(void)
{
	DVD_LOG("motor turning forward....");
	//pwm1h_l
	PWM1H_L;
	//pwm1l_l
	PWM1L_L;
	//pwm2h_h
	PWM2H_H;
	//pwm2l_h
	PWM2L_H;
}

static void MotorTurnBack(void)
{
	DVD_LOG("motor turning back....");
	//pwm1h_h
	PWM1H_H;
	//pwm1l_h
	PWM1L_H;
	//pwm2h_l
	PWM2H_L;
	//pwm2l_l
	PWM2L_L;
}

static void MotorStop(void)
{
	DVD_LOG("motor stop...");
}
static void DvdExitDisc(void)
{
	DVD_LOG("DVD exit disc!\n");
	MotorTurnForward();
}

static void DvdEnterDisc(void)
{
	DVD_LOG("DVD enter disc!\n");	
	MotorTurnBack();
}
static void DvdMotorStop(void)
{
	DVD_LOG("DVD Motor is stop!\n");
	MotorStop();
}

static long dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned int command = cmd;
	switch(command)
	{
		case DVD_MOTOR_GO_FORWARD:
			MotorTurnForward();
			break;
		default: break;
	}
	return 0;
}

struct file_operations dvd_fops = {
	.owner = THIS_MODULE,
	.open  = dev_open,
	.release = dev_release,
	.read  = dev_read,
	.write = dev_write,
	.fasync = dev_async,
	.unlocked_ioctl = dev_ioctl,
};


//0:disc 1:no disc
irqreturn_t DiscEnterPosSignal_func(int irq, void *dev_id)
{
	//if (aih_dvd.async_irq)
	//	kill_fasync(&aih_dvd.async_irq, SIGIO, POLL_IN);
	UpdateExternalSignalStatus();
	if (aih_dvd.DiscEnterSignalStatus == 0)
	{
		if (aih_dvd.DvdRtStatus.DVD_Eject == DVD_ENTER_DISC)
		{
			//mdelay(10);
			//tell app dvd is already entered ?
			DVD_LOG("Disc is already entered!\n");
		}
	}
	return IRQ_HANDLED;
}

//if DVD disc is already out, then call this function
//1:disc is already out
irqreturn_t DiscExitPosSignal_func(int irq, void *dev_id)
{
	UpdateExternalSignalStatus();
	if (aih_dvd.DiscExitSignalStatus == 1) //1:disc is already out
	{
		if (DVD_EXIT_DISC == aih_dvd.DvdRtStatus.DVD_Eject)
		{
			DvdMotorStop();
			udelay(10);
		}
	}
	return IRQ_HANDLED;
}

//if there is a DVD ready to instert,then call this function.
//0:disc 1:no disc
irqreturn_t DiscInsertSignal_func(int irq, void *dev_id)
{
	UpdateExternalSignalStatus();
	if (aih_dvd.DiscInsertSignalStatus == 0)
	{
		DvdEnterDisc();
		aih_dvd.DvdRtStatus.DVD_Eject = DVD_ENTER_DISC;
	}
	else
	{
		DvdMotorStop();
		aih_dvd.DvdRtStatus.DVD_Eject = DVD_EXIT_DISC;
	}

	return IRQ_HANDLED;
}

//if the user wants to get DVD disc out will call this fuction.
irqreturn_t DVDButton_func(int irq, void *dev_id)
{
	DVD_LOG("DVD Enter/Exit!\n");
	DvdExitDisc();
	
	return IRQ_HANDLED;
}

static void ExternalInterruptInit(void)
{
	//DiscEnterSignal 
	gpio_request(DiscEnterSignal.gpio_num, DiscEnterSignal.gpio_name);
	s3c_gpio_setpull(DiscEnterSignal.gpio_num, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(DiscEnterSignal.gpio_num, S3C_GPIO_SFN(0xf));
	//DiscExitSignal
	gpio_request(DiscExitSignal.gpio_num, DiscExitSignal.gpio_name);	
	s3c_gpio_setpull(DiscExitSignal.gpio_num, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(DiscExitSignal.gpio_num, S3C_GPIO_SFN(0xf));
	//DiscInsertSignal
	gpio_request(DiscInsertSignal.gpio_num, DiscInsertSignal.gpio_name);	
	s3c_gpio_setpull(DiscInsertSignal.gpio_num, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(DiscInsertSignal.gpio_num, S3C_GPIO_SFN(0xf));

	//DVD Enter/Exit Button Signal
	gpio_request(DiscEnterExitButton.gpio_num, DiscEnterExitButton.gpio_name);	
	s3c_gpio_setpull(DiscEnterExitButton.gpio_num, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(DiscEnterExitButton.gpio_num, S3C_GPIO_SFN(0xf));
}

static int ExternalInterruptSetup(void)
{
	int retval = 0;

	//disc in position status 
	retval = request_irq(DiscEnterSignal.irq, DiscEnterPosSignal_func,  IRQ_TYPE_EDGE_FALLING, DiscEnterSignal.gpio_name, (void *)DiscEnterSignal.irq);//IRQF_DISABLED
	if (retval < 0)
	{
		DVD_LOG("request irq 22 error!\n");
		goto err_enter;
	}
	//disc out position status          0:disc is not out      1:disc is out
	retval = request_irq(DiscExitSignal.irq, DiscExitPosSignal_func,  IRQ_TYPE_EDGE_RISING, DiscExitSignal.gpio_name, (void *)DiscExitSignal.irq);//IRQF_DISABLED
	if (retval < 0)
	{
		DVD_LOG("request irq 26 error!\n");
		goto err_exit;
	}
	//is there a disc in or not        0:no disc    1:disc
	retval = request_irq(DiscInsertSignal.irq, DiscInsertSignal_func,  IRQ_TYPE_EDGE_BOTH, DiscInsertSignal.gpio_name, (void *)DiscInsertSignal.irq);//IRQF_DISABLED
	if (retval < 0)
	{
		DVD_LOG("request irq 27 error!\n");
		goto err_insert;
	}
	retval = request_irq(DiscEnterExitButton.irq, DVDButton_func,  IRQ_TYPE_EDGE_FALLING, DiscEnterExitButton.gpio_name, (void *)DiscEnterExitButton.irq);//IRQF_DISABLED
	if (retval < 0)
	{
		DVD_LOG("request irq 2 error!\n");
		goto err_button;
	}
	return 0;

err_enter:
	return -1;
err_exit:
	free_irq(DiscEnterSignal.irq, (void *)DiscEnterSignal.irq);
	return -1;
err_insert:
	free_irq(DiscEnterSignal.irq, (void *)DiscEnterSignal.irq);
	free_irq(DiscExitSignal.irq,  (void *)DiscExitSignal.irq);
	return -1;
err_button:
	free_irq(DiscEnterSignal.irq, (void *)DiscEnterSignal.irq);
	free_irq(DiscExitSignal.irq,  (void *)DiscExitSignal.irq);
	free_irq(DiscInsertSignal.irq,(void *)DiscInsertSignal.irq);
	return -1;
}


static void DvdMotorGpioInit(void)
{
	//pwm1h
	gpio_request(MotorPWM1H.gpio_num, MotorPWM1H.gpio_name);
	s3c_gpio_cfgpin(MotorPWM1H.gpio_num, S3C_GPIO_OUTPUT);
	gpio_direction_output(MotorPWM1H.gpio_num, 0);
	
	//pwm1l
	gpio_request(MotorPWM1L.gpio_num, MotorPWM1L.gpio_name);
	s3c_gpio_cfgpin(MotorPWM1L.gpio_num, S3C_GPIO_OUTPUT);
	gpio_direction_output(MotorPWM1L.gpio_num, 0);
	
	//pwm2h
	gpio_request(MotorPWM2H.gpio_num, MotorPWM2H.gpio_name);
	s3c_gpio_cfgpin(MotorPWM2H.gpio_num, S3C_GPIO_OUTPUT);
	gpio_direction_output(MotorPWM2H.gpio_num, 0);
	//pwm2l
	gpio_request(MotorPWM2L.gpio_num, MotorPWM2L.gpio_name);
	s3c_gpio_cfgpin(MotorPWM2L.gpio_num, S3C_GPIO_OUTPUT);
	gpio_direction_output(MotorPWM2L.gpio_num, 0);
}

static __init int AIH_DVDDriverInit(void)
{
	int retval;
	DVD_LOG("%s", __FUNCTION__);
	
	aih_dvd.dev_fops = &dvd_fops;	
	retval = alloc_chrdev_region(&aih_dvd.dev_num, 0, 1, DVD_NAME);
	if (retval)
	{
		goto err_alloc;
	}

	cdev_init(&aih_dvd.dev_cdev, &dvd_fops);
	retval = cdev_add(&aih_dvd.dev_cdev, aih_dvd.dev_num, 1);
	if (retval)
	{
		goto err_cdev;
	}

	aih_dvd.dev_class = class_create(THIS_MODULE, DVD_NAME);
	if (IS_ERR(aih_dvd.dev_class))
	{
		goto err_class;
	}

	aih_dvd.dev_device = device_create(aih_dvd.dev_class, NULL, aih_dvd.dev_num, NULL, DVD_NAME);
	if (IS_ERR(aih_dvd.dev_device))
	{
		goto err_device;
	}	

	//interrupt gpio init
	ExternalInterruptInit();
	//setup interrupt
	retval = ExternalInterruptSetup();
	if (-1 == retval)
	{
		DVD_LOG("dvd setup irq error!\n");
		goto err_irq;
	}
	DVD_LOG("DVD external irq request success!\n");
	//motor gpio init
	DvdMotorGpioInit();

	return 0;
err_irq:
	device_destroy(aih_dvd.dev_class, aih_dvd.dev_num);

err_device:
	class_destroy(aih_dvd.dev_class);

err_class:
	cdev_del(&aih_dvd.dev_cdev);

err_cdev:
	unregister_chrdev_region(aih_dvd.dev_num, 1);

err_alloc:
	return retval;
}



static __exit void AIH_DVDDriverExit(void)
{
	DVD_LOG("%s", __FUNCTION__);
	free_irq(DiscEnterSignal.irq, (void *)DiscEnterSignal.irq);
	free_irq(DiscExitSignal.irq,  (void *)DiscExitSignal.irq);
	free_irq(DiscInsertSignal.irq,(void *)DiscInsertSignal.irq);
	free_irq(DiscEnterExitButton.irq,(void *)DiscEnterExitButton.irq);
	device_destroy(aih_dvd.dev_class, aih_dvd.dev_num);
	class_destroy(aih_dvd.dev_class);
	cdev_del(&aih_dvd.dev_cdev);
	unregister_chrdev_region(aih_dvd.dev_num, 1);
}



module_init(AIH_DVDDriverInit);
module_exit(AIH_DVDDriverExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wsb <wanshibo1992@163.com>");










































