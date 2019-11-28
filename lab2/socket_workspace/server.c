
#include<stdio.h> 			/* perror */
#include<stdlib.h>			/* exit	*/
#include<sys/types.h>		/* WNOHANG */
#include<sys/wait.h>		/* waitpid */
#include<string.h>			/* memset */

#include "socketwrapper.h" 	/* socket layer wrapper */

#define	true		1
#define false		0

#define MYPORT 		3490                /* 监听的端口 */ 
#define BACKLOG 	10                 	/* listen的请求接收队列长度 */

int main() 
{
	int sockfd, new_fd;            /* 监听端口，数据端口 */
	struct sockaddr_in sa;         /* 自身的地址信息 */ 
	struct sockaddr_in their_addr; /* 连接对方的地址信息 */ 
	int sin_size;
	
	if ((sockfd = Socket(PF_INET, SOCK_STREAM, 0)) == -1) 
	{
		perror("socket"); 
		exit(1); 
	}
	
	sa.sin_family = AF_INET;
	sa.sin_port = Htons(MYPORT);         /* 网络字节顺序 */
	sa.sin_addr.s_addr = INADDR_ANY;     /* 自动填本机IP */
	memset(&(sa.sin_zero),0, 8);            /* 其余部分置0 */
	
	if ( Bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) == -1) 
	{
		perror("bind");
		exit(1);
	}
	
	if (Listen(sockfd, BACKLOG) == -1) 
	{
		perror("listen");
		exit(1);
	}
	
	/* 主循环 */
	while(1) 
	{
		sin_size = sizeof(struct sockaddr_in);
		new_fd = Accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) 
		{
			perror("accept");
			continue;
		}
		
		printf("Got connection from %s\n", Inet_ntoa(their_addr.sin_addr));
		if (fork() == 0) 
		{
			/* 子进程 */
			if (Send(new_fd, "Hello, world!\n", 14, 0) == -1)
			perror("send");
			Close(new_fd);
			exit(0);
		}
	
		Close(new_fd);
		
		/*清除所有子进程 */
		while(waitpid(-1,NULL,WNOHANG) > 0); 

	}
	return true;
}

