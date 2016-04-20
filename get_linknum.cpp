#include "share_memory.h"
#include "utils.h"

#include <libxml/parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <string>

using namespace std;

#define		FILE_PATH 		"/proc/net/nf_conntrack"

struct protocol_port{
	char protocol[10];
	vector<int> port;
};


int main(int argc,char **argv)
{
	Shm_mem mem;
	if (mem.shm_init() < 0)
	{
		syslog(LOG_ERR,"shared memory init failed");
		return 0;
	}

	struct protocol_port nfs = {"NFS"};			///方便从XML文件中读取NAS协议下的端口号
	struct protocol_port cifs = {"CIFS"};
	int collect = 0;	/// 收集间隔
	vector<string> ip;

	if (get_virip(ip) < 0)
	{
		syslog(LOG_ERR,"get virtual IP from CTDB ip failed");
		return 0;
	}
	if (get_port_by_protocol(nfs.protocol,nfs.port) < 0)
	{
		syslog(LOG_ERR,"get port by NFS protocol from XML file failed");
		return 0;
	}
	if (get_port_by_protocol(cifs.protocol,cifs.port) < 0)
	{
		syslog(LOG_ERR,"get port by CIFS protocol from XML file failed");
		return 0;
	}
	if ((collect = get_interval(INTERVAL_COLLECT_STRING)) <= 0)
	{
		syslog(LOG_ERR,"get collect interval from XML file failed");
		return 0;
	}

	nfs.port.push_back(22);
	cifs.port.push_back(123);

	if (mem.shmopen() < 0)
	{
		syslog(LOG_ERR,"the shared memory opened failed");
		return 0;
	}

	/*
	   vector<int>::iterator it;
	   for( it = nfs.port.begin(); it != nfs.port.end(); it++)
	   printf("nfs port:%d\n",*it);
	   for( it = cifs.port.begin(); it != cifs.port.end(); it++)
	   printf("cifs port:%d\n",*it);
	   printf("collect:%d   send:%d\n",collect,send);
	   */

	//	while(1)
	{
		vector<struct conn_data> data;
		parse_file(ip,nfs.port,cifs.port,data);

		vector<struct conn_data>::iterator it;
		for (it=data.begin();it!=data.end();it++)
			printf("ip:%s\ncifs_conn:%d\nnfs_conn:%d\n",
					(*it).ip,(*it).cifs_conn,(*it).nfs_conn);

		mem.shm_clear();
		if (mem.shm_write(data) < 0)
			syslog(LOG_ERR,"write the data into shared memory");
sleep:
		sleep(collect);
	}
}
