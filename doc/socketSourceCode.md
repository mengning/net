# Socket接口对应的Linux内核系统调用处理代码分析

理解Linux内核中socket接口层的代码，找出112号系统调用socketcall的内核处理函数sys_socketcall，理解socket接口函数编号和对应的socket接口内核处理函数
通过前面构建MenuOS实验环境使得我们有方法跟踪socket接口通过系统调用进入内核代码，在我们的环境中socket接口通过[112号系统调用socketcall](http://codelab.shiyanlou.com/xref/linux-3.18.6/arch/x86/syscalls/syscall_32.tbl#111)进入内核的，具体系统调用的处理机制不是本专栏的重点，本专栏将重点放在网络部分的代码分析。

# 112号系统调用socketcall的内核处理函数sys_socketcall


112号系统调用socketcall的内核处理函数为sys_socketcall，函数实现见[/linux-3.18.6/net/socket.c#2492](http://codelab.shiyanlou.com/xref/linux-3.18.6/net/socket.c#2492) ，我们摘录部分代码如下：

```
/*
2485 *	System call vectors.
2486 *
2487 *	Argument checking cleaned up. Saved 20% in size.
2488 *  This function doesn't need to set the kernel lock because
2489 *  it is set by the callees.
2490 */
2491
2492SYSCALL_DEFINE2(socketcall, int, call, unsigned long __user *, args)
2493{
...
2517	switch (call) {
2518	case SYS_SOCKET:
2519		err = sys_socket(a0, a1, a[2]);
2520		break;
2521	case SYS_BIND:
2522		err = sys_bind(a0, (struct sockaddr __user *)a1, a[2]);
2523		break;
2524	case SYS_CONNECT:
2525		err = sys_connect(a0, (struct sockaddr __user *)a1, a[2]);
2526		break;
2527	case SYS_LISTEN:
2528		err = sys_listen(a0, a1);
2529		break;
2530	case SYS_ACCEPT:
2531		err = sys_accept4(a0, (struct sockaddr __user *)a1,
2532				  (int __user *)a[2], 0);
2533		break;
2534	case SYS_GETSOCKNAME:
2535		err =
2536		    sys_getsockname(a0, (struct sockaddr __user *)a1,
2537				    (int __user *)a[2]);
2538		break;
2539	case SYS_GETPEERNAME:
2540		err =
2541		    sys_getpeername(a0, (struct sockaddr __user *)a1,
2542				    (int __user *)a[2]);
2543		break;
2544	case SYS_SOCKETPAIR:
2545		err = sys_socketpair(a0, a1, a[2], (int __user *)a[3]);
2546		break;
2547	case SYS_SEND:
2548		err = sys_send(a0, (void __user *)a1, a[2], a[3]);
2549		break;
2550	case SYS_SENDTO:
2551		err = sys_sendto(a0, (void __user *)a1, a[2], a[3],
2552				 (struct sockaddr __user *)a[4], a[5]);
2553		break;
2554	case SYS_RECV:
2555		err = sys_recv(a0, (void __user *)a1, a[2], a[3]);
2556		break;
2557	case SYS_RECVFROM:
2558		err = sys_recvfrom(a0, (void __user *)a1, a[2], a[3],
2559				   (struct sockaddr __user *)a[4],
2560				   (int __user *)a[5]);
2561		break;
2562	case SYS_SHUTDOWN:
2563		err = sys_shutdown(a0, a1);
2564		break;
2565	case SYS_SETSOCKOPT:
2566		err = sys_setsockopt(a0, a1, a[2], (char __user *)a[3], a[4]);
2567		break;
2568	case SYS_GETSOCKOPT:
2569		err =
2570		    sys_getsockopt(a0, a1, a[2], (char __user *)a[3],
2571				   (int __user *)a[4]);
2572		break;
2573	case SYS_SENDMSG:
2574		err = sys_sendmsg(a0, (struct msghdr __user *)a1, a[2]);
2575		break;
2576	case SYS_SENDMMSG:
2577		err = sys_sendmmsg(a0, (struct mmsghdr __user *)a1, a[2], a[3]);
2578		break;
2579	case SYS_RECVMSG:
2580		err = sys_recvmsg(a0, (struct msghdr __user *)a1, a[2]);
2581		break;
2582	case SYS_RECVMMSG:
2583		err = sys_recvmmsg(a0, (struct mmsghdr __user *)a1, a[2], a[3],
2584				   (struct timespec __user *)a[4]);
2585		break;
2586	case SYS_ACCEPT4:
2587		err = sys_accept4(a0, (struct sockaddr __user *)a1,
2588				  (int __user *)a[2], a[3]);
2589		break;
2590	default:
2591		err = -EINVAL;
2592		break;
2593	}
2594	return err;
2595}
2596
```

在我们的实验环境中，socket接口的调用是通过给socket接口函数编号的方式通过112号系统调用来处理的。这些socket接口函数编号的宏定义见[/linux-3.18.6/include/uapi/linux/net.h#26](http://codelab.shiyanlou.com/xref/linux-3.18.6/include/uapi/linux/net.h#26)

```
26#define SYS_SOCKET	1		/* sys_socket(2)		*/
27#define SYS_BIND	2		/* sys_bind(2)			*/
28#define SYS_CONNECT	3		/* sys_connect(2)		*/
29#define SYS_LISTEN	4		/* sys_listen(2)		*/
30#define SYS_ACCEPT	5		/* sys_accept(2)		*/
31#define SYS_GETSOCKNAME	6		/* sys_getsockname(2)		*/
32#define SYS_GETPEERNAME	7		/* sys_getpeername(2)		*/
33#define SYS_SOCKETPAIR	8		/* sys_socketpair(2)		*/
34#define SYS_SEND	9		/* sys_send(2)			*/
35#define SYS_RECV	10		/* sys_recv(2)			*/
36#define SYS_SENDTO	11		/* sys_sendto(2)		*/
37#define SYS_RECVFROM	12		/* sys_recvfrom(2)		*/
38#define SYS_SHUTDOWN	13		/* sys_shutdown(2)		*/
39#define SYS_SETSOCKOPT	14		/* sys_setsockopt(2)		*/
40#define SYS_GETSOCKOPT	15		/* sys_getsockopt(2)		*/
41#define SYS_SENDMSG	16		/* sys_sendmsg(2)		*/
42#define SYS_RECVMSG	17		/* sys_recvmsg(2)		*/
43#define SYS_ACCEPT4	18		/* sys_accept4(2)		*/
44#define SYS_RECVMMSG	19		/* sys_recvmmsg(2)		*/
45#define SYS_SENDMMSG	20		/* sys_sendmmsg(2)		*/
```

接下来我们根据TCP server程序调用socket接口的顺序依次看一下socket、bind、listen、accept等socket接口的内核处理函数。

# socket接口函数的内核处理函数sys_socket

sys_socket内核处理函数见[/linux-3.18.6/net/socket.c#1377](http://codelab.shiyanlou.com/xref//linux-3.18.6/net/socket.c#1377) ，摘录其中的关键代码如下：

```
1377SYSCALL_DEFINE3(socket, int, family, int, type, int, protocol)
1378{
1379	int retval;
1380	struct socket *sock;
...
1397	retval = sock_create(family, type, protocol, &sock);
...
```

socket接口函数主要作用是建立socket套接字描述符，Unix-like系统非常成功的设计是将一切都抽象为文件，socket套接字也是一种特殊的文件，sock_create内部就是使用文件系统中的数据结构inode为socket套接字分配了文件描述符。socket套接字与普通的文件在内部存储结构上是一致的，甚至文件描述符和套接字描述符是通用的，但是套接字和文件还是特殊之处，因此定义了结构体struct socket，struct socket的结构体定义见[/linux-3.18.6/include/linux/net.h#105](http://codelab.shiyanlou.com/xref/linux-3.18.6/include/linux/net.h#105)，具体代码摘录如下：

```
95/**
96 *  struct socket - general BSD socket
97 *  @state: socket state (%SS_CONNECTED, etc)
98 *  @type: socket type (%SOCK_STREAM, etc)
99 *  @flags: socket flags (%SOCK_ASYNC_NOSPACE, etc)
100 *  @ops: protocol specific socket operations
101 *  @file: File back pointer for gc
102 *  @sk: internal networking protocol agnostic socket representation
103 *  @wq: wait queue for several uses
104 */
105struct socket {
106	socket_state		state;
107
108	kmemcheck_bitfield_begin(type);
109	short			type;
110	kmemcheck_bitfield_end(type);
111
112	unsigned long		flags;
113
114	struct socket_wq __rcu	*wq;
115
116	struct file		*file;
117	struct sock		*sk;
118	const struct proto_ops	*ops;
119};
```

sock_create内部还根据指定的网络协议族family和protocol初始化了相关协议的处理接口到结构体struct socket中，结构体struct socket在后续的分析和理解中还会用到，这里简单略过用到时再具体研究。

# bind接口函数的内核处理函数sys_bind

内核处理函数sys_bind见[/linux-3.18.6/net/socket.c#1527](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/socket.c#1527)，它的功能是绑定网络地址。

```
1519/*
1520 *	Bind a name to a socket. Nothing much to do here since it's
1521 *	the protocol's responsibility to handle the local address.
1522 *
1523 *	We move the socket address to kernel space before we call
1524 *	the protocol layer (having also checked the address is ok).
1525 */
1526
1527SYSCALL_DEFINE3(bind, int, fd, struct sockaddr __user *, umyaddr, int, addrlen)
1528{
1529	struct socket *sock;
1530	struct sockaddr_storage address;
1531	int err, fput_needed;
1532
1533	sock = sockfd_lookup_light(fd, &err, &fput_needed);
1534	if (sock) {
1535		err = move_addr_to_kernel(umyaddr, addrlen, &address);
1536		if (err >= 0) {
1537			err = security_socket_bind(sock,
1538						   (struct sockaddr *)&address,
1539						   addrlen);
1540			if (!err)
1541				err = sock->ops->bind(sock,
1542						      (struct sockaddr *)
1543						      &address, addrlen);
1544		}
1545		fput_light(sock->file, fput_needed);
1546	}
1547	return err;
1548}
```

如上代码可以看到，move_addr_to_kernel将用户态的struct sockaddr结构体数据拷贝到内核里的结构体变量struct sockaddr_storage address，然后使用sock->ops->bind将该网络地址绑定到之前创建的套接字。这里用到了通过套接字描述符fd找到之前分配的套接字struct socket *sock，利用该套接字中的成员const struct proto_ops    *ops找到对应网络协议的bind函数指针即sock->ops->bind。这里即是一个socket接口层通往具体协议处理的接口。

# listen接口函数的内核处理函数sys_listen

内核处理函数sys_listen见[/linux-3.18.6/net/socket.c#1556](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/socket.c#1556)，具体代码如下：

```
1550/*
1551 *	Perform a listen. Basically, we allow the protocol to do anything
1552 *	necessary for a listen, and if that works, we mark the socket as
1553 *	ready for listening.
1554 */
1555
1556SYSCALL_DEFINE2(listen, int, fd, int, backlog)
1557{
1558	struct socket *sock;
1559	int err, fput_needed;
1560	int somaxconn;
1561
1562	sock = sockfd_lookup_light(fd, &err, &fput_needed);
1563	if (sock) {
1564		somaxconn = sock_net(sock->sk)->core.sysctl_somaxconn;
1565		if ((unsigned int)backlog > somaxconn)
1566			backlog = somaxconn;
1567
1568		err = security_socket_listen(sock, backlog);
1569		if (!err)
1570			err = sock->ops->listen(sock, backlog);
1571
1572		fput_light(sock->file, fput_needed);
1573	}
1574	return err;
1575}
```

listen接口的主要作用是通知网络底层开始监听套接字并接收网络连接请求，listen接口正常处理完TCP服务就已经启动了，只是这时网络连接请求都会暂存在缓冲区，等调用accept建立连接，listen接口函数的参数backlog就是用来配置支持的连接数。

我们发现实际处理的工作是由sock->ops->listen完成的，这也是一个socket接口层通往具体协议处理的接口。

# accept接口函数的内核处理函数sys_accept

内核处理函数sys_accept的主要功能是调用sys_accept4完成的，sys_accept4见[/linux-3.18.6/net/socket.c#1589](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/socket.c#1589)，具体代码摘录如下：

```
1577/*
1578 *	For accept, we attempt to create a new socket, set up the link
1579 *	with the client, wake up the client, then return the new
1580 *	connected fd. We collect the address of the connector in kernel
1581 *	space and move it to user at the very end. This is unclean because
1582 *	we open the socket then return an error.
...
1589SYSCALL_DEFINE4(accept4, int, fd, struct sockaddr __user *, upeer_sockaddr,
1590		int __user *, upeer_addrlen, int, flags)
1591{
...
1608	newsock = sock_alloc();
...
1612	newsock->type = sock->type;
1613	newsock->ops = sock->ops;
...
1621	newfd = get_unused_fd_flags(flags);
...
1627	newfile = sock_alloc_file(newsock, flags, sock->sk->sk_prot_creator->name);
...
1639	err = sock->ops->accept(sock, newsock, sock->file->f_flags);
...
1643	if (upeer_sockaddr) {
1644		if (newsock->ops->getname(newsock, (struct sockaddr *)&address,
...
1649		err = move_addr_to_user(&address,
...
1657	fd_install(newfd, newfile);
1658	err = newfd;
...
1668}
```

在TCP的服务器端通过socket函数创建的套接字描述符只是用来监听客户连接请求，accept函数内部会为每一个请求连接的客户创建一个新的套接字描述符专门负责与该客户端进行网络通信，并将该客户的网络地址和端口等地址信息返回到用户态。这里涉及更多的网络协议处理的接口如sock->ops->accept、ewsock->ops->getname。

send和recv接口的内核处理函数类似也是通过调用网络协议处理的接口来将具体的工作交给协议层来完成，比如sys_recv最终调用了sock->ops->recvmsg，sys_send最终调用了sock->ops->sendmsg，但send和recv接口涉及网络数据流，是理解网络部分的关键内容，我们后续部分专门具体研究。
