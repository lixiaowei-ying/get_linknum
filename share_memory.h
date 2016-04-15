#ifndef		__SHARE_MEMORY_H__
#define		__SHARE_MEMORY_H__

#include <semaphore.h>
#include <vector>
#include <string>

#define		SHM_NAME	"shm_conn"		/// 共享内存文件名
#define		SEM_NAME	"sem_conn"		/// 有名信号量的文件名

/*
 *	用来存储虚拟IP和对应的连接数的值
 * */
struct conn_data
{
	char ip[16];						/// 虚拟IP
	char protocol[10];					/// NAS协议
	int conn;							/// 连接数的值
};

/*
 *	提供一套操作共享内存的API
 * */
class Shm_mem
{
	public:
		Shm_mem();
		~Shm_mem();

	public:
		char *shmptr;				/// 共享内存首地址
		sem_t *sem;					/// 信号量

	public:
		int shm_init(void);				/// 初始化共享内存
		int shmopen(void);				/// 打开共享内存
		int shm_destory(void);			/// 销毁共享内存
		int shmclose(void);				/// 关闭共享内存

		int shm_write(std::vector<struct conn_data> &vet);
		int shm_read(std::vector<struct conn_data> &vet);	/// 根据传入的虚拟IP 返回该IP的连接数

		void shm_clear(void);			/// 清空内存区
};


#endif		//__SHARE_MEMORY_H__
