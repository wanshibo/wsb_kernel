/*
 * Copyright (c) 2012 sunplusapp
 * this program is free software; you can redistribute it and/or modify it
 *	
 *
 * Date and Edition:		2012-11-18  v1.0
 * Author:					Valor-Lion
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/slab.h>

static struct i2c_board_info si4702_info = {	
	I2C_BOARD_INFO("si4702", 0x10),
};

static struct i2c_client *si4702_client;

static int dev_test_dev_init(void)
{
	struct i2c_adapter *i2c_adap;

	i2c_adap = i2c_get_adapter(1);
	if(i2c_adap == NULL){
		printk("err: can not get i2c adapter 1...\n");
		return -ENODEV;
	}
		
	si4702_client = i2c_new_device(i2c_adap, &si4702_info);
	i2c_put_adapter(i2c_adap);
	
	return 0;
}

static void dev_test_dev_exit(void)
{
	i2c_unregister_device(si4702_client);
}

module_init(dev_test_dev_init);
module_exit(dev_test_dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sunplusedu");

