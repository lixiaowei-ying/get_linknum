#include "share_memory.h"
#include "utils.h"

#include <libxml/parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>

using namespace std;

#define		FILE_PATH 		"/proc/net/nf_conntrack"
#define		INIFILE_PATH	"/root/example.xml"

struct protocol_port{
	char protocol[10];
	vector<int> port;
};


int main(int argc,char **argv)
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
	parse_file(ip,nfs.port,nfs.protocol,data);
}
