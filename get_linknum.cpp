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

//	while(1)
	{
		struct protocol_port nfs = {"NFS"};
		struct protocol_port cifs = {"CIFS"};

		int collect,send;	/// 收集间隔和发送间隔

		get_port_by_protocol(nfs.protocol,nfs.port);
		get_port_by_protocol(cifs.protocol,cifs.port);
		collect = get_interval("Collect");
		send = get_interval("Send");

		/*
		   vector<int>::iterator it;
		   for( it = nfs.port.begin(); it != nfs.port.end(); it++)
		   printf("nfs port:%d\n",*it);
		   for( it = cifs.port.begin(); it != cifs.port.end(); it++)
		   printf("cifs port:%d\n",*it);

		   printf("collect:%d   send:%d\n",collect,send);
		   */

		vector<struct conn_data> data;
		vector<string> ip;
		ip.push_back("192.168.1.4");
		nfs.port.push_back(22);
		cifs.port.push_back(123);
		parse_file(ip,nfs.port,cifs.port,data);

		   vector<struct conn_data>::iterator it;
		   for (it=data.begin();it!=data.end();it++)
		   printf("ip:%s\ncifs_conn:%d\nnfs_conn:%d\n",
		   (*it).ip,(*it).cifs_conn,(*it).nfs_conn);

		if (mem.shmopen() < 0)
		{
			syslog(LOG_ERR,"the shared memory opened failed");
			goto sleep;
		}
		mem.shm_clear();
		if (mem.shm_write(data) < 0)
			syslog(LOG_ERR,"write the data into shared memory");
sleep:
		sleep(collect);
	}
}
