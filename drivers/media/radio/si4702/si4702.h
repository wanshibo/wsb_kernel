

#ifndef __SI4702_H
#define __SI4702_H


//fm control commands
#define SI4702_SET_MUTE				24
#define SI4702_SET_VOLUME				25
#define SI4702_SET_AREA					26
#define SI4702_SET_SPACE				27
#define SI4702_SET_SKMODE				28
#define SI4702_SET_SEEKTH				29
#define SI4702_SEEK_NEXT				30
#define SI4702_SEEK_PREV				31
#define SI4702_AUTO_SEEK				32
#define SI4702_PLAY_NEXT				33
#define SI4702_PLAY_PREV				34
#define SI4702_GET_CHANINFO			35

#define SI4702_ENABLE					1
#define SI4702_DISABLE					0


struct si4702_channel_info{
	unsigned int manu_id;
	unsigned int firm_version;
	unsigned int chip_version;

	unsigned int freq;
	unsigned int rssi;
	unsigned char stereo;
	unsigned char volume;
	unsigned char mute;
	unsigned char band;
};


#define 	CHANNEL_LIST_SIZE			100

/*struct channel list is used to save the active channels*/
struct si4702_channel_list{
	unsigned int list[CHANNEL_LIST_SIZE];			//电台列表
	unsigned int list_size;			//电台列表总大小
	unsigned int index;			//电台列表索引
	unsigned int index_cur;		//当前播放的电台索引
	
#define SI4702_AUTOSEEK_FINISHED	1
#define SI4702_AUTOSEEK_UNFINISH	0
	int state;					//电台列表有效状态
};


/* internal partation
  * @definations of driver, used for kernel
  */
//register POWERCFG 
#define SI4702_MUTE_ENABLE				0
#define SI4702_MUTE_DISABLE			1
#define SI4702_SMUTE_ENABLE			0
#define SI4702_SMUTE_DISABLE			1
#define SI4702_STEREO					0
#define SI4702_MONO					1
#define SI4702_RDS_STD					0
#define SI4702_RDS_VBS					1
#define SI4702_SKMODE_WRAP			0
#define SI4702_SKMODE_HALT				1
#define SI4702_SEEK_UP					1
#define SI4702_SEEK_DOWN				0
#define SI4702_SEEK_ENABLE				1
#define SI4702_SEEK_DISABLE				0

//register SYSCONFIG1
#define SI4702_DE_50US					1
#define SI4702_DE_75US					0
#define SI4702_BLNDADJ_31_49			0
#define SI4702_BLNDADJ_37_55			1
#define SI4702_BLNDADJ_19_37			2
#define SI4702_BLNDADJ_25_43			3

//register SYSCONFIG2
#define SI4702_SEEKTH_MAX				0x7f
#define SI4702_SEEKTH_MIN				0x00
#define SI4702_BAND_87to108				0
#define SI4702_BAND_76to108				1
#define SI4702_BAND_76to90				2
#define SI4702_BAND_MAX				SI4702_BAND_76to90
#define SI4702_BAND_RESERVED			3
#define SI4702_SPACE_200KHZ			0
#define SI4702_SPACE_100KHZ			1
#define SI4702_SPACE_50KHZ				2
#define SI4702_SPACE_MAX				SI4702_SPACE_50KHZ
#define SI4702_VOLUME_MAX				0x0f

//register SYSCONFIG3
#define SI4702_SMUTER_FASTEST			0
#define SI4702_SMUTER_FAST				1
#define SI4702_SMUTER_SLOW			2
#define SI4702_SMUTER_SLOWEST			3
#define SI4702_SMUTEA_16dB				0
#define SI4702_SMUTEA_14dB				1
#define SI4702_SMUTEA_12dB				2
#define SI4702_SMUTEA_10dB				3
#define SI4702_VOLEXT_ENABLE			1
#define SI4702_VOLEXT_DISABLE			0
#define SI4702_SKSNR_DISABLE			0
#define SI4702_SKSNR_MIN				1
#define SI4702_SKSNR_MAX				0x0f
#define SI4702_SKCNR_DISABLE			0
#define SI4702_SKCNR_MIN				0x0f
#define SI4702_SKCNR_MAX				1

//register TEST1
#define SI4702_XOSCEN_ENABLE			1
#define SI4702_XOSCEN_DISABLE			0
#define SI4702_AHIZEN_ENABLE			1
#define SI4702_AHIZEN_DISABLE			0


//register STATUSRSSI
#define 	SI4702_STC_HIGH			1
#define 	SI4702_STC_LOW			0
#define 	SI4702_SFBL_HIGH			1
#define 	SI4702_SFBL_LOW			0


typedef	unsigned short		u16bit;

/* the definition of bits in registers, one or some bits can configure one attribute of 
  * si4702 chip
  *
  * @si4702共有16个寄存器，每个寄存器大小为16bit，寄存器
  * 中有多个part，用来控制芯片的不同属性和获取状态信息，
  * 以下结构体完成了对寄存器的抽象
  */
struct reg_bit_def {
	/*register DEVICEID*/
	u16bit mfgid:12;
	u16bit pn:4;

	/*register CHIPID*/
	u16bit firmware:6;
	u16bit dev:4;
	u16bit rev:6;

	/*register POWERCFG*/
	u16bit enable:1;
	u16bit :5;
	u16bit disable:1;
	u16bit :1;
	u16bit seek:1;
	u16bit seekup:1;
	u16bit skmode:1;
	u16bit rdsm:1;
	u16bit :1;
	u16bit mono:1;
	u16bit dmute:1;
	u16bit dsmute:1;

	/*register CHANNEL*/
	u16bit chan:10;
	u16bit :5;
	u16bit tune:1;

	/*register SYSCONFIG1*/
	u16bit gpio1:2;
	u16bit gpio2:2;
	u16bit gpio3:2;
	u16bit blndadj:2;
	u16bit :2;
	u16bit agcd:1;
	u16bit de:1;
	u16bit rds:1;
	u16bit :1;
	u16bit stcien:1;
	u16bit rdsien:1;

	/*register SYSCONFIG2*/
	u16bit volume:4;
	u16bit space:2;
	u16bit band:2;
	u16bit seekth:8;

	/*register SYSCONFIG3*/
	u16bit skcnt:4;
	u16bit sksnr:4;
	u16bit volext:1;
	u16bit :3;
	u16bit smutea:2;
	u16bit smuter:2;

	/*register TEST1*/
	u16bit :14;
	u16bit ahizen:1;
	u16bit xoscen:1;

	/*register TEST2*/
	u16bit reserve0;
	
	/*register BOOTCONFIG*/
	u16bit reserve1;

	/*register STATUSRSSI*/
	u16bit rssi:8;
	u16bit st:1;
	u16bit blera:2;
	u16bit rdss:1;
	u16bit afcrl:1;
	u16bit sfbl:1;
	u16bit stc:1;
	u16bit rdsr:1;

	/*register READCHAN*/
	u16bit readchan:10;
	u16bit blerd:2;
	u16bit blerc:2;
	u16bit blerb:2;

	/*register RDSA~RDSD*/
	u16bit rdsa;
	u16bit rdsb;
	u16bit rdsc;
	u16bit rdsd;
};

union si4702_regval{
	unsigned char buf[32];
	struct reg_bit_def reg;
};


static inline unsigned int get_readchan_from_status(union si4702_regval *regval)
{
	return regval->reg.readchan;
}
static inline unsigned int get_stc_from_status(union si4702_regval *regval)
{
	return regval->reg.stc;
}
static inline unsigned int get_sfbl_from_status(union si4702_regval *regval)
{
	return regval->reg.sfbl;
}
static inline unsigned int get_rssi_from_status(union si4702_regval *regval)
{
	return regval->reg.rssi;
}
static inline unsigned int get_chan_from_status(union si4702_regval *regval)
{
	return regval->reg.chan;
}



#endif
