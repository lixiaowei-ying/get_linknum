#ifndef		__UTILS_H__
#define		__UTILS_H__

#include <vector>
#include <string>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

/*
 *	用于连接数收集和发送的数据结构
 * */
struct LinkNum{
	char virip[16];				/// 虚拟IP
	int cifs_links;				/// 协议CIFS的连接数
	int nfs_links;				/// 协议NFS的连接数
};

/*
 *	通过读取XML配置文件，将数据放入到次结构体中，
 *	方便连接数的收集和发送部分。
 * */
struct XmlData{
	int collect_interval;
	int send_interval;
	std::string dst_ip;
	int dst_port;
	std::string community;
	int version;
	std::vector<int> cifs_port;
	std::vector<int> nfs_port;
};


/*
 *	检测XML文件并解析文件，
 *	将解析到的数据放入结构体xmldata中
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int check_parse_xmlfile(void);
	
/*
 *	通过CTDB获取节点主机的虚拟IP
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int get_virip(std::vector<std::string> &ip);

/*
 *	通过CTDB获取节点的物理IP
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int get_device_ip(std::string &ip);

/*
 *	根据传入参数ip和port对文件进行解析过滤
 *	获取到每个虚拟IP对应的连接数
 *	参数：
 *		传入：ip，port
 *		传出：data
 *	成功返回0，失败返回-1
 * */
int parse_links_file(std::vector<std::string> &ip,
		std::vector<int> &nfs_port,std::vector<int> &cifs_port,
		std::vector<struct LinkNum> &data);


/*
 *	初始化snmp并填充pdu的初始部分
 *	成功返回0，失败返回-1
 * */
int init_snmp_pdu(std::string &dst_ip,int dst_port,
		std::string &community,std::string &device_ip);

/*
 *	根据向量data中的连接数，填充pdu
 *	成功返回0，失败返回-1
 * */
int create_pdu(std::vector<struct LinkNum> &data);

#endif 		//__UTILS_H__
