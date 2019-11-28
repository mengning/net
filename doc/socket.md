# 通过Socket编程接口学习计算机网络

## 动手写一个网络聊天程序

## Socket编程接口详解

Socket套接字是通信过程中两端的编程接口。使用套接字类型（Socket Types）和特定协议来创建套接字，创建套接字时获得的文件描述符是编程中访问特定套接字的依据。

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

## SOCK_STREAM及其他协议

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
 
如上几点对于我们使用socket套接口编程非常重要，乃至后续进一步理解和分析Linux网络协议栈代码也比较重要，代码中涉及的其他接口及参数可以在实验过程中自行查阅相关资料。
