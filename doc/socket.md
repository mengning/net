# 通过Socket编程接口学习计算机网络

## 动手写第一个网络程序

```
/* client.c */
#include <stdio.h> 		/* perror */
#include <stdlib.h>		/* exit	*/
#include <sys/types.h>		/* WNOHANG */
#include <sys/wait.h>		/* waitpid */
#include <string.h>		/* memset */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>		/* gethostbyname */

#define	true		1
#define false		0

#define PORT 		3490 	/* Server的端口 */
#define MAXDATASIZE 	100 	/* 一次可以读的最大字节数 */


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
	if ((he=gethostbyname(argv[1])) == NULL) 
	{
		/* 注意：获取DNS信息时，显示出错需要用herror而不是perror */
		/* herror 在新的版本中会出现警告，已经建议不要使用了 */
		perror("gethostbyname");
		exit(1);
	}
	
	if ((sockfd=socket(PF_INET,SOCK_STREAM,0)) == -1) 
	{
		perror("socket");
		exit(1);
	}
	
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(PORT); /* short, NBO */
	their_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
	memset(&(their_addr.sin_zero),0, 8); /* 其余部分设成0 */

	if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) 
	{
		perror("connect");
		exit(1);
	}
	
	if ((numbytes = recv(sockfd,buf,MAXDATASIZE,0)) == -1) 
	{
		perror("recv");
		exit(1);
	}

	buf[numbytes] = '\0';
	printf("Received: %s",buf);
	close(sockfd);
	
	return true;
}
```
```

/* server.c */
#include <stdio.h> 		/* perror */
#include <stdlib.h>		/* exit	*/
#include <sys/types.h>		/* WNOHANG */
#include <sys/wait.h>		/* waitpid */
#include <string.h>		/* memset */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>		/* gethostbyname */

#define	true		1
#define false		0

#define MYPORT 		3490    /* 监听的端口 */ 
#define BACKLOG 	10      /* listen的请求接收队列长度 */

int main() 
{
	int sockfd, new_fd;            /* 监听端口，数据端口 */
	struct sockaddr_in sa;         /* 自身的地址信息 */ 
	struct sockaddr_in their_addr; /* 连接对方的地址信息 */ 
	int sin_size;
	
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) 
	{
		perror("socket"); 
		exit(1); 
	}
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons(MYPORT);         /* 网络字节顺序 */
	sa.sin_addr.s_addr = INADDR_ANY;     /* 自动填本机IP */
	memset(&(sa.sin_zero),0, 8);         /* 其余部分置0 */
	
	if ( bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) == -1) 
	{
		perror("bind");
		exit(1);
	}
	
	if (listen(sockfd, BACKLOG) == -1) 
	{
		perror("listen");
		exit(1);
	}
	
	/* 主循环 */
	while(1) 
	{
		sin_size = sizeof(struct sockaddr_in);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) 
		{
			perror("accept");
			continue;
		}
		
		printf("Got connection from %s\n", inet_ntoa(their_addr.sin_addr));
		if (fork() == 0) 
		{
			/* 子进程 */
			if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
			perror("send");
			close(new_fd);
			exit(0);
		}
	
		close(new_fd);
		
		/*清除所有子进程 */
		while(waitpid(-1,NULL,WNOHANG) > 0); 
	}
	close(sockfd);
	return true;
}
```

```
gcc client.c -o client
gcc server.c -o server
```
## Socket编程接口详解

Socket套接字是通信过程中两端的编程接口。使用套接字类型（Socket Types）和特定协议来创建套接字，创建套接字时获得的文件描述符是编程中访问特定套接字的依据。除了套接字类型和协议族，还有网络地址的存储结构也非常重要，在进一步学习Socket编程接口之前，我们先来具体看看网络地址的存储结构、协议族和地址族、以及套接字类型。

## sockaddr和sockaddr_in的不同作用

一般在linux环境下/usr/include/bits/socket.h或/usr/include/sys/socket.h可以看到sockaddr的结构体声明。

```
/* Structure describing a generic socket address.  */
struct sockaddr
  {
    __SOCKADDR_COMMON (sa_);	/* Common data: address family and length.  */
    char sa_data[14];		/* Address data.  */
  };
```

这是一个通用的socket地址可以兼容不同的协议，当然包括基于TCP/IP的互联网协议，为了方便起见互联网socket地址的结构提供定义的更具体见/usr/include/netinet/in.h文件中的struct sockaddr_in。

```
/* Structure describing an Internet socket address.  */
struct sockaddr_in
  {
    __SOCKADDR_COMMON (sin_);
    in_port_t sin_port;			/* Port number.  */
    struct in_addr sin_addr;		/* Internet address.  */

    /* Pad to size of `struct sockaddr'.  */
    unsigned char sin_zero[sizeof (struct sockaddr) -
			   __SOCKADDR_COMMON_SIZE -
			   sizeof (in_port_t) -
			   sizeof (struct in_addr)];
  };
```

sockaddr和sockaddr_in的关系有点像面向对象编程中的父类和子类，子类重新定义了父类的地址数据格式。同一块数据我们根据需要使用两个不同的结构体变量来存取数据内容，这也是最简单的面向对象编程中的继承特性的实现方法。

## AF_INET和PF_INET

在/usr/include/bits/socket.h或/usr/include/sys/socket.h中一般可以找到AF_INET和PF_INET的宏定义如下。

```
/* Protocol families.  */
...
#define	PF_INET		2	/* IP protocol family.  */
...
/* Address families.  */
...
#define	AF_INET		PF_INET
...
```

尽管他们的值相同，但它们的含义是不同的，网上很多代码将AF_INET和PF_INET混用，如果您了解他们的含义就不会随便混用了，根据如下注释可以看到A代表Address families，P代表Protocol families，也就是说当表示地址时用AF_INET，表示协议时用PF_INET。我们一般写代码时给地址结构体变量赋值如“serveraddr.sin_family = AF_INET;”中使用AF_INET，而创建套接口时如“sockfd = socket(PF_INET,SOCK_STREAM,0);”中使用PF_INET。

## SOCK_STREAM及其他套接字类型

在/usr/include/bits/socket_type.h可以找到“__socket_type”，不同协议族一般都会定义不同类型的通信方式，对于基于TCP/IP的互联网协议族（即PF_INET），面向连接的TCP协议的socket类型即为SOCK_STREAM，无连接的UDP协议即为SOCK_DGRAM，而SOCK_RAW 工作在网络层。SOCK_RAW 可以处理ICMP、IGMP等网络报文、特殊的IPv4报文等。

```
/* Types of sockets.  */
enum __socket_type
{
  SOCK_STREAM = 1,		/* Sequenced, reliable, connection-based
				   byte streams.  */
#define SOCK_STREAM SOCK_STREAM
  SOCK_DGRAM = 2,		/* Connectionless, unreliable datagrams
				   of fixed maximum length.  */
#define SOCK_DGRAM SOCK_DGRAM
  SOCK_RAW = 3,			/* Raw protocol interface.  */
#define SOCK_RAW SOCK_RAW
  SOCK_RDM = 4,			/* Reliably-delivered messages.  */
#define SOCK_RDM SOCK_RDM
  SOCK_SEQPACKET = 5,		/* Sequenced, reliable, connection-based,
				   datagrams of fixed maximum length.  */
...			 
 ```
 
如上几点对于我们使用socket套接口编程非常重要，乃至后续进一步理解和分析Linux网络协议栈代码也比较重要，接下来我们继续看具体的socket套接口API。

## 创建一个新的套接字

创建一个新的套接字，返回套接字描述符。实际上就是告诉操作系统内核协议栈，我准备使用什么协议族、什么套接字类型、以及哪个协议，内核协议栈帮你分配一个套接字描述符用于后续操作。从程序员的角度看，就是通过socket函数创建一个通信协议实例对象，获得一个操作这个通信协议实例对象的句柄。对于Linux系统来说，套接字描述符使用的是文件描述符，换句话说套接字描述符是一个特殊的文件描述符。按照Unix类系统的设计理念————“一些皆文件”，I/O设备都是以设备文件的形式存在于系统中，网络设备也就是一个特殊的文件，这样也比较好理解。
```
int socket( int domain, int type, int protocol);
```
socket函数原型中有三个参数：
* domain：域类型，指明使用的协议栈，如TCP/IP使用的是 PF_INET	
* type: 指明需要的服务类型, 如SOCK_DGRAM:数据报服务，UDP协议SOCK_STREAM: 流服务，TCP协议
* protocol:一般都取0

socket函数应用举例：
```
int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
```
