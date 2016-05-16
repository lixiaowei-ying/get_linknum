#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <vector>
#include <string>

#include "utils.h"
#include "ctdb_ip.h"

#define		UPDATE_VIRIP_INTERVAL	1
using namespace std;

extern struct XmlData xml_data;
vector<struct LinkNum> link_num;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
extern netsnmp_session* ss;
extern netsnmp_pdu* pdu;


void* pthread_collect(void*)
{
	while(1)
	{
		if (getppid() == 1)
			pthread_exit(NULL);
		vector<string> virip;
		vector<struct LinkNum> temp_link;
		if (get_virip(virip) < 0)
		{
			syslog(LOG_ERR,"get virtual ip from ctdb ip failed");
			goto sleep;
		}
		if (parse_links_file(virip,xml_data.nfs_port,xml_data.cifs_port,temp_link) < 0)
		{
			syslog(LOG_ERR,"parse nf_conntrack file failed");
			goto sleep;
		}
		pthread_mutex_lock(&lock);
		link_num = temp_link;
		pthread_mutex_unlock(&lock);
sleep:
		sleep(xml_data.collect_interval);
	}
	pthread_exit(NULL);
}

void* pthread_send(void*)
{
	init_snmp("send_linknum");
	while(1)
	{
		if (getppid() == 1)
			pthread_exit(NULL);
		vector<struct LinkNum> temp_link;
		string device_ip;
		if (get_device_ip(device_ip) < 0)
		{
			syslog(LOG_ERR,"get device ip from ctdb ip failed");
			goto sleep;
		}
		if (init_snmp_pdu(xml_data.dst_ip,xml_data.dst_port,
					xml_data.community,device_ip) < 0)
		{
			syslog(LOG_ERR,"init snmp pdu failed");
			goto sleep;
		}
		pthread_mutex_lock(&lock);
		temp_link = link_num;
		pthread_mutex_unlock(&lock);
		if (create_pdu(temp_link) == 0)
			if (!snmp_send(ss,pdu))
				snmp_perror("snmp_send");
		snmp_close(ss);
sleep:
		sleep(xml_data.send_interval);
	}
	SOCK_CLEANUP;
	snmp_shutdown("send_linknum");
	pthread_exit(NULL);
}

static int process_handle_information(void)
{
	pthread_t pcollect,psend;
	/// 创建收集连接数信息的线程
	int ret = pthread_create(&pcollect,NULL,pthread_collect,NULL);
	if (ret != 0)
	{
		syslog(LOG_ERR,"%s",strerror(ret));
		return -1;
	}
	
	/// 创建发送连接数的线程
	ret = pthread_create(&psend,NULL,pthread_send,NULL);
	if (ret != 0)
	{
		syslog(LOG_ERR,"%s",strerror(ret));
		return -1;
	}

	/// 只要有一个线程退出，就返回
	if (pthread_join(pcollect,NULL) || 
			pthread_join(psend,NULL))
		return 0;
}

static int process_handle_virip(void)
{
	int ret = shm_init();
	if (ret < 0)
		return 0;
	char *shmptr = shmopen();
	if (shmptr == NULL)
		return 0;
	sem_t *sem = semopen();
	if (sem == NULL)
	{
		munmap(shmptr,MAX_DEV_VIRIP * sizeof(struct Dev_Virip));
		return 0;
	}
	while(1)
	{
		if (getppid() == 1)
			exit(0);
		update_dev_virip(shmptr,sem);
		sleep(UPDATE_VIRIP_INTERVAL);
	}
	sem_close(sem);
	return 0;
}

void main_process(void)
{
	if (check_parse_xmlfile() < 0)
	{
		syslog(LOG_ERR,"The XML file is NOT right!");
		return ;
	}
	pid_t pid_information,pid_virip;

	/// 进程一：处理连接数的收集和发送程序
	pid_information = fork();
	if (pid_information == 0)
	{
		process_handle_information();
		exit(1);
	}

	/// 进程二：实时更新虚拟ip的共享内存
	pid_virip = fork();
	if (pid_virip == 0)
	{
		process_handle_virip();
		exit(0);
	}
	int status;
	pid_t pid = wait(&status);
	if (WIFEXITED(status))
	{
		pid = pid == pid_information ? pid_virip:pid_information;
		kill(pid,SIGKILL);
	}
	return ;
}

int main(int argc,char **argv)
{
	pid_t pid = fork();
	if (pid == 0)
	{
		setsid();
		main_process();
	}
	return 0;
}
