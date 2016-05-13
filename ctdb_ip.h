#ifndef		__CTDB_IP_H__
#define		__CTDB_IP_H__

#include <semaphore.h>
#include <sys/types.h>
#include <vector>
#include <string>

#define		MAX_DEV_VIRIP	1024
#define		SEM_NAME			"sem_dev_virip"
#define		SHM_NAME			"shm_dev_virip"

struct Dev_Virip{
	char devip[16];
	int num;
	char virip[1024][16];
};

/*
 *	根据指定的物理ip获取他的所有虚拟ip
 *	参数：
 *		devip，传入的物理IP，
 *		virip，用于存放虚拟IP的指针数组,
 *		确保指针指向的字符串长度不小于16
 *		size，用于指定虚拟IP的数组大小
 *	返回值：
 *		成功返回虚拟ip的格式，失败返回-1
 * */
int virip_by_devip(const char *devip,char virip[][16]);

/*
 *	根据指定的虚拟IP，获取对应的唯一的物理IP
 *	参数：
 *		virip，传入的虚拟IP
 *		devip，用于存放虚拟IP对应的物理IP
 *		length，指定devip的长度，
 *	成功返回物理IP的指针，失败返回NULL
 * */
char* devip_by_virip(const char *virip, char *devip);

/*
 *	初始化信号量和共享内存
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int shm_init(void);


/*
 *	打开信号量
 *	返回值：
 *		成功返回指针sem，失败返回NULL
 * */
sem_t *semopen(void);

/*
 *	打开共享内存
 *	返回值：
 *		成功返回指针shmptr，失败返回NULL
 * */
char *shmopen(void);

/*
 *	根据有名信号量将获取到的物理ip和虚拟IP放入全局变量dev_virip中
 *	成功返回0，失败返回-1
 * */
int update_dev_virip(char *shmptr,sem_t *sem);

#endif		//__CTDB_IP_H__
