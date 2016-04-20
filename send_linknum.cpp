#include "share_memory.h"
#include "utils.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <libxml/parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <string>

#define 	DATA_TYPE_LINKNUM	1
#define		PROTOCOL_VERSION	"1.0"
#define		DEFINED_OID		".1.3.6.1.4.1.16550"

oid objid_sysuptime[] 		= { 1, 3, 6, 1, 2, 1, 1, 3, 0 }; 
oid objid_snmptrap[] 		= { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 }; 
oid objid_proto_version[] 	= { 1, 3, 6, 1, 4, 1, 16550, 0  }; 
oid objid_current_time[] 	= { 1, 3, 6, 1, 4, 1, 16550, 1  }; 
oid objid_data_type[] 		= { 1, 3, 6, 1, 4, 1, 16550, 2  }; 
oid objid_device_ip[] 		= { 1, 3, 6, 1, 4, 1, 16550, 3  }; 
oid objid_data_length[] 	= { 1, 3, 6, 1, 4, 1, 16550, 4  }; 
oid objid_virtual_ip[] 		= { 1, 3, 6, 1, 4, 1, 16550, 5  }; 
oid objid_nfs_links[] 		= { 1, 3, 6, 1, 4, 1, 16550, 6  }; 
oid objid_cifs_links[] 		= { 1, 3, 6, 1, 4, 1, 16550, 7  }; 

using namespace std;

netsnmp_session *ss;
netsnmp_pdu *pdu = NULL;

/*
 *	初始化snmp并填充pdu的初始部分
 *	成功返回0，失败返回-1
 * */
int init_snmp_pdu(string &dst_ip,int dst_port,
		string &community,string &device_ip)
{
	if (dst_ip.empty() || dst_port <= 0 
			|| community.empty() ||
			device_ip.empty())
		return -1;
	init_snmp("send_linknum");
	struct snmp_session session;
	snmp_sess_init(&session);
	
	char peername[32] = {0};
	char comm[16] = {0};
	char dev_ip[16] = {0};

	sprintf(peername,"%s:%d",dst_ip.c_str(),dst_port);
	strncpy(comm,community.c_str(),sizeof(comm));
	strncpy(dev_ip,device_ip.c_str(),sizeof(dev_ip));

	session.version = SNMP_VERSION_2c;
	session.peername = peername;
	session.community = (unsigned char *)comm;
	session.community_len = strlen(comm);
	ss = snmp_open(&session);
	if (ss == NULL)
	{
		snmp_perror("snmp_open");
		return -1;
	}
	
	pdu = snmp_pdu_create(SNMP_MSG_TRAP2);
	if (NULL == pdu)
	{
		snmp_perror("snmp_pdu_create");
		return -1;
	}

	char systimebuf[128] = {0};
	char timebuf[128] = {0};
	char data_typebuf[24] = {0};
	sprintf(systimebuf,"%ld",get_uptime());
	sprintf(timebuf,"%ld",time(NULL));
	sprintf(data_typebuf,"%ld",DATA_TYPE_LINKNUM);

	/// 以下的元素增加有顺序之说，不应随意变动
	snmp_add_var(pdu,objid_sysuptime,sizeof(objid_sysuptime)/sizeof(oid),'t',systimebuf);
	snmp_add_var(pdu,objid_snmptrap,sizeof(objid_snmptrap)/sizeof(oid),'o',DEFINED_OID);
	snmp_add_var(pdu,objid_proto_version,sizeof(objid_proto_version)/sizeof(oid),'s',PROTOCOL_VERSION);
	snmp_add_var(pdu,objid_current_time,sizeof(objid_current_time)/sizeof(oid),'i',timebuf);
	snmp_add_var(pdu,objid_data_type,sizeof(objid_data_type)/sizeof(oid),'i',data_typebuf);
	snmp_add_var(pdu,objid_device_ip,sizeof(objid_device_ip)/sizeof(oid),'s',dev_ip);
	
	return 0;
}

/*
 *	根据向量data中的数据构造snmp的PDU
 *	返回值：
 *		成功返回0，失败-1
 * */
int create_pdu(vector<struct conn_data> &data)
{
	
}

int main(int argc, char **argv)
{
	string dst_ip;
	string community;
	string device_ip;
	int version = 0;
	int dst_port = 0;
	int send_interval = 0;

	if (get_community(community) < 0)
	{
		syslog(LOG_ERR,"get community from XML file failed");
		return 0;
	}
	if (get_dstip(dst_ip) < 0)
	{
		syslog(LOG_ERR,"get dst IP from XML file failed");
		return 0;
	}
	if ((dst_port = get_dstport()) <= 0)
	{
		syslog(LOG_ERR,"get dst port from XML file failed");
		return 0;
	}
	if ((send_interval = get_interval(INTERVAL_SEND_STRING)) <= 0)
	{
		syslog(LOG_ERR,"get send interval from XML file failed");
		return 0;
	}
	if ((version = get_version()) != 2)
	{
		syslog(LOG_ERR,"The SNMP version in XML file must be 2");
		return 0;
	}
	if (get_device_ip(device_ip) < 0)
	{
		syslog(LOG_ERR,"get actual device IP from CTDB ip failed");
		return 0;
	}

//	printf("version:%d  dstip:%s  dstport:%d  comm:%s\n",
//			version,dst_ip.c_str(),dst_port,community.c_str());

	Shm_mem mem;
	if (mem.shmopen() < 0)
	{
		syslog(LOG_ERR,"the shared memory opened failed");
		return 0;
	}

	if (init_snmp_pdu(dst_ip,dst_port,community,device_ip) < 0)
	{
		syslog(LOG_ERR,"init snmp pdu failed");
		return 0;
	}

	vector<struct conn_data>::iterator it;
	size_t total_length;
	char lengthbuf[64] = {0};

//	while(1)
	{
		vector<struct conn_data> data;
		if (mem.shm_read(data) < 0)
		{
			syslog(LOG_ERR,"read the data from the shared memory");
			goto sleep;
		}
		/*	
		for( it = data.begin(); it != data.end(); it++)
			printf("ip:%s\nnfs_conn:%d\ncifs_conn:%d\n",(*it).ip,(*it).cifs_conn,(*it).nfs_conn);
			*/

		memset(lengthbuf,0,sizeof(lengthbuf));
		total_length = data.size() * sizeof(struct conn_data);
		sprintf(lengthbuf,"%d",total_length);
		snmp_add_var(pdu,objid_data_length,sizeof(objid_data_length)/sizeof(oid),'i',lengthbuf);

		for( it = data.begin(); it != data.end(); it++)
		{
			char cifsbuf[64] = {0};
			char nfsbuf[64] = {0};
			sprintf(cifsbuf,"%d",it->cifs_conn);
			sprintf(nfsbuf,"%d",it->nfs_conn);
			snmp_add_var(pdu,objid_virtual_ip,sizeof(objid_virtual_ip)/sizeof(oid),'s',it->ip);
			snmp_add_var(pdu,objid_nfs_links,sizeof(objid_nfs_links)/sizeof(oid),'i',cifsbuf);
			snmp_add_var(pdu,objid_cifs_links,sizeof(objid_cifs_links)/sizeof(oid),'i',nfsbuf);
		}
		if (!snmp_send(ss,pdu))
			syslog(LOG_ERR,"snmp send PDU error");

sleep:
		sleep(send_interval);
	}
	snmp_close(ss);
	snmp_shutdown("send_linknum");
	SOCK_CLEANUP;
}
