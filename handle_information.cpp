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

#include <vector>
#include <string>

#include "utils.h"
#include "ctdb_ip.h"

using namespace std;

extern struct XmlData xml_data;
vector<struct LinkNum> link_num;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
extern netsnmp_session* ss;
extern netsnmp_pdu* pdu;

void* pthread_collect(void*)
{
	vector<string> virip;
	if (get_virip(virip) < 0)
	{
		syslog(LOG_ERR,"get virtual IP from CTDB failed");
		pthread_exit(NULL);
	}
	while(1)
	{
		vector<struct LinkNum> temp_link;
		if (parse_links_file(virip,xml_data.nfs_port,xml_data.cifs_port,temp_link) < 0)
			goto sleep;
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
	string device_ip;
	if (get_device_ip(device_ip) < 0)
	{
		syslog(LOG_ERR,"get actual device IP from CTDB ip failed");
		pthread_exit(NULL);
	}
	if (init_snmp_pdu(xml_data.dst_ip,xml_data.dst_port,
				xml_data.community,device_ip) < 0)
	{
		syslog(LOG_ERR,"init snmp pdu failed");
		pthread_exit(NULL);
	}

	vector<struct LinkNum> temp_link;
	while(1)
	{
		pthread_mutex_lock(&lock);
		temp_link = link_num;
		pthread_mutex_unlock(&lock);
		create_pdu(temp_link);
		if (!snmp_send(ss,pdu))
			snmp_perror("snmp_send");
		vector<struct LinkNum>::iterator it;
		for (it=temp_link.begin();it!=temp_link.end();it++)
			printf("virtual ip :%s  cifs:%d  nfs:%d \n",
					it->virip.c_str(),it->cifs_links,it->nfs_links);
		sleep(xml_data.send_interval);
	}
	snmp_close(ss);
	snmp_shutdown("send_linknum");
	SOCK_CLEANUP;
	pthread_exit(NULL);
}

static int process_handle_information(void)
{
	pthread_t pcollect,psend;
	int ret = pthread_create(&pcollect,NULL,pthread_collect,NULL);
	if (ret != 0)
	{
		syslog(LOG_ERR,"%s",strerror(ret));
		return -1;
	}
	
	ret = pthread_create(&psend,NULL,pthread_send,NULL);
	if (ret != 0)
	{
		syslog(LOG_ERR,"%s",strerror(ret));
		return -1;
	}

	if (pthread_join(pcollect,NULL) || 
			pthread_join(psend,NULL))
		return 0;
}

static int process_handle_virip(void)
{
	sem_t *sem = init_sem();
	if (sem == NULL)
		return 0;
	while(1)
	{
		update_dev_virip(sem);
		sleep(10);
	}
	sem_close(sem);
	return 0;
}

int main(int argc,char **argv)
{
	if (check_parse_xmlfile() < 0)
	{
		syslog(LOG_ERR,"The XML file is NOT right!");
		return 0;
	}

	printf("collect:%d  send:%d\n"
			"snmp dst_ip:%s dst_port:%d community:%s  version:%d\n"
			,xml_data.collect_interval,xml_data.send_interval,
			xml_data.dst_ip.c_str(),xml_data.dst_port,
			xml_data.community.c_str(),xml_data.version);
	vector<int>::iterator it;
	for(it=xml_data.cifs_port.begin();it!=xml_data.cifs_port.end();it++)
		printf("cifs port :%d \n",*it);
	for(it=xml_data.nfs_port.begin();it!=xml_data.nfs_port.end();it++)
		printf("nfs port :%d \n",*it);

	pid_t pid_information,pid_virip;

	/// 进程一：处理连接数的收集和发送程序
	pid_information = fork();
	if (pid_information == 0)
	{
		int ret = process_handle_information();
		exit(1);
	}

	/// 进程二：实时更新虚拟ip的共享内存
	pid_virip = fork();
	if (pid_virip == 0)
	{
		int ret = process_handle_virip();
		exit(0);
	}

	int status;
	pid_t pid = wait(&status);
	if (WIFEXITED(status))
	{
		pid = pid == pid_information ? pid_virip:pid_information;
		kill(pid,SIGKILL);
	}
	return 0;
}
