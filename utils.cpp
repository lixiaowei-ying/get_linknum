#include "utils.h"

#include <libxml/parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

using namespace std;

#define		FILE_PATH 		"/proc/net/nf_conntrack"

/*
 *	通过CTDB获取节点主机的虚拟IP
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int get_virip(vector<string> &ip)
{
	
}

/*
 *	通过CTDB获取节点的物理IP
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int get_device_ip(std::string &ip)
{

}

/*
 *	根据协议获取旗下的端口号
 *	参数：	protocol传入的协议字符串，
 *			port，存储该协议的多个端口号的值
 *	返回值：成功返回0，失败返回-1
 * */
int get_port_by_protocol(const char *protocol,vector<int> &port)
{
	xmlDocPtr 	doc;
	xmlNodePtr 	curnode;
	port.clear();													/// 清空向量
	doc = xmlReadFile(INIFILE_PATH,"GB2312",XML_PARSE_RECOVER);		/// 指定文件格式
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
	xmlNodePtr propnode = curnode;									///	属性节点
	
	while(curnode != NULL)
	{
		/// 处理Protocol子节点
		if (!xmlStrcmp(curnode->name,BAD_CAST "Protocol"))
		{
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
						/// 处理协议
						if (!strcmp((char*)attrkey,protocol))		/// 节点属性为CIFS
						{
							xmlNodePtr temp = curnode->xmlChildrenNode;			/// 指向CIFS的子节点
							while(temp != NULL)
							{
								if (!xmlStrcmp(temp->name,BAD_CAST "Port"))		/// 获取多个Port的值
								{
									xmlChar *childkey = xmlNodeGetContent(temp);
									if (childkey)
									{
										port.push_back(atoi((char*)childkey));		/// 放入向量中
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
		curnode = curnode->next;
	}
	xmlFreeDoc(doc);
	return 0;
}

/*
 *	从XML配置文件中获取到时间间隔
 *	参数：
 *		interval 期望获取到的时间间隔字符串
 *	返回值：
 *		成功返回时间间隔，失败返回-1
 * */
int get_interval(const char *interval)
{
	xmlDocPtr 	doc;
	xmlNodePtr 	curnode;
	doc = xmlReadFile(INIFILE_PATH,"GB2312",XML_PARSE_RECOVER);		/// 指定文件格式
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
	
	int ret = -1;
	curnode = curnode->xmlChildrenNode;								///	指向根节点的子节点
	xmlNodePtr propnode = curnode;									///	属性节点

	while(curnode != NULL)
	{
		/// 处理Interval子节点
		if (!xmlStrcmp(curnode->name,BAD_CAST "Interval"))
		{
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
						if (!strcmp((char*)attrkey,interval))	///	获取Collect的值
							ret = atoi((char *)key);
						xmlFree(key);
					}
					if (attrkey)
						xmlFree(attrkey);
				}
				attrptr = attrptr->next;
			}
		}
		curnode = curnode->next;
	}
	xmlFreeDoc(doc);
	return ret;
}

/*
 *	解析XML配置文件获取目的IP
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int get_dstip(string &ip)
{
	xmlDocPtr 	doc;
	xmlNodePtr 	curnode;
	doc = xmlReadFile(INIFILE_PATH,"GB2312",XML_PARSE_RECOVER);		/// 指定文件格式
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
	
	int ret = -1;
	curnode = curnode->xmlChildrenNode;								///	指向根节点的子节点

	while(curnode != NULL)
	{
		/// 处理Interval子节点
		if (!xmlStrcmp(curnode->name,BAD_CAST "Snmp"))
		{
			xmlNodePtr 	childnode = curnode->xmlChildrenNode;
			while(childnode != NULL)
			{
				if (!xmlStrcmp(childnode->name,BAD_CAST "Dst_ip"))
				{
					xmlChar *key = xmlNodeGetContent(childnode);						/// 获取节点的值
					if (key)
					{
						ip = string((char *)key);
						ret = 0;
						xmlFree(key);
						break;
					}
				}
				childnode = childnode->next;
			}
		}
		curnode = curnode->next;
	}
	xmlFreeDoc(doc);
	return ret;
}


/*
 *	解析XML配置文件获取目的端口
 *	返回值：
 *		成功返回端口号，失败返回-1
 * */
int get_dstport(void)
{
	xmlDocPtr 	doc;
	xmlNodePtr 	curnode;
	doc = xmlReadFile(INIFILE_PATH,"GB2312",XML_PARSE_RECOVER);		/// 指定文件格式
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
	
	int ret = -1;
	curnode = curnode->xmlChildrenNode;								///	指向根节点的子节点

	while(curnode != NULL)
	{
		/// 处理Interval子节点
		if (!xmlStrcmp(curnode->name,BAD_CAST "Snmp"))
		{
			xmlNodePtr 	childnode = curnode->xmlChildrenNode;
			while(childnode != NULL)
			{
				if (!xmlStrcmp(childnode->name,BAD_CAST "Dst_port"))
				{
					xmlChar *key = xmlNodeGetContent(childnode);						/// 获取节点的值
					if (key)
					{
						ret = atoi((char*)key);
						xmlFree(key);
						break;
					}
				}
				childnode = childnode->next;
			}
		}
		curnode = curnode->next;
	}
	xmlFreeDoc(doc);
	return ret;
}


/*
 *	解析XML配置文件获取snmp的版本号
 *	返回值：
 *		成功返回版本号，失败返回-1
 * */
int get_version(void)
{
	xmlDocPtr 	doc;
	xmlNodePtr 	curnode;
	doc = xmlReadFile(INIFILE_PATH,"GB2312",XML_PARSE_RECOVER);		/// 指定文件格式
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
	
	int ret = -1;
	curnode = curnode->xmlChildrenNode;								///	指向根节点的子节点

	while(curnode != NULL)
	{
		/// 处理Interval子节点
		if (!xmlStrcmp(curnode->name,BAD_CAST "Snmp"))
		{
			xmlNodePtr 	childnode = curnode->xmlChildrenNode;
			while(childnode != NULL)
			{
				if (!xmlStrcmp(childnode->name,BAD_CAST "Version"))
				{
					xmlChar *key = xmlNodeGetContent(childnode);						/// 获取节点的值
					if (key)
					{
						ret = atoi((char*)key);
						xmlFree(key);
						break;
					}
				}
				childnode = childnode->next;
			}
		}
		curnode = curnode->next;
	}
	xmlFreeDoc(doc);
	return ret;
}

/*
 *	解析XML配置文件获取snmp的通讯密码
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int get_community(string &community)
{
	xmlDocPtr 	doc;
	xmlNodePtr 	curnode;
	doc = xmlReadFile(INIFILE_PATH,"GB2312",XML_PARSE_RECOVER);		/// 指定文件格式
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
	
	int ret = -1;
	curnode = curnode->xmlChildrenNode;								///	指向根节点的子节点

	while(curnode != NULL)
	{
		/// 处理Interval子节点
		if (!xmlStrcmp(curnode->name,BAD_CAST "Snmp"))
		{
			xmlNodePtr 	childnode = curnode->xmlChildrenNode;
			while(childnode != NULL)
			{
				if (!xmlStrcmp(childnode->name,BAD_CAST "Community"))
				{
					xmlChar *key = xmlNodeGetContent(childnode);						/// 获取节点的值
					if (key)
					{
						community = string((char*)key);
						xmlFree(key);
						ret = 0;
						break;
					}
				}
				childnode = childnode->next;
			}
		}
		curnode = curnode->next;
	}
	xmlFreeDoc(doc);
	return ret;
}

/*
 *	检测该行是否条件
 *	参数：
 *		line：该行字符串，ip：目的IP，port：目的端口
 *	返回值：
 *		成功返回true，失败返回false
 *
 * */
static bool check_line(const char *line,const char *ip,int port)
{
	
	if (line == NULL || 
			line[0] == '\0' ||
			ip[0] == '\0' || port <= 0)
		return false;
	
	char *pline = strdup(line);
	char *pfree = pline;
	char *pitem = NULL;

	pitem = strtok(pline," ");
	while(pitem != NULL)
	{
		if (!strncmp(pitem,"dst=",strlen("dst=")))
		{
			if (string(pitem) != string("dst=") + string(ip))
				break;
		}
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
int parse_file(vector<string> &ip,vector<int> &nfs_port,
		vector<int> &cifs_port,vector<struct conn_data> &data)
{
	if (ip.empty() || nfs_port.empty() 
			|| cifs_port.empty())
		return -1;
	data.clear();

	FILE *fp = fopen(FILE_PATH,"r");
	if (fp == NULL)
		return -1;
	
	vector<string>::iterator it_ip;
	vector<int>::iterator it_port;
	vector<struct conn_data>::iterator it_data;

	/// 根据虚拟IP的个数初始化好conn_data向量，以方便解析文件时统计连接数
	for (it_ip = ip.begin(); it_ip != ip.end(); it_ip++)
	{
		struct conn_data t_data;
		memset(&t_data,0,sizeof(t_data));
		strcpy(t_data.ip,(*it_ip).c_str());
		data.push_back(t_data);
	}

	char line[1024] = {0};
	while(!feof(fp))
	{
		if(fgets(line,sizeof(line),fp) != NULL)
		{
			for (it_data = data.begin(); it_data != data.end(); it_data++)
			{
				/// 统计NFS的连接数
				for (it_port = nfs_port.begin(); it_port != nfs_port.end(); it_port++)
					if (check_line(line,(*it_data).ip,*it_port))				/// 检测该行是否符合目的地址是ip，目的端口是port
						(*it_data).nfs_conn++;
				/// 统计CIFS的连接数
				for (it_port = cifs_port.begin(); it_port != cifs_port.end(); it_port++)
					if (check_line(line,(*it_data).ip,*it_port))				/// 检测该行是否符合目的地址是ip，目的端口是port
						(*it_data).cifs_conn++;
			}
		}
	}
	return 0;
}
