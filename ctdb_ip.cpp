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

#include "ctdb_ip.h"

#define		NODES_FILE			"/etc/ctdb/nodes"
#define		CTDB_IP				"/etc/init.d/ctdb ip"
#define		SEM_NAME			"sem_dev_virip"

using namespace std;

struct Dev_Virip{
	string devip;
	vector<string> virip;
};

vector<struct Dev_Virip> dev_virip;


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
int virip_by_devip(const char *devip,char *virip[],size_t size)
{
	if (devip == NULL)
		return -1;
	vector<struct Dev_Virip> temp;
	sem_t *sem = sem_open(SEM_NAME,O_RDWR);
	if (sem == SEM_FAILED)
	{
		syslog(LOG_ERR,"sem open failed");
		return -1;
	}
	sem_wait(sem);
	temp = dev_virip;
	sem_post(sem);
	sem_close(sem);
	
	vector<struct Dev_Virip>::iterator it;
	for (it = temp.begin(); it != temp.end(); it++)
		if (it->devip == string(devip))
		{
			if (size < it->virip.size())
				return -1;
			for (int i = 0; i < it->virip.size(); i++)
				strcpy(virip[i],it->virip.at(i).c_str());
			return it->virip.size();
		}
	return -1;
}

/*
 *	根据指定的虚拟IP，获取对应的唯一的物理IP
 *	参数：
 *		virip，传入的虚拟IP
 *		devip，用于存放虚拟IP对应的物理IP
 *		length，指定devip的长度，
 *	成功返回物理IP的指针，失败返回NULL
 * */
char* devip_by_virip(const char *virip, char *devip,size_t length)
{
	if (devip == NULL)
		return NULL;
	vector<struct Dev_Virip> temp;
	sem_t *sem = sem_open(SEM_NAME,O_RDWR);
	if (sem == SEM_FAILED)
	{
		syslog(LOG_ERR,"sem open failed");
		return NULL;
	}
	sem_wait(sem);
	temp = dev_virip;
	sem_post(sem);
	sem_close(sem);
	
	vector<struct Dev_Virip>::iterator it;
	for (it = temp.begin(); it != temp.end(); it++)
		for (int i = 0; i < it->virip.size(); i++)
		{
			if (string(virip) == it->virip.at(i))
			{
				strncpy(devip,it->devip.c_str(),length);
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
		memset(&tempip,0,sizeof(struct Dev_Virip));
		if (fgets(line,sizeof(line),fp) != NULL)
			tempip.devip = string(line);
		dev.push_back(tempip);
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
	if (dev_virip.empty())
		return -1;

	///	打开ctdb ip 并获取第一行数据的节点号
	FILE *fp = popen("/etc/init.d/ctdb ip","r");
	if (NULL == fp)
	{
		syslog(LOG_ERR,"/etc/init.d/ctdb ip execute failed");
		return -1;
	}

	char line[1024] = {0};
	if (fgets(line,sizeof(line),fp) == NULL)
	{
		syslog(LOG_ERR,"/etc/init.d/ctdb ip has not result");
		pclose(fp);
		return -1;
	}
	if (strstr(line,"Public IPs on node") == NULL)
	{
		syslog(LOG_ERR,"/etc/init.d/ctdb ip has wrong result");
		pclose(fp);
		return -1;
	}

	int index = 0;
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
			if (node > dev_virip.size() - 1)
			{
				pclose(fp);
				return -1;
			}
			dev.at(node).virip.push_back(tip);
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
sem_t *init_sem(void)
{
	sem_t *sem = sem_open(SEM_NAME,O_CREAT|O_RDWR,666,1);
	if (sem == SEM_FAILED)
	{
		syslog(LOG_ERR,"sem init failed");
		return NULL;
	}
	return sem;
}

/*
 *	根据有名信号量将获取到的物理ip和虚拟IP放入全局变量dev_virip中
 *	成功返回0，失败返回-1
 * */
int update_dev_virip(sem_t *sem)
{
	vector<struct Dev_Virip> dev;
	if (get_dev_virip(dev) < 0)
		return -1;
	sem_wait(sem);
	dev_virip = dev;
	sem_post(sem);
	return 0;
}
