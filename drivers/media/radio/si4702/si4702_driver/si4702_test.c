

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "si4702.h"

#define SI4702_PATH 	"/dev/fm_si4702"



typedef struct fm_list{
	unsigned int list[100];
	unsigned int act_num;
}fm_list_t;

fm_list_t my_list;

int main(void)
{
	struct si4702_channel_list chanlist;
	int fd;
	int vol = 12, seek_num;
	int i;
	int retval, rssi;
	
	/*打开设备文件*/
	fd = open(SI4702_PATH,O_RDWR);
	if(fd < 0){
		perror("SI4702_FM open:");
		return 0;
	}

	if(ioctl(fd, SI4702_AUTO_SEEK, &seek_num))
		printf("ioctl err!...\n");
	else
		printf("seek number is: %d\n", seek_num);

	if(seek_num)
	retval = read(fd, &chanlist, sizeof(chanlist));
	if(retval != sizeof(chanlist)){
		return -1;
		printf("read channel list failed!\n");
	}

	for(i=0; i<chanlist.index; i++)
		printf("channel: %d \n", chanlist.list[i]);

	while(1){
		if(ioctl(fd, SI4702_PLAY_NEXT, &rssi))
			break;
		sleep(1);
	}

	printf("mute test...\n");
	ioctl(fd, SI4702_SET_MUTE, SI4702_MUTE_ENABLE);
	sleep(2);
	ioctl(fd, SI4702_SET_MUTE, SI4702_MUTE_DISABLE);
	//sleep(1);
	while(vol--){
		if(ioctl(fd, SI4702_SET_VOLUME,vol))
			printf("ioctl SI4702_SET_VOLUME failed!!");
		sleep(1);
	}
	close(fd);

	return 0;
}

