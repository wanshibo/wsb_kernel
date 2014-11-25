
/**
  *Copyright (c) 2012 sunplusapp
  *this program is free software; you can redistribute it and/or modify it
  *
  *
  *Description:		
  *
  *Date and Edition:		<2012-11-02  v1.1>	<2013-02-20  v1.2>
  *Author:				Valor Lion
  */


#include <linux/module.h>   	
#include <linux/kernel.h>		

#include <linux/init.h>			
#include <linux/fs.h>			
#include <asm/uaccess.h>		
#include <linux/device.h>  		
#include <linux/errno.h>		
#include <linux/slab.h>
#include <linux/i2c.h>

#include <mach/regs-gpio.h>
#include <linux/gpio.h>  			
#include <linux/miscdevice.h>

#include <asm/delay.h>     		
#include <linux/delay.h>		
#include <plat/gpio-cfg.h>
#include "si4702.h"


#define SI4702RST		EXYNOS4_GPA0(2)//S5PV210_GPB(6)
#define SI4702_REGISTER_NUM		16
#define SI4702_REGISTER_SIZE		2

#define __debug(fmt,arg...)			printk(KERN_WARNING fmt, ##arg)
#define 	DRIVER_NAME				"fm_si4702"

struct si4702_data{
	struct i2c_client *client;
	unsigned char chip_addr;

	struct si4702_channel_list channel_list;
	union si4702_regval regval;
	
	int flag;
};

static struct si4702_data *si4702_ptr;



void debug_printreg(union si4702_regval *regval)
{
	int i;

	__debug("============\n");
	for(i=0; i<32; i++)
		__debug("the value is %x\n",regval->buf[i]);
	__debug("\n");
}


void si4702_hardware_reset(void)
{/*
	gpio_set_value(SI4702RST, 0);
	udelay(20);
	gpio_set_value(SI4702RST, 1);
	udelay(20);*/
	gpio_request(EXYNOS4212_GPV0(0),"rest");
	gpio_direction_output(EXYNOS4212_GPV0(2), 0);
	udelay(20);
	gpio_direction_output(EXYNOS4212_GPV0(2), 1);
	udelay(20);
}


#define CHAN_TO_FREQ		1
#define FREQ_TO_CHAN		2
static int chan_freq_convert(union si4702_regval *regval, unsigned int freq, 
			unsigned int chan, int direc)
{
	unsigned int space;
	unsigned int base;
	unsigned int tmp = 0;
	switch(regval->reg.space){
		case SI4702_SPACE_50KHZ:
			space = 50;
			break;
		case SI4702_SPACE_100KHZ:
			space = 100;
			break;
		case SI4702_SPACE_200KHZ:
			space = 200;
			break;
		default:
			space = 0;
			break;
	}

	switch(regval->reg.band){
		case SI4702_BAND_87to108:
			base = 875;
			tmp = 1080;
			freq = min(freq, tmp);
			tmp = 875;
			freq = max(freq, tmp);
			break;
		case SI4702_BAND_76to108:
			tmp = 1080;
			freq = min(freq, tmp);
			tmp = 760;
			freq = max(freq, tmp);
			base = 760;
			break;
		case SI4702_BAND_76to90:
			base = 760;
			tmp  = 900;
			freq = min(freq, tmp);
			tmp = 760;
			freq = max(freq, tmp);
			break;
		default:
			base = 0;
			break;
	}

	if(!base || !space)
		return -1;

	if(direc == CHAN_TO_FREQ)
		return (space/100 * chan + base);
	else
		return ((freq - base)*100/space);
}

static int channel_to_freq(union si4702_regval *regval, unsigned int chan)
{
	return chan_freq_convert(regval, 0, chan, CHAN_TO_FREQ);
}

static int freq_to_channel(union si4702_regval *regval, unsigned int freq)
{
	return chan_freq_convert(regval, freq, 0, FREQ_TO_CHAN);
}


/* channel list operations
  * @ the list initial, add one channel to list, delete one channel from list, get the
  * number of active channel, seek state and so on.
  *
  */
static void si4702_channel_list_init(struct si4702_channel_list *ch_list)
{
	memset(ch_list->list, sizeof(ch_list->list), 0);
	
	ch_list->list_size = CHANNEL_LIST_SIZE;
	ch_list->index = 0;
	ch_list->index_cur = 0;
	ch_list->state = SI4702_AUTOSEEK_UNFINISH;
}

#define si4702_channel_list_clear(_list)		si4702_channel_list_init(_list)

static int si4702_channel_list_add(struct si4702_channel_list *ch_list, unsigned int freq)
{
	int i;
	
	if(ch_list->index == ch_list->list_size){
		__debug("[SI4702 WARNING]: the channel list is full!!\n");
		goto finished;
	}

	for(i=0; i<ch_list->index; i++)
		if(ch_list->list[i] == freq)
			return 0;

	ch_list->list[ch_list->index] = freq;
	ch_list->index ++;
	ch_list->index_cur = 0;
	
finished:
	return ch_list->index;
}

static int si4702_channel_list_del(struct si4702_channel_list *ch_list, unsigned int freq)
{
	int i;
	
	for(i=0; i<ch_list->index; i++)
		if(ch_list->list[i] == freq)
			break;
	for(;i<ch_list->index-1; i++){
		ch_list->list[i] = ch_list->list[i+1];
	}
	ch_list->index--;
	ch_list->index_cur = 0;

	return ch_list->index;
}

static int si4702_channel_list_active(struct si4702_channel_list *ch_list)
{
	return ch_list->index;
}

static int si4702_channel_list_size(struct si4702_channel_list *ch_list)
{
	return ch_list->list_size;
}

/* i2c data read and write using controller 
  * 
  * si4702芯片支持两线式和三线式两种连接方式，我们采用的
  * 是两线式(标准i2c接口，可使用linux中i2c总线进行通信)，该芯
  * 有读写两种时序，可参考芯片手册
  *
  * si4702_read_regs 与si4702_write_regs完成了芯片中寄存器的读写操作，
  * 该芯片内部会有一个地址累加器，用户在连续读写寄存器
  * 数据时，寄存器地址会自动加1；当读寄存器时起始地址为
  * 0x0a，当写时起始地址为0x02，且当地址加到0xf时会自动回零
  *
  */
#define	SI4702_I2C_READ	0
#define 	SI4702_I2C_WRITE	1
static int __i2c_read_write(struct i2c_client *client, unsigned char *buf, int len, int dir)
{
	struct i2c_msg msg;
	struct si4702_data *si4702 = i2c_get_clientdata(client);
	int ret = -1;

	msg.addr = si4702->chip_addr;
	msg.flags = (dir==SI4702_I2C_READ) ? I2C_M_RD : !I2C_M_RD;
	msg.len = len;
	msg.buf = buf;
	printk("si4702++++addr = %d\n", client->addr);
	//printk("si4702++++adap = %d\n", client->adapter.);
	printk("si4702++++name = %s\n", client->name);
	ret = i2c_transfer(client->adapter, &msg, 1);
	printk("ret = %d\n", ret);
	if(ret < 0)
		return -1;
	return 0;
}

#define si4702_i2c_write(client, buf, len)	\
		__i2c_read_write(client, buf, len, SI4702_I2C_WRITE)

#define si4702_i2c_read(client, buf, len)	\
		__i2c_read_write(client, buf, len, SI4702_I2C_READ)


int si4702_read_regs(struct i2c_client *client, union si4702_regval *regval)
{
	unsigned char tmp[SI4702_REGISTER_NUM*SI4702_REGISTER_SIZE];
	int reg_start = 0x0a;		//the start address of register when reading
	int i;
	//while(1)
	//{
	if(si4702_i2c_read(client, tmp, sizeof(tmp)))
		return -1;
		//si4702_i2c_read(client, tmp, sizeof(tmp));
		//for (i=0; i < SI4702_REGISTER_NUM*SI4702_REGISTER_SIZE; i++)
			//printk("msg = %d\n", tmp[i]);
	//}
	for(i=0; i<SI4702_REGISTER_NUM; i++){
		regval->buf[reg_start*2+1] = tmp[i*2];
		regval->buf[reg_start*2] = tmp[i*2+1];
		reg_start ++;
		if(reg_start>0x0f)
			reg_start = 0;
	}
	return 0;
}

int si4702_write_regs(struct i2c_client *client, union si4702_regval *regval, unsigned int wlen)
{
	unsigned char tmp[SI4702_REGISTER_NUM*SI4702_REGISTER_SIZE];
	int reg_start = 0x02;			//the start address of register when wirting
	int i;

	for(i=0; i<SI4702_REGISTER_NUM; i++) {
		tmp[i*2] = regval->buf[reg_start*2+1];
		tmp[i*2+1] = regval->buf[reg_start*2];
		reg_start ++;
		if(reg_start>0x0f)
			reg_start = 0;
	}

	wlen = min(sizeof(tmp), wlen);
	if(si4702_i2c_write(client, tmp, wlen))
		return -1;
	return 0;
}


static int set_all_registers(struct si4702_data *si4702)
{
	return si4702_write_regs(si4702->client, &si4702->regval, 
				SI4702_REGISTER_NUM*SI4702_REGISTER_SIZE);
}

static int get_all_registers(struct si4702_data *si4702)
{
	return si4702_read_regs(si4702->client, &si4702->regval);
}


/* si4702 chip configuration
  *
  * 该部分完成si4702芯片的配置，主要配置有:电源的开关控制
  * 静音设置，音量调节，波段选择，手动/自动搜台等
  *
  * 注意该部分函数主要通过si4702_regval结构体完成芯片参数设
  * 置，然后将参数通过上面的寄存器读写函数进行写入或
  * 读出
  */

#define BEFORE_POWERUP_SI4702		0
#define AFTER_POWERUP_SI4702		1
#define BEFORE_POWERUP_SI4703		0x08
#define AFTER_POWERUP_SI4703		0x09

#define FIRMWARE_BEFORE_POWERUP		0
#define FIRMWARE_AFTER_POWERUP		0x13

static int fm_power_up(struct si4702_data *si4702)
{
	union si4702_regval *regval = &si4702->regval;

	/*reset the fm si4702 chip using RST pin*/
	si4702_hardware_reset();

	/*read registers first, when open the device*/
	si4702_read_regs(si4702->client, regval);
	
	regval->reg.xoscen = 1;		//enable the xoscen
	if(si4702_write_regs(si4702->client, regval, 14))
		return -1;
	msleep(300);
	
	regval->reg.enable = 1;		//enable: 1
	regval->reg.disable = 0;		//disable: 0
	if(si4702_write_regs(si4702->client, regval, 2))
		return -1;
	msleep(150);
	
	return 0;
}

static int fm_power_down(struct si4702_data *si4702)
{
	union si4702_regval *regval = &si4702->regval;
	/*disable rds , we needn't here*/

	/*set fm power down*/
	regval->reg.enable = 1;
	regval->reg.disable = 1;
	if(si4702_write_regs(si4702->client, regval, 2))
		return -1;
	else
		return 0;
}


/* open operations
  * @open the si4702 device, power up, set the register initial values  and
  * send the registers to si4702
  *
  * notice that we should get the default registers values before setting them, the 
  * values setting is roled in the default values
  */
static int si4702_device_start(struct si4702_data *si4702)
{
	union si4702_regval *regval = &si4702_ptr->regval;
	int ret = -ENOMEM;

	if(fm_power_up(si4702_ptr))
		return -ENODEV;

	if(get_all_registers(si4702_ptr))
		goto err;

	/*check the device powerup or not*/
	if(regval->reg.dev != AFTER_POWERUP_SI4702 ||
		regval->reg.firmware != FIRMWARE_AFTER_POWERUP)
		goto err;

	/*set the si4702 attributes*/
	regval->reg.seekth = 20;			//搜台精度，自动搜台时大于该值为搜索成功电台
	regval->reg.skmode = SI4702_SKMODE_WRAP;		//搜台模式
	regval->reg.band = SI4702_BAND_87to108;		//波段设置
	regval->reg.space = SI4702_SPACE_200KHZ;		//搜台间隔
	regval->reg.dmute = SI4702_MUTE_DISABLE;		//静音失效
	regval->reg.dsmute = SI4702_SMUTE_ENABLE;		//软件静音功能使能
	regval->reg.volume = SI4702_VOLUME_MAX;		//音量设置为最大值
	/*argument setting of smute*/
	regval->reg.smuter = SI4702_SMUTER_SLOW;
	regval->reg.smutea = SI4702_SMUTEA_12dB;
	regval->reg.volext = SI4702_VOLEXT_DISABLE;

	regval->reg.sksnr = 3;			//0:disable, range:1~15
	regval->reg.skcnt = 8;			//0:disable, range:1~15
	//regval->reg.ahizen = SI4702_AHIZEN_ENABLE;
	regval->reg.de = SI4702_DE_75US;
	
	if(set_all_registers(si4702_ptr))
		goto err;

	debug_printreg(regval);
	
	return 0;

err:
	fm_power_down(si4702_ptr);
	return ret;
}

static int si4702_open(struct inode *inode, struct file *filp)
{
	int ret;

	ret = si4702_device_start(si4702_ptr);
	if(ret){
		__debug("[SI4702 ERR]: si4702_device open failed!\n");
		return ret;
	}
	
	return 0;
	
}


/* close operations
  * @stop the si4702 device, power down
  */
static int si4702_device_stop(struct si4702_data *si4702)
{
	if(fm_power_down(si4702_ptr))
		return -1;
	return 0;
}

static int si4702_release(struct inode *inode, struct file *filp)
{
	si4702_device_stop(si4702_ptr);
	
	return 0;
}


/* write operations
  * @we can play a channel through write operation, if we have a channel to play,
  * first we send the freq of channel to device driver from user, then the si4702_write
  * function is called, next call the si4702_channel_play function, last the freq is send
  * to chip and play the channel you want
  */
static int __si4702_check_stc(struct si4702_data *si4702, int level, int time_limit)
{
	
	do{
		msleep(1);
		if(get_all_registers(si4702))
			return -1;
	}while(get_stc_from_status(&si4702->regval)!=level && time_limit--);

	if(get_stc_from_status(&si4702->regval) == level)
		return 0;
	else
		return -1;
}

static int si4702_channel_play(struct si4702_data *si4702, unsigned int freq)
{
	union si4702_regval *regval = &si4702->regval;
	int chan_val = freq_to_channel(&si4702->regval, freq);
	int rssi;


	/*send command to play a channel, start the tune, and check the stc.
	  *@ if the stc is hight, the tune is succeed
	  */
	regval->reg.chan = chan_val;
	regval->reg.tune = 1;
	if(si4702_write_regs(si4702->client, regval, 4))
		return -1;
	if(__si4702_check_stc(si4702,SI4702_STC_HIGH, 10))
		return -1;

	/*stop the tune, and check that the stc
	  *@ we write zero to tune bit, and the stc bit will be cleared
	  */
	regval->reg.tune = 0;
	if(si4702_write_regs(si4702->client, regval, 4))
		return -1;
	if(__si4702_check_stc(si4702,SI4702_STC_LOW, 1))
		return -1;

	rssi = get_rssi_from_status(&si4702->regval);
	__debug("[SI4702] playing:: freq is %d, rssi is %d...\n",
		channel_to_freq(regval, get_readchan_from_status(regval)), rssi);
	
	/*return the rssi of the channel*/
	return rssi;
}

static ssize_t si4702_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned int frq = 0;
	unsigned int len;
	
	len = min(sizeof(frq), count);
	if(copy_from_user(&frq, buf, len))
		return -ENODEV;

	if(si4702_channel_play(si4702_ptr, frq) < 0){
		__debug("[SI4702 ERR]: play channel failed!\n");
		return 0;
	}
	
	return len;
}


/* read operations
  * @ users can get the channel list struct by read system call, the struct is defined in
  * file "si4702.h" 
  */
static ssize_t si4702_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{	
	struct si4702_channel_list *chanlist = &si4702_ptr->channel_list;
	int len = 0;

	if(chanlist->state == SI4702_AUTOSEEK_UNFINISH || !chanlist->index)
		return 0;
	
	len = min(count, sizeof(struct si4702_channel_list));
	if(copy_to_user(buf, chanlist, len))
		return -ENODEV;

	return len;
}


/* ioctl operation
  * @the operation is very important, it can finish the configuration of si4702, such as
  * mute_setting, volume_setting, band_setting, seek_channel, auto_seek, play_channel...
  */
static int fm_mute_setting(struct si4702_data *si4702, unsigned int mute)
{
	union si4702_regval *regval = &si4702->regval;

	regval->reg.dmute = !!mute;
	if(si4702_write_regs(si4702->client, regval, 2))
		return -1;
	return 0;
}

static int fm_skmode_setting(struct si4702_data *si4702, unsigned int skmode)
{
	union si4702_regval *regval = &si4702->regval;

	regval->reg.skmode= !!skmode;
	if(si4702_write_regs(si4702->client, regval, 1))
		return -1;
	return 0;
}

static int fm_seekth_setting(struct si4702_data *si4702, unsigned int seekth)
{
	union si4702_regval *regval = &si4702->regval;
	unsigned int tmp = SI4702_SEEKTH_MAX;
	regval->reg.seekth = min(seekth, tmp);
	if(si4702_write_regs(si4702->client, regval, 8))
		return -1;
	return 0;
}

static int fm_band_setting(struct si4702_data *si4702, unsigned char band)
{
	union si4702_regval *regval = &si4702->regval;

	if(band > SI4702_BAND_MAX)
		return -1;

	regval->reg.band = band;
	if(si4702_write_regs(si4702->client, regval, 8))
		return -1;
	return 0;
}

static int fm_space_setting(struct si4702_data *si4702, unsigned char space)
{
	union si4702_regval *regval = &si4702->regval;

	if(space > SI4702_SPACE_MAX)
		return -1;

	regval->reg.space = space;
	if(si4702_write_regs(si4702->client, regval, 8))
		return -1;
	return 0;
}

static int fm_volume_setting(struct si4702_data *si4702, unsigned int volume)
{
	union si4702_regval *regval = &si4702->regval;

	unsigned int tmp = SI4702_VOLUME_MAX;
	regval->reg.volume = min(volume, tmp);
	if(si4702_write_regs(si4702->client, regval, 8))
		return -1;
	__debug("[SI4702] volume:: %d\n",volume);
	return 0;
}

/*play the channel saved in channel list
  *@index rang from 0 to active channel number
  */
static int __si4702_channel_list_play(struct si4702_data *si4702, unsigned int index)
{
	struct si4702_channel_list *chanlist = &si4702->channel_list;

	index = min(index, chanlist->index);
	return si4702_channel_play(si4702, chanlist->list[index]);
}

static int si4702_channel_list_play_current(struct si4702_data *si4702)
{
	struct si4702_channel_list *chanlist = &si4702->channel_list;

	if(chanlist->index_cur == chanlist->index)
		return 0;
	
	return __si4702_channel_list_play(si4702, chanlist->index_cur);
}

static int si4702_channel_list_play_next(struct si4702_data *si4702)
{
	struct si4702_channel_list *chanlist = &si4702->channel_list;

	if(chanlist->index_cur+1 == chanlist->index)
		return 0;
	
	chanlist->index_cur ++;
	return __si4702_channel_list_play(si4702, chanlist->index_cur);
}

static int si4702_channel_list_play_prev(struct si4702_data *si4702)
{
	struct si4702_channel_list *chanlist = &si4702->channel_list;

	if(chanlist->index_cur == 0)
		return 0;
	
	chanlist->index_cur --;
	return __si4702_channel_list_play(si4702, chanlist->index_cur);
}

/* seek one channel of si4702, if we seek one channel successfully, the function return the frequency,
  * otherwise return one err number
  * @dirc: it's the direction of seeking, we can seek forward and backward
  */
static int si4702_channel_seek(struct si4702_data *si4702, int dirc)
{
	union si4702_regval *regval = &si4702->regval;
	int seek_fail, rfc_rail;
	int freq;

	/*start to seek, and stop when seeked one channel*/
	regval->reg.seekup = dirc;
	regval->reg.seek = SI4702_SEEK_ENABLE;
	if(si4702_write_regs(si4702->client, regval, 1))
		return -1;
	if(__si4702_check_stc(si4702,SI4702_STC_HIGH, 1000*10))
		return -1;
	
	seek_fail = regval->reg.sfbl;		//get SF/BL bit value
	rfc_rail = regval->reg.afcrl;			//get AFCRL bit value

	/*stop the seek operation, the sate bit is cleared*/
	regval->reg.seek = SI4702_SEEK_DISABLE;
	if(si4702_write_regs(si4702->client, regval, 1))
		return -1;
	if(__si4702_check_stc(si4702,SI4702_STC_LOW, 5))
		return -1;

	/*if seek failed*/
	if(seek_fail)
		return 0;
	/*convert the seeked channel value to frequency*/
	freq = channel_to_freq(regval, get_readchan_from_status(regval));

	__debug("[SI4702] seeking::freq is %d, rssi is %d...\n",freq, get_rssi_from_status(regval));

	return freq;
}


/* rewind the si4702 to frequency->875
  * @ we should rewind the device before auto to seek channels
  */
static inline int __si4702_channel_rewind(struct si4702_data *si4702)
{
	int base;
	
	switch(si4702->regval.reg.band){
		case SI4702_BAND_87to108:
			base = 875;
			break;
		case SI4702_BAND_76to108:
			base = 760;
			break;
		case SI4702_BAND_76to90:
			base = 760;
			break;
		default:
			base = 0;
			break;
	}

	return si4702_channel_play(si4702, base);
}

static int si4702_channel_auto_seek(struct si4702_data *si4702)
{
	struct si4702_channel_list *chanlist = &si4702->channel_list;
	int cur_freq=0;
	int cycle = chanlist->list_size;
	int retval = 0;

	/*clear the current channel list value*/
	si4702_channel_list_clear(chanlist);

	if(__si4702_channel_rewind(si4702) < 0)
		return -1;
	
	while(cycle--){
		/*do channel seek*/
		cur_freq = si4702_channel_seek(si4702, SI4702_SEEK_UP);
		if(cur_freq <= 0)
			goto seek_err;
		
		/*if we finish a seek cycle, break out*/
		retval = si4702_channel_list_add(chanlist, cur_freq);
		if(retval == 0)
			break;
	}
	chanlist->state = SI4702_AUTOSEEK_FINISHED;
	
	if(si4702_channel_list_play_current(si4702) < 0)
		__debug("[SI4702 WARNING]: play the first channel failed!!\n");

	return si4702_channel_list_active(chanlist);
seek_err:
	si4702_channel_list_clear(chanlist);
	return -1;
}

/* get the information of the current playing channel
  * @we get the info by reading all register values
  */
static int si4702_get_channel_info(struct si4702_data *si4702, struct si4702_channel_info *info)
{
	union si4702_regval *regval = &si4702->regval;

	if(get_all_registers(si4702))
		return -ENOMEM;

	info->chip_version = regval->reg.rev;
	info->firm_version = regval->reg.firmware;
	info->manu_id = regval->reg.mfgid;
	info->freq = regval->reg.chan;
	info->band = regval->reg.band;
	info->volume = regval->reg.volume;
	info->mute = regval->reg.dmute;
	info->rssi = regval->reg.rssi;
	info->stereo = regval->reg.st;
	
	return 0;
}

long si4702_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
//long si4702_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct si4702_channel_info chan_info;
	int ret = -1;
	unsigned char *ptr = (void *)arg;
	
	
	switch(cmd){
		case SI4702_SET_MUTE:
			if(fm_mute_setting(si4702_ptr, arg))
				goto err;
			break;
			
		case SI4702_SET_VOLUME:
			if(fm_volume_setting(si4702_ptr, arg))
				goto err;
			break;
			
		case SI4702_SET_AREA:
			if(fm_band_setting(si4702_ptr, arg))
				goto err;
			break;
			
		case SI4702_SET_SPACE:
			if(fm_space_setting(si4702_ptr, arg))
				goto err;
			break;

		case SI4702_SET_SKMODE:
			if(fm_skmode_setting(si4702_ptr, arg))
				goto err;
			break;
			
		case SI4702_SET_SEEKTH:
			if(fm_seekth_setting(si4702_ptr, arg))
				goto err;
			break;
		
		case SI4702_SEEK_NEXT:
			ret = si4702_channel_seek(si4702_ptr, SI4702_SEEK_UP);
			if(ret <= 0)
				goto err;
			else
				if(copy_to_user(ptr, &ret, sizeof(unsigned long)))
					goto err;
			break;
			
		case SI4702_SEEK_PREV:
			ret = si4702_channel_seek(si4702_ptr, SI4702_SEEK_DOWN);
			if(ret <= 0)
				goto err;
			else
				if(copy_to_user(ptr, &ret, sizeof(unsigned long)))
					goto err;
			break;
		
		case SI4702_AUTO_SEEK:
			ret = si4702_channel_auto_seek(si4702_ptr);
			if(ret < 0)
				goto err;
			else
				if(copy_to_user(ptr, &ret, sizeof(unsigned long)))
					goto err;
			break;

		case SI4702_PLAY_NEXT:
			ret = si4702_channel_list_play_next(si4702_ptr);
			if(ret <= 0)
				goto err;
			else
				if(copy_to_user(ptr, &ret, sizeof(unsigned long)))
					goto err;
			break;
			
		case SI4702_PLAY_PREV:
			ret = si4702_channel_list_play_prev(si4702_ptr);
			if(ret <= 0)
				goto err;
			else
				if(copy_to_user(ptr, &ret, sizeof(unsigned long)))
					goto err;
			break;
			
		case SI4702_GET_CHANINFO:
			ret = si4702_get_channel_info(si4702_ptr, &chan_info);
			if(ret)
				goto err;
			else
				if(copy_to_user(ptr, &chan_info, sizeof(chan_info)))
					goto err;
			break;
			
		default:
			break;
	}
	return 0;

err:
	return -ENODEV;
}

static struct file_operations si4702_fops = {
	.owner = THIS_MODULE,
	.write = si4702_write,
	.read =	si4702_read,
	.unlocked_ioctl = si4702_ioctl,
	.open =	si4702_open,
	.release =	si4702_release,
};

static struct miscdevice si4702_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DRIVER_NAME,
	.fops = &si4702_fops,
};

static int si4702_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct si4702_data *si4702;
	
	int retval;	

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c_check_functionnality error\n");
		__debug("[SI4702 ERR]: i2c check failed!\n");
		return -EIO;
	}


	/*malloc the si4702_data and clear the space*/
	si4702 = kzalloc(sizeof(struct si4702_data), GFP_KERNEL);
	if(!si4702)
		return -ENOMEM;

	si4702->client = client;
	si4702->chip_addr = client->addr;

	i2c_set_clientdata(client, si4702);

	si4702_channel_list_init(&si4702->channel_list);
	
	si4702_ptr = si4702;
	
	/*device intial do the reset through RST pin*/
	s3c_gpio_cfgpin(SI4702RST,S3C_GPIO_OUTPUT);
	s5p_gpio_set_drvstr(SI4702RST, 2);

	/*register the si4702 device*/
	retval = misc_register(&si4702_misc);
	if(retval < 0)
		goto err;
	__debug("[info]: register si4702 succeed!\n");
	return 0;

err:
	kfree(si4702);
	return retval;
}

static int si4702_remove(struct i2c_client *client)
{
	struct si4702_data *si4702 = i2c_get_clientdata(client);

	kfree(si4702);
	misc_deregister(&si4702_misc);

	return 0;
}

static const struct i2c_device_id  i2c_si4702_id_table[] = {
	{"si4702", 0},
	{}
};

static struct i2c_driver si4702_driver = {
	.probe = si4702_probe,
	.remove = __devexit_p(si4702_remove),
	.id_table = i2c_si4702_id_table,
	.driver = {
		.owner = THIS_MODULE,
		.name = "si4702_driver",
	}
};

static int __init si4702_module_init(void)
{
	return i2c_add_driver(&si4702_driver);
}

static void __exit si4702_module_exit(void)
{
	i2c_del_driver(&si4702_driver);	
}

module_init(si4702_module_init);
module_exit(si4702_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sunplusedu");
