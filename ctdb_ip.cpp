#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/mman.h>

#include "ctdb_ip.h"

#define		NODES_FILE			"/etc/ctdb/nodes"
#define		CTDB_IP_COMMAND		"/usr/bin/ctdb ip"

using namespace std;


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
int virip_by_devip(const char *devip,char virip[][16])
{
	if (devip == NULL)
		return -1;
	char *shmptr = shmopen();
	if (shmptr == NULL)
		return -1;
	sem_t *sem = semopen();
	if (sem == NULL)
		return -1;
	vector<struct Dev_Virip> temp;
	int num = 0;
	struct Dev_Virip *ptr = (struct Dev_Virip*)(shmptr+4);
	sem_wait(sem);
	memcpy(&num,shmptr,4);
	for (int i = 0; i < num;i++)
	{
		struct Dev_Virip tempip;
		memcpy(&tempip,ptr+i,sizeof(struct Dev_Virip));
		temp.push_back(tempip);
	}
	sem_post(sem);
	sem_close(sem);
	munmap(shmptr,MAX_DEV_VIRIP * sizeof(struct Dev_Virip));
	
	vector<struct Dev_Virip>::iterator it;
	for (it = temp.begin(); it != temp.end(); it++)
	{
		/*
		printf ("devip:%s\n",it->devip);
		for (int j = 0; j < it->num; j++)
			printf ("----virip:%s\n",it->virip[j]);
			*/
		if (strcmp(it->devip,devip) == 0)
		{
			for (int i = 0; i < it->num; i++)
				strncpy(virip[i],it->virip[i],sizeof(it->virip[i]));
			return it->num;
		}
	}
	return -1;
}

/*
 *	根据指定的虚拟IP，获取对应的唯一的物理IP
 *	参数：
 *		virip，传入的虚拟IP
 *		devip，用于存放虚拟IP对应的物理IP
 *	成功返回物理IP的指针，失败返回NULL
 * */
char* devip_by_virip(const char *virip, char *devip)
{
	if (virip == NULL)
		return NULL;
	char *shmptr = shmopen();
	if (shmptr == NULL)
		return NULL;
	sem_t *sem = semopen();
	if (sem == NULL)
		return NULL;
	vector<struct Dev_Virip> temp;
	int num = 0;
	struct Dev_Virip *ptr = (struct Dev_Virip*)(shmptr+4);
	sem_wait(sem);
	memcpy(&num,shmptr,4);
	for (int i = 0; i < num;i++)
	{
		struct Dev_Virip tempip;
		memcpy(&tempip,ptr+i,sizeof(struct Dev_Virip));
		temp.push_back(tempip);
	}
	sem_post(sem);
	sem_close(sem);
	munmap(shmptr,MAX_DEV_VIRIP * sizeof(struct Dev_Virip));
	
	vector<struct Dev_Virip>::iterator it;
	for (it = temp.begin(); it != temp.end(); it++)
		for (int i = 0; i < it->num; i++)
		{
			if (strcmp(virip,it->virip[i]) == 0)
			{
				strncpy(devip,it->devip,strlen(it->devip));
				return devip;
			}
		}
	return NULL;
}

/*
 *	根据/etc/ctdb/nodes/文件，依次获取所有的物理IP，
 *	并按照顺序放入向量dev_virip中,先确定dev_virip的size
 *	返回值：
 *		成功返回物理IP的个数，失败返回-1
 * */
static int get_all_devip(vector<struct Dev_Virip> &dev)
{
	dev.clear();
	FILE *fp = fopen(NODES_FILE,"r");
	if (fp == NULL)
	{
		syslog(LOG_ERR,"open %s failed\n",NODES_FILE);
		return -1;
	}
	///	解析nodes文件，获取到文件中的所有IP
	while(!feof(fp))
	{
		char line[1024] = {0};
		struct Dev_Virip tempip;
		memset(&tempip,0,sizeof(tempip));
		if (fgets(line,sizeof(line),fp) != NULL)
		{
			line[strlen(line)-1] = '\0';
			strncpy(tempip.devip,line,strlen(line));
			dev.push_back(tempip);
		}
	}
	fclose(fp);
	return 0;
}

/*
 *	通过调用/etc/init.d/ctdb ip命令 获取到所有的虚拟IP
 *	将其按照顺序放入向量dev_virip中
 *	返回值：
 *		成功返回dev_virip的元素个数，失败返回-1
 * */
static int get_dev_virip(vector<struct Dev_Virip> &dev)
{
	get_all_devip(dev);
	if (dev.empty())
		return -1;

	///	打开ctdb ip 并获取第一行数据的节点号
	FILE *fp = popen(CTDB_IP_COMMAND,"r");
	if (NULL == fp)
	{
		syslog(LOG_ERR,"%s execute failed",CTDB_IP_COMMAND);
		return -1;
	}

	char line[1024] = {0};
	if (fgets(line,sizeof(line),fp) == NULL)
	{
		syslog(LOG_ERR,"%s has not result",CTDB_IP_COMMAND);
		pclose(fp);
		return -1;
	}
	if (strstr(line,"Public IPs on node") == NULL)
	{
		syslog(LOG_ERR,"%s has wrong result",CTDB_IP_COMMAND);
		pclose(fp);
		return -1;
	}

	int i = 0;
	while (!feof(fp))
	{
		memset(line,0,sizeof(line));
		if (fgets(line,sizeof(line),fp) != NULL)
		{
			char *tnode = NULL;
			char *tip = NULL;
			tip = strtok(line," ");
			if (tip == NULL)
			{
				pclose(fp);
				return -1;
			}
			tnode = strtok(NULL," ");			/// 获取到该行的node值
			if (tnode == NULL)
			{
				pclose(fp);
				return -1;
			}
			int node = atoi(tnode);
			if (node > dev.size() - 1)
			{
				pclose(fp);
				return -1;
			}
			if (strlen(tip) > 15)
				return -1;
			strncpy(dev.at(node).virip[dev.at(node).num],tip,strlen(tip));
			dev.at(node).num++;
			i++;
		}
	}
	pclose(fp);
	return 0;
}

/*
 *	初始化信号量
 *	返回值：
 *		成功返回指针sem，失败返回NULL
 * */
int shm_init(void)
{
	int fd = shm_open(SHM_NAME,O_CREAT | O_RDWR,666);
	if (fd == -1)
		return -1;
	sem_t *sem = sem_open(SEM_NAME,O_CREAT | O_RDWR,666,1);
	if (sem == SEM_FAILED)
	{
		close(fd);
		return -1;
	}
	ftruncate(fd,4 + MAX_DEV_VIRIP * sizeof(struct Dev_Virip));
	close(fd);
	sem_close(sem);
	return 0;
}

char *shmopen(void)
{
	int fd = shm_open(SHM_NAME,O_RDWR,666);
	if (fd == -1)
		return NULL;
	char *shmptr = (char*)mmap(NULL,MAX_DEV_VIRIP*sizeof(struct Dev_Virip),
			PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	close(fd);
	if (shmptr == MAP_FAILED)
		return NULL;
	return shmptr;
}

sem_t *semopen(void)
{
	sem_t *sem = sem_open(SEM_NAME,O_RDWR);
	if (sem == SEM_FAILED)
		return NULL;
	return sem;
}

/*
 *	根据有名信号量将获取到的物理ip和虚拟IP放入全局变量dev_virip中
 *	成功返回0，失败返回-1
 * */
int update_dev_virip(char *shmptr,sem_t *sem)
{
	if (shmptr == NULL || sem == NULL)
		return -1;
	vector<struct Dev_Virip> dev;
	if (get_dev_virip(dev) < 0)
		return -1;
	int num = dev.size();
	sem_wait(sem);
	memcpy(shmptr,&num,4);
	struct Dev_Virip *ptr = (struct Dev_Virip*)(shmptr+4);
	for (int i = 0; i < dev.size(); i++)
		memcpy(ptr + i,&dev.at(i),sizeof(struct Dev_Virip));
	sem_post(sem);
	int a = 0;
	memcpy(&a,shmptr,4);
	return 0;
}
