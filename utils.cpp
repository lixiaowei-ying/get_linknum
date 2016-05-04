#include "utils.h"

#include <libxml/parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>


#define		LINKS_FILE_PATH 			"/proc/net/nf_conntrack"
#define		NODES_FILE					"/etc/ctdb/nodes"
#define		INIFILE_PATH				"/root/example.xml"
#define		INTERVAL_STRING_SEND		"Send"
#define		INTERVAL_STRING_COLLECT		"Collect"
#define		PROTOCOL_STRING_CIFS		"CIFS"
#define		PROTOCOL_STRING_NFS			"NFS"

#define 	DATA_TYPE_LINKNUM	1
#define		PROTOCOL_VERSION	"1.0"
#define		DEFINED_OID			".1.3.6.1.4.1.16550"

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
struct XmlData xml_data;
netsnmp_session *ss = NULL;
netsnmp_pdu *pdu = NULL;

/*
 *	通过传入的node当前节点，解析XML中protocol子节点中的port字段
 *	并将port放入结构体xml_data中。
 * */
static void deal_xml_protocol(xmlNodePtr curnode)
{
	xmlNodePtr propnode = curnode;									///	属性节点
	if (xmlHasProp(curnode,BAD_CAST "name"))
		propnode = curnode;
	xmlAttrPtr attrptr = propnode->properties;
	while(attrptr != NULL)
	{
		if (!xmlStrcmp(attrptr->name,BAD_CAST "name"))					/// 比较属性的名称是name
		{
			xmlChar *attrkey = xmlGetProp(propnode,BAD_CAST "name");	/// 获取节点属性的值
			xmlChar *key = xmlNodeGetContent(curnode);					/// 获取节点的值
			if (key)
			{
				/// 处理CIFS协议
				if (!strcmp((char*)attrkey,PROTOCOL_STRING_CIFS))		/// 节点属性为CIFS
				{
					xmlNodePtr temp = curnode->xmlChildrenNode;			/// 指向CIFS的子节点
					while(temp != NULL)
					{
						if (!xmlStrcmp(temp->name,BAD_CAST "Port"))		/// 获取多个Port的值
						{
							xmlChar *childkey = xmlNodeGetContent(temp);
							if (childkey)
							{
								xml_data.cifs_port.push_back(atoi((char*)childkey));		/// 放入向量中
								xmlFree(childkey);
							}
						}
						temp = temp->next;
					}
				}
				/// 处理NFS协议
				if (!strcmp((char*)attrkey,PROTOCOL_STRING_NFS))		/// 节点属性为CIFS
				{
					xmlNodePtr temp = curnode->xmlChildrenNode;			/// 指向CIFS的子节点
					while(temp != NULL)
					{
						if (!xmlStrcmp(temp->name,BAD_CAST "Port"))		/// 获取多个Port的值
						{
							xmlChar *childkey = xmlNodeGetContent(temp);
							if (childkey)
							{
								xml_data.nfs_port.push_back(atoi((char*)childkey));		/// 放入向量中
								xmlFree(childkey);
							}
						}
						temp = temp->next;
					}
				}
				xmlFree(key);
			}
			if (attrkey)
				xmlFree(attrkey);
		}
		attrptr = attrptr->next;
	}
}

/*
 *	通过传入的node当前节点，解析XML中snmp子节点中的所有字段
 *	并将所有字段值放入结构体xml_data中。
 * */
static void deal_xml_snmp(xmlNodePtr curnode)
{
	xmlNodePtr 	childnode = curnode->xmlChildrenNode;
	while(childnode != NULL)
	{
		if (!xmlStrcmp(childnode->name,BAD_CAST "Dst_ip"))
		{
			xmlChar *key = xmlNodeGetContent(childnode);						/// 获取节点的值
			if (key)
			{
				xml_data.dst_ip = string((char *)key);
				xmlFree(key);
			}
		}

		if (!xmlStrcmp(childnode->name,BAD_CAST "Dst_port"))
		{
			xmlChar *key = xmlNodeGetContent(childnode);						/// 获取节点的值
			if (key)
			{
				xml_data.dst_port = atoi((char *)key);
				xmlFree(key);
			}
		}

		if (!xmlStrcmp(childnode->name,BAD_CAST "Community"))
		{
			xmlChar *key = xmlNodeGetContent(childnode);						/// 获取节点的值
			if (key)
			{
				xml_data.community = string((char *)key);
				xmlFree(key);
			}
		}

		if (!xmlStrcmp(childnode->name,BAD_CAST "Version"))
		{
			xmlChar *key = xmlNodeGetContent(childnode);						/// 获取节点的值
			if (key)
			{
				xml_data.version = atoi((char *)key);
				xmlFree(key);
			}
		}
		childnode = childnode->next;
	}
}

/*
 *	通过传入的node当前节点，解析XML中interval子节点中的所有字段
 *	并将collect_interval、send_interval放入结构体xml_data中。
 * */
static void deal_xml_interval(xmlNodePtr curnode)
{
	xmlNodePtr propnode = curnode;									///	属性节点
	if (xmlHasProp(curnode,BAD_CAST "name"))
		propnode = curnode;
	xmlAttrPtr attrptr = propnode->properties;
	while(attrptr != NULL)
	{
		if (!xmlStrcmp(attrptr->name,BAD_CAST "name"))
		{
			xmlChar *attrkey = xmlGetProp(propnode,BAD_CAST "name");
			xmlChar *key = xmlNodeGetContent(curnode);						/// 获取节点的值
			if (key)
			{
				if (!strcmp((char*)attrkey,INTERVAL_STRING_SEND))	///	获取Send的值
					xml_data.send_interval = atoi((char*)key);
				if (!strcmp((char*)attrkey,INTERVAL_STRING_COLLECT))	///	获取Collect的值
					xml_data.collect_interval = atoi((char*)key);
				xmlFree(key);
			}
			if (attrkey)
				xmlFree(attrkey);
		}
		attrptr = attrptr->next;
	}
}


/*
 *	检测结构体xml_data中的数据是否是期望值
 *	返回值：
 *		成功返回0，失败返回-1
 * */
static int check_xmlfile(void)
{
	if (xml_data.collect_interval <= 0 ||
		xml_data.send_interval <= 0 ||
		xml_data.dst_port <= 0 ||
		xml_data.version != 2 ||
		xml_data.dst_ip.empty() ||
		xml_data.community.empty() ||
		xml_data.cifs_port.empty() ||
		xml_data.nfs_port.empty()
		)
		return -1;
}

/*
 *	检测XML文件并解析文件，
 *	将解析到的数据放入结构体xml_data中
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int check_parse_xmlfile(void)
{
	memset(&xml_data,0,sizeof(struct XmlData));

	xmlDocPtr 	doc;
	xmlNodePtr 	curnode;
	doc = xmlReadFile(INIFILE_PATH,"UTF-8",XML_PARSE_RECOVER);		/// 指定文件格式
	if (NULL == doc)
		return -1;

	xmlKeepBlanksDefault(0);										/// 忽略空白
	curnode = xmlDocGetRootElement(doc);							///	获得根节点
	if (NULL == curnode)
	{
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(curnode->name,BAD_CAST "Numconn"))				///	对比根节点是否是期望值
	{
		xmlFreeDoc(doc);
		return -1;
	}
	
	curnode = curnode->xmlChildrenNode;								///	指向根节点的子节点
	while(curnode != NULL)
	{
		/// 处理Protocol子节点
		if (!xmlStrcmp(curnode->name,BAD_CAST "Protocol"))
			deal_xml_protocol(curnode);
		/// 处理snmp子节点
		if (!xmlStrcmp(curnode->name,BAD_CAST "Snmp"))
			deal_xml_snmp(curnode);
		/// 处理interval子节点
		if (!xmlStrcmp(curnode->name,BAD_CAST "Interval"))
			deal_xml_interval(curnode);
		curnode = curnode->next;
	}
	xmlFreeDoc(doc);
	return check_xmlfile();
}


/*
 *	通过CTDB获取节点主机的虚拟IP
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int get_virip(vector<string> &ip)
{
	ip.clear();
	ip.push_back("192.168.1.4");
	return 0;
	///	打开ctdb ip 并获取第一行数据的节点号
	FILE *fp = popen("/etc/init.d/ctdb ip","r");
	if (NULL == fp)
	{
		syslog(LOG_ERR,"/etc/init.d/ctdb ip execute failed");
		return -1;
	}
	char line[1024] = {0};
	int node = -1;
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
	node = atoi(line+strlen("Public IPs on node "));		///	读取出第一行该节点的数值
	if (node < 0)
	{
		syslog(LOG_ERR,"get the node of the device failed");
		pclose(fp);
		return -1;
	}

	while (!feof(fp))
	{
		memset(line,0,sizeof(line));
		if (fgets(line,sizeof(line),fp) != NULL)
		{
			char *tok = NULL;
			char *tip = NULL;
			tip = strtok(line," ");
			if (tip == NULL)
			{
				pclose(fp);
				return -1;
			}
			tok = strtok(NULL," ");			/// 获取到该行的node值
			if (tok == NULL)
			{
				pclose(fp);
				return -1;
			}
			if (atoi(tok) == node)			/// 如果node值和本节点的node相等，添加该虚拟IP
				ip.push_back(string(tip));
		}
	}
	pclose(fp);
	return 0;
}

/*
 *	通过ioctl调用，根据接口名获取到当接口的IP
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int ip_by_interface(string &interface,string &ip)
{
	int sock;  
	struct ifreq ifr;  

	sock = socket(AF_INET, SOCK_DGRAM, 0);  
	if (sock == -1)  
	{  
		syslog(LOG_ERR,"create socket failed");
		return -1;                  
	}  

	strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ);  
	ifr.ifr_name[IFNAMSIZ - 1] = 0;  

	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)  
	{  
		syslog(LOG_ERR,"ioctl failed");
		return -1;  
	}  

	/// 将取得到的IP赋值给传出型参数ip
	char addr[INET_ADDRSTRLEN] = {0};
	if (inet_ntop(AF_INET,&ifr.ifr_addr,addr,INET_ADDRSTRLEN))
		ip = string(addr);
	close(sock);
	return 0;  
	
}

/*
 *	通过读取/etc/ctdb/nodes文件获取到多个IP
 *	然后和当前节点下的物理ip进行比对，
 *	然后得到真实的物理IP
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int get_device_ip(std::string &ip)
{
	ip.clear();
	ip = "192.168.1.4";
	return 0;
	FILE *fp = fopen(NODES_FILE,"r");
	if (fp == NULL)
	{
		syslog(LOG_ERR,"open %s failed\n",NODES_FILE);
		return -1;
	}
	///	解析nodes文件，获取到文件中的所有IP
	vector<string> nodes_ip;
	while(!feof(fp))
	{
		char line[1024] = {0};
		if (fgets(line,sizeof(line),fp) != NULL)
			nodes_ip.push_back(string(line));
	}
	fclose(fp);
	
	/// 读取/proc/net/dev文件，获取设备的所有网络接口
	FILE *fp_dev = fopen("/prov/net/dev","r");
	if (fp_dev == NULL)
	{
		syslog(LOG_ERR,"open /prov/net/dev failed");
		return -1;
	}
	vector<string> interface;
	while(!feof(fp_dev))			
	{
		char line[1024] = {0};
		if (fgets(line,sizeof(line),fp_dev) != NULL)
		{
			if (strstr(line,":") == NULL)
				continue;
			char *inter = strtok(line,":");
			if (strcmp(inter,"lo") == 0)
				continue;
			interface.push_back(inter);
		}
	}
	fclose(fp_dev);

	/// 根据网络接口获取每个接口对应的IP
	//	然后和nodes中得到的IP进行比对，
	vector<string>::iterator it;
	vector<string>::iterator it_nodes;
	for (it = interface.begin(); it != interface.end(); it++)
	{
		string dev_ip;
		if (ip_by_interface(*it,dev_ip) < 0)
			continue;
		for (it_nodes = nodes_ip.begin();
				it_nodes != nodes_ip.end(); it_nodes++)
		{
			if (dev_ip == *it_nodes)
			{
				ip = dev_ip;
				return 0;
			}
		}
	}
	return -1;
}

/*
 *	检测该行是否条件
 *	参数：
 *		line：该行字符串，ip：目的IP，port：目的端口
 *	返回值：
 *		成功返回true，失败返回false
 *
 * */
static bool check_line(const char *line,string ip,int port)
{
	
	if (line == NULL || 
			line[0] == '\0' ||
			ip.empty() || port <= 0)
		return false;
	
	char *pline = strdup(line);
	char *pfree = pline;				///	free指针
	char *pitem = NULL;					/// 临时指针

	pitem = strtok(pline," ");
	while(pitem != NULL)
	{
		///	先判断目的IP是否匹配，
		if (!strncmp(pitem,"dst=",strlen("dst=")))
		{
			if (string(pitem) != string("dst=") + ip)
				break;
		}
		/// 再判断目的端口是否匹配
		if (!strncmp(pitem,"dport=",strlen("dport=")))
		{
			char str_port[10] = {0};
			sprintf(str_port,"%d",port);
			if (string(pitem) == string("dport=") + string(str_port))
			{
				free(pfree);
				return true;
			}
			break;
		}
		pitem = strtok(NULL," ");
	}
	free(pfree);
	return false;
}

/*
 *	根据传入参数ip和port对文件进行解析过滤
 *	获取到每个虚拟IP对应的连接数
 *	参数：
 *		传入：ip，port
 *		传出：data
 *	成功返回0，失败返回-1
 * */
int parse_links_file(vector<string> &ip,vector<int> &nfs_port,
		vector<int> &cifs_port,vector<struct LinkNum> &data)
{
	if (ip.empty() || 
			(nfs_port.empty() && cifs_port.empty()))
		return -1;
	data.clear();

	FILE *fp = fopen(LINKS_FILE_PATH,"r");
	if (fp == NULL)
		return -1;
	
	vector<string>::iterator it_ip;
	vector<int>::iterator it_port;
	vector<struct LinkNum>::iterator it_data;

	/// 根据虚拟IP的个数初始化好LinkNum向量，以方便解析文件时统计连接数
	for (it_ip = ip.begin(); it_ip != ip.end(); it_ip++)
	{
		struct LinkNum t_data;
		memset(&t_data,0,sizeof(t_data));
		t_data.virip = *it_ip;
		data.push_back(t_data);
	}

	char line[1024] = {0};
	while(!feof(fp))
	{
		memset(line,0,sizeof(line));
		if(fgets(line,sizeof(line),fp) != NULL)
		{
			for (it_data = data.begin(); it_data != data.end(); it_data++)
			{
				/// 统计NFS的连接数
				for (it_port = nfs_port.begin(); it_port != nfs_port.end(); it_port++)
					if (check_line(line,(*it_data).virip,*it_port))				/// 检测该行是否符合目的地址是ip，目的端口是port
						(*it_data).nfs_links++;
				/// 统计CIFS的连接数
				for (it_port = cifs_port.begin(); it_port != cifs_port.end(); it_port++)
					if (check_line(line,(*it_data).virip,*it_port))				/// 检测该行是否符合目的地址是ip，目的端口是port
						(*it_data).cifs_links++;
			}
		}
	}
	fclose(fp);
	return 0;
}

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
 *	根据向量data中的连接数，填充pdu
 *	成功返回0，失败返回-1
 * */
int create_pdu(vector<struct LinkNum> &data)
{
	if (pdu == NULL || data.empty())
		return -1;
	
	char lengthbuf[1024];
	memset(lengthbuf,0,sizeof(lengthbuf));
	int total_length = data.size() * sizeof(struct LinkNum);
	sprintf(lengthbuf,"%d",total_length);
	snmp_add_var(pdu,objid_data_length,sizeof(objid_data_length)/sizeof(oid),'i',lengthbuf);

	vector<struct LinkNum>::iterator it;
	for( it = data.begin(); it != data.end(); it++)
	{
		char cifsbuf[64] = {0};
		char nfsbuf[64] = {0};
		sprintf(cifsbuf,"%d",it->cifs_links);
		sprintf(nfsbuf,"%d",it->nfs_links);
		snmp_add_var(pdu,objid_virtual_ip,sizeof(objid_virtual_ip)/sizeof(oid),'s',it->virip.c_str());
		snmp_add_var(pdu,objid_nfs_links,sizeof(objid_nfs_links)/sizeof(oid),'i',cifsbuf);
		snmp_add_var(pdu,objid_cifs_links,sizeof(objid_cifs_links)/sizeof(oid),'i',nfsbuf);
	}
	return 0;
}

