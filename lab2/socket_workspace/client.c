
#include<stdio.h> 			/* perror */
#include<stdlib.h>			/* exit	*/
#include<sys/types.h>		/* WNOHANG */
#include<sys/wait.h>		/* waitpid */
#include<string.h>			/* memset */
#include "dbtime.h"
#include "socketwrapper.h" 	/* socket layer wrapper */

#define	true		1
#define false		0

#define PORT 		3490 	/* Server的端口 */
#define MAXDATASIZE 100 	/*一次可以读的最大字节数 */


int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct hostent *he; /* 主机信息 */
	struct sockaddr_in their_addr; /* 对方地址信息 */
	if (argc != 2) 
	{
		fprintf(stderr,"usage: client hostname\n");
		exit(1);
	}
	
	/* get the host info */
	if ((he=Gethostbyname(argv[1])) == NULL) 
	{
		/* 注意：获取DNS信息时，显示出错需要用herror而不是perror */
		/* herror 在新的版本中会出现警告，已经建议不要使用了 */
		perror("gethostbyname");
		exit(1);
	}
	
	if ((sockfd=Socket(PF_INET,SOCK_STREAM,0))==-1) 
	{
		perror("socket");
		exit(1);
	}
	
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = Htons(PORT); /* short, NBO */
	their_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
	memset(&(their_addr.sin_zero),0, 8); /* 其余部分设成0 */

	dbtime_startTest ("Connect & Recv");
	
	if (Connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) 
	{
		perror("connect");
		exit(1);
	}
	
	if ((numbytes=Recv(sockfd,buf,MAXDATASIZE,0))==-1) 
	{
		perror("recv");
		exit(1);
	}
	
	dbtime_endAndShow ();

	buf[numbytes] = '\0';
	printf("Received: %s",buf);
	Close(sockfd);
	
	dbtime_startTest ("Sleep 5s");
    sleep(5);
	dbtime_endAndShow ();
	dbtime_finalize ();
	return true;

}

