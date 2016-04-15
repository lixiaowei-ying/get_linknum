#include "share_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#define		VIRIP_NUM	1024
#define		SHM_SIZE	VIRIP_NUM * sizeof(struct conn_data)

using namespace std;

/*
 *	构造函数
 * */
Shm_mem::Shm_mem()
{
	shmptr = NULL;
	sem = NULL;
}

/*
 *	析构函数
 * */
Shm_mem::~Shm_mem()
{
	if (shmptr)
		munmap(shmptr,SHM_SIZE);			///	释放映射的区域
	shmptr = NULL;
}

/*
 *	初始化共享内存和信号量，
 *	同时将共享内存映射到内存中。
 *	成功返回0，失败返回-1
 * */

int Shm_mem::shm_init(void)
{
	int fd = shm_open(SHM_NAME,O_CREAT | O_RDWR,666);
	if (fd == -1)
		return -1;
	sem = sem_open(SEM_NAME,O_CREAT | O_RDWR,666,1);
	if (sem == SEM_FAILED)
	{
		close(fd);
		return -1;
	}

	ftruncate(fd,SHM_SIZE);
	shmptr = (char*)mmap(NULL,SHM_SIZE,PROT_READ | PROT_WRITE,
			MAP_SHARED,fd,0);
	if (shmptr == MAP_FAILED)
	{
		shmptr = NULL;
		close(fd);
		sem_close(sem);
		return -1;
	}
	return 0;
}

/*
 *	打开共享内存和信号量，
 *	成功返回0，失败返回-1
 * */
int Shm_mem::shmopen(void)
{
	int fd = shm_open(SHM_NAME,O_RDWR,666);
	if (fd == -1)
		return -1;
	sem = sem_open(SEM_NAME,O_RDWR);
	if (sem == SEM_FAILED)
	{
		close(fd);
		return -1;
	}

	shmptr = (char*)mmap(NULL,SHM_SIZE,PROT_READ | PROT_WRITE,
			MAP_SHARED,fd,0);
	if (shmptr == MAP_FAILED)
	{
		shmptr = NULL;
		close(fd);
		sem_close(sem);
		return -1;
	}
	return 0;
}

/*
 *	销毁共享内存文件
 * */
int Shm_mem::shm_destory(void)
{
	if (shmptr)
		munmap(shmptr,SHM_SIZE);
	if (sem)
		sem_close(sem);
	shmptr = NULL;
	sem = NULL;
	return shm_unlink(SHM_NAME);
}

/*
 *	将内存区域置0
 * */
void Shm_mem::shm_clear(void)
{
	if (shmptr)
		memset(shmptr,0,SHM_SIZE);
}

/*
 *	关闭映射区域，
 * */
int Shm_mem::shmclose(void)
{
	if (sem)
		sem_close(sem);
	if (shmptr)
	{
		shmptr = NULL;
		return munmap(shmptr,SHM_SIZE);
	}
}

/*
 *	写入函数，将向量中的数据写入到共享内存中
 *	成功返回0，失败返回-1
 * */
int Shm_mem::shm_write(vector<struct conn_data> &vet)
{
	if (NULL == shmptr)
		return -1;
	if (vet.empty())
		return -1;

	vector<struct conn_data>::iterator it;
	it = vet.begin();
	struct conn_data *buf = (struct conn_data*)shmptr;
	sem_wait(sem);
	for (int i = 0; it != vet.end() && i != VIRIP_NUM; it++,i++)
	{
		struct conn_data temp = *it;
		memcpy(buf+i,&temp,sizeof(struct conn_data));
	}
	sem_post(sem);
	return 0;
}

/*
 * 	读出函数，将内存中的数据一次性读出
 * 	根据传入的虚拟IP，返回该IP的连接数
 * 	成功返回0，失败返回-1
 * */
int Shm_mem::shm_read(vector<struct conn_data> &vet)
{
	if (shmptr == NULL)
		return -1;
	vet.clear();
	int i = 0;
	struct conn_data *temp = (struct conn_data*)shmptr;
	sem_wait(sem);
	for (;i < VIRIP_NUM; i++)
	{
		if ((temp+i)->ip[0] == '\0')
			break;
		vet.push_back(*(temp+i));
	}
	sem_post(sem);
	return 0;
}
