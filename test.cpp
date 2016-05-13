#include "ctdb_ip.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int f(char *ip[])
{
	printf("%s\n",ip[0]);
	printf("%s\n",ip[0]);
	return 0;
}

int main(int argc,char **argv)
{
	char devip[] = "100.2.84.106";
	char virip[100][16];
	memset(virip,0,100*16);
	int ret = virip_by_devip(argv[1],virip);
	printf("devip:%s  num:%d\n",argv[1],ret);
	for (int i = 0; i < 3; i++)
		printf("        %s\n",virip[i]);
	
	devip_by_virip("100.2.83.222",devip);
	printf("virip:%s   devip:%s\n","100.2.83.222",devip);
	return 0;
}
