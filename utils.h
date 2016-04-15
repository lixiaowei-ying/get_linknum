#ifndef		__UTILS_H__
#define		__UTILS_H__

#include <vector>
#include <string>

#include "share_memory.h"

/*
 *	通过CTDB获取节点主机的虚拟IP
 *	返回值：
 *		成功返回0，失败返回-1
 * */
int get_virip(std::vector<std::string> &ip);


/*
 *	根据协议获取旗下的端口号
 *	参数：	protocol传入的协议字符串，
 *			port，存储该协议的多个端口号的值
 *	返回值：成功返回0，失败返回-1
 * */
int get_port_by_protocol(const char *protocol,std::vector<int> &port);


/*
 *	从XML配置文件中获取到时间间隔
 *	参数：
 *		interval 期望获取到的时间间隔字符串
 *	返回值：
 *		成功返回时间间隔，失败返回-1
 * */
int get_interval(const char *interval);

/*
 *	根据传入参数ip和port对文件进行解析过滤
 *	获取到每个虚拟IP对应的连接数
 *	参数：
 *		传入：ip，port
 *		传出：data
 *	成功返回0，失败返回-1
 * */
int parse_file(std::vector<std::string> &ip,std::vector<int> &port,
		const char *protocol,std::vector<struct conn_data> &data);

#endif 		//__UTILS_H__
