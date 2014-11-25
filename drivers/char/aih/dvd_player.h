#ifndef __DVD_PLAYER_H__
#define __DVD_PLAYER_H__
#include <linux/types.h>

#define DVD_NAME "DVD_PLAYER"

//ioctl commands
#define DVD_MOTOR_GO_FORWARD 0





//DVD Status Flags
#define	DVD_CLASS_DVD_VIDEO									0x01
#define	DVD_CLASS_DVD_AUDIO									0x02
#define	DVD_CLASS_DVD_ISO									0x03
#define	DVD_CLASS_VCD										0x05
#define	DVD_CLASS_CD										0x80
#define	DVD_CLASS_MP4										0x11
#define	DVD_ENTER_DISC										0x01
#define	DVD_EXIT_DISC										0x00
#define	DVD_MENU_HAVE										0x02
#define	DVD_MENU_NO											0x00
#define	DVD_STOP											0x00
#define	DVD_PLAY											0x02
#define	DVD_PAUSE											0x08
#define	DVD_MODE_DISC										0x00
#define	DVD_MODE_SD											0x02
#define	DVD_MODE_USB										0x02
#define	DVD_PLAY_MODE_RPT_OFF								0x02
#define	DVD_PLAY_MODE_RPT_ALL								0x03
#define	DVD_PLAY_MODE_RPT_ONE								0x04
#define	DVD_PLAY_MODE_RAD_ON								0x0F
#define	DVD_PLAY_MODE_RAD_OFF								0x02
#define	DVD_MENU_ON											0x02
#define	DVD_MENU_OFF										0x00
#define	DVD_DISC_READ_OK									0x01
#define	DVD_DISC_READ_NO									0x00

typedef struct
{
	uint8_t							DiscCurrStatus;				//current disc status
	uint8_t							USD_SD_Status;				//sd card flag(is there a sd card or not)
	uint8_t							DVD_Mode;					//dvd current work mode(DVD USB SD)
	uint8_t							DVD_MENU_Status;			//ROOT MENU status
	uint8_t							DVD_Play_Status;			//����ģʽ����ͣ�����š�ֹͣ
	uint8_t							DVD_Mode_RPTStatus;			//����ģʽ�ظ�
	uint8_t							DVD_Mode_RADStatus;			//����ģʽ���
	uint8_t							DVD_Menu_Status;			//DVD��ǰ�˵�״̬
	uint8_t							DVD_Disc_Class_Status;		//dvd disc class
	uint8_t							DVD_Disc_Read_Done;			//dvd disc is already read
	uint8_t							DVD_Eject;					//0:exit disc 1:enter disc
	uint16_t						DVD_PlayModeSyncCount;		//ʵ�ʲ���״̬�밴����ʾ״̬ͬ��(��DVD����������ʾ����)
	uint16_t						DVD_WorkModeSyncCount;		//ʵ�ʲ���ģʽ����ʾģʽͬ��(��DVD����������ʾ����)
	uint16_t						DVD_RptModeSyncCount;		//ʵ���ظ�ģʽ����ʾ�Ĳ���״̬ͬ��
	uint16_t						DVD_RamModeSyncCount;		//ʵ�����ģʽ����ʾ�Ĳ���״̬ͬ��
	uint16_t						DVD_SetPlayTime_X;			//�ж�DVD��ת������ʱ���X��ֵ
	uint16_t						DVD_SetPlayTime_Y;			//�ж�DVD��ת������ʱ���Y��ֵ
}DVD_RT_STATUS;


struct dvd_player
{
	dev_t dev_num;
	struct cdev dev_cdev;
	struct class *dev_class;
	struct device *dev_device;
	struct file_operations *dev_fops;
	struct fasync_struct *async_irq;
	int dev_signal_status;
	int DiscEnterSignalStatus;
	int DiscExitSignalStatus;
	int DiscInsertSignalStatus;
	DVD_RT_STATUS DvdRtStatus;
};
typedef struct dvd_player DVD_PLAYER;

struct dvd_gpio_resource
{
	char gpio_name[20];
	int  gpio_num;
	unsigned int irq;
};
typedef struct dvd_gpio_resource DVD_GPIO;



#define PWM1H_H  gpio_direction_output(MotorPWM1H.gpio_num, 1)
#define PWM1H_L  gpio_direction_output(MotorPWM1H.gpio_num, 0)
#define PWM1L_H  gpio_direction_output(MotorPWM1L.gpio_num, 1)
#define PWM1L_L  gpio_direction_output(MotorPWM1L.gpio_num, 0)
#define PWM2H_H  gpio_direction_output(MotorPWM2H.gpio_num, 1)
#define PWM2H_L  gpio_direction_output(MotorPWM2H.gpio_num, 0)
#define PWM2L_H  gpio_direction_output(MotorPWM2L.gpio_num, 1)
#define PWM2L_L  gpio_direction_output(MotorPWM2L.gpio_num, 0)



#endif

