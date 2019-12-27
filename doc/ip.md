# 敢问路在何方？—— IP协议和路由表

IP协议和路由表是整个互联网架构的核心基础设施，从本文开始我们将深入理解互联网架构的核心，其中包括IP协议和路由表是核心中的核心，本文将从IP地址及无类别区间路由CIDR谈起，先从原理上理解选路路由的方法，然后再阅读Linux内核中IP协议相关的代码，原理和代码相互印证，感兴趣的同学还可以基于我们的MenuOS进一步跟踪分析路由表查询相关的代码。

## IP协议及IP封包格式

协议本质上就是通讯的双方共同遵守的规则，对于IP协议来说最关键的规则就是IP的封包格式

![](https://s1.51cto.com/images/blog/201901/08/9840b1b6ce8d52ce0b1510e34b525fc8.png)

其中源IP地址和目的IP地址是最关键的信息。

## IP地址及无类别区间路由CIDR

IP地址最初是分类别的，如A类、B类、C类等，IPv4地址长度32位，对应的A类网络的网络号占一个字节，B类网络的网络号占2个字节，C类网络的网络号占3个字节。随着互联网的发展32位的IP地址资源明星不足，而获得A类网络的组织却很可能有大量地址资源没有得到充分利用，而获得C类网络的组织随着规模的扩大却又申请了多个C类网络，因此按类别划分IP网络地址的方案逐渐暴露出IP地址资源利用率不高、灵活性差的问题。

解决IPv4地址利用率不高的问题，诞生了多种方案比如NAT和NAPT，通过网络地址转换将私有网络地址转换为公网地址，大大提高了IP地址的利用效率，这种方案在地址资源较为匮乏的国家和地区应用非常广泛，比如中国。

无类别区间路由CIDR是其中最重要的一个方案，它既可以有效提交IPv4地址的利用率，又能灵活低拆分和汇聚地址块，大大减少路由表条目，提高了整个网络的运作效率。CIDR是怎么做到这一点的呢？

所谓无类别区间路由CIDR，无类别就是打破最初A类、B类、C类等按类别划分网络的地址的方法，使用掩码Mast来指明网络号，比如原来的A网络我们可以使用255.0.0.0的掩码来提取出网络号，另外一种表现形式是这样一个A类网络的IP地址x.x.x.x/8可以通过后缀/8来指明网络的比特长度。

这么做的好处是显而易见的，通过增加网络号的长度可以灵活地划分子网，同时路由表可以通过缩短网络号合并其中包含的子网络，减少路由表的项目数量，提高路由查询速度。

## 路由表查询的方法

如图网络结构中R1路由器有四个接口分别连接了四个网络，其中一个m2接口连接的网络中有一台路由器连接到互联网上。

![](https://s1.51cto.com/images/blog/201901/08/58f8a4d12a50b72a6c2236140f6611b4.png)

R1路由器的路由表如图所示：

![](https://s1.51cto.com/images/blog/201901/08/d45f75b01a717c4125f0700e1f974c51.png)

当R1路由器接到一个IP包时会解析出IP包中的目的IP地址，用掩码Mast提取出目的IP地址的网络地址作为输入参数查询路由表，匹配路由表项的网络地址，匹配成功获得下一跳next-hop的IP地址及网络接口号，如下图所示。

![](https://s1.51cto.com/images/blog/201901/08/218ed33cd03b1420721710ca52887c9d.png)

路由表查询结果是下一跳next-hop的IP地址及网络接口号，通过下一跳next-hop的IP地址及网络接口号可以进行ARP解析获取对应的MAC地址，这一部分是理解网络架构中网络层与链路层相互协作的关键，我们将在下一篇文章中专题介绍。

## 在Linux系统使用route命令可以查看路由表信息

```
shiyanlou:~/ $ route
Kernel IP routing table
Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
default         192.168.40.1    0.0.0.0         UG    0      0        0 eth0
192.168.40.0    *               255.255.255.0   U     0      0        0 eth0
shiyanlou:~/ $ 
```

有了前面的相关背景知识的铺垫接下来就可以看代码了。

## IP协议的初始化

[IP协议的初始化函数ip_init](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/af_inet.c#1740)与TCP一样也是在inet_init函数中被调用的，如/linux-3.18.6/net/ipv4/af_inet.c中1740行代码处。
```
1674static int __init inet_init(void)
1675{
...
1730	/*
1731	 *	Set the ARP module up
1732	 */
1733
1734	arp_init();
1735
1736	/*
1737	 *	Set the IP module up
1738	 */
1739
1740	ip_init();
1741
1742	tcp_v4_init();
1743
1744	/* Setup TCP slab cache for open requests. */
1745	tcp_init();
1746
1747	/* Setup UDP memory threshold */
1748	udp_init();
1749
1750	/* Add UDP-Lite (RFC 3828) */
1751	udplite4_register();
1752
1753	ping_init();
1754
1755	/*
1756	 *	Set the ICMP layer up
1757	 */
1758
1759	if (icmp_init() < 0)
1760		panic("Failed to create the ICMP control socket.\n");
1761
1762	/*
1763	 *	Initialise the multicast router
1764	 */
1765#if defined(CONFIG_IP_MROUTE)
1766	if (ip_mr_init())
1767		pr_crit("%s: Cannot init ipv4 mroute\n", __func__);
1768#endif
1769
1770	if (init_inet_pernet_ops())
1771		pr_crit("%s: Cannot init ipv4 inet pernet ops\n", __func__);
1772	/*
1773	 *	Initialise per-cpu ipv4 mibs
1774	 */
1775
1776	if (init_ipv4_mibs())
1777		pr_crit("%s: Cannot init ipv4 mibs\n", __func__);
1778
1779	ipv4_proc_init();
1780
1781	ipfrag_init();
1782
1783	dev_add_pack(&ip_packet_type);
...
1795}
1796
1797fs_initcall(inet_init);
```
路由表的结构和初始化过程，ip协议初始化ip_init过程中包含路由表的初始化ip_rt_init/ip_fib_init，主要代码见route.c及fib*.c。[ip_init函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_output.c#1602)主要做了三方面工作：
* ip_rt_init() 初始化路由缓存,通过哈希结构提供快速获取目的IP地址的下一跳（Next Hop）访问, 以及初始化作为路由表内部表示形式的FIB (Forwarding Information Base) 
* ip_rt_init() 还调用ip_fib_init() 初始化上层的路由相关数据结构
* inet_initpeers()初始化AVL tree用于跟踪最近有数据通信的IP peers和hosts。
```
1602void __init ip_init(void)
1603{
1604	ip_rt_init();
1605	inet_initpeers();
1606
1607#if defined(CONFIG_IP_MULTICAST)
1608	igmp_mc_init();
1609#endif
1610}
```


##  查询路由表
通过目的IP查询路由表的到下一跳的IP地址的过程, fib_lookup为起点，从[fib_lookup函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/include/net/ip_fib.h#222)这里可以进一步深入了解查询路由表的过程，当然这里需要理解路由表的数据结构和查询算法，会比较复杂。
```
222static inline int fib_lookup(struct net *net, const struct flowi4 *flp,
223			     struct fib_result *res)
224{
225	struct fib_table *table;
226
227	table = fib_get_table(net, RT_TABLE_LOCAL);
228	if (!fib_table_lookup(table, flp, res, FIB_LOOKUP_NOREF))
229		return 0;
230
231	table = fib_get_table(net, RT_TABLE_MAIN);
232	if (!fib_table_lookup(table, flp, res, FIB_LOOKUP_NOREF))
233		return 0;
234	return -ENETUNREACH;
235}
```
[5.x版本的fib_lookup函数](https://github.com/torvalds/linux/blob/386403a115f95997c2715691226e11a7b5cffcfd/include/net/ip_fib.h#L294) - [fib_table_lookup](https://github.com/torvalds/linux/blob/386403a115f95997c2715691226e11a7b5cffcfd/net/ipv4/fib_trie.c#L1314)

[struct flowi4](https://github.com/torvalds/linux/blob/63bdf4284c38a48af21745ceb148a087b190cd21/include/net/flow.h#L70)
```
struct flowi4 {
	struct flowi_common	__fl_common;
#define flowi4_oif		__fl_common.flowic_oif
#define flowi4_iif		__fl_common.flowic_iif
#define flowi4_mark		__fl_common.flowic_mark
#define flowi4_tos		__fl_common.flowic_tos
#define flowi4_scope		__fl_common.flowic_scope
#define flowi4_proto		__fl_common.flowic_proto
#define flowi4_flags		__fl_common.flowic_flags
#define flowi4_secid		__fl_common.flowic_secid
#define flowi4_tun_key		__fl_common.flowic_tun_key
#define flowi4_uid		__fl_common.flowic_uid
#define flowi4_multipath_hash	__fl_common.flowic_multipath_hash

	/* (saddr,daddr) must be grouped, same order as in IP header */
	__be32			saddr;
	__be32			daddr;

	union flowi_uli		uli;
#define fl4_sport		uli.ports.sport
#define fl4_dport		uli.ports.dport
#define fl4_icmp_type		uli.icmpt.type
#define fl4_icmp_code		uli.icmpt.code
#define fl4_ipsec_spi		uli.spi
#define fl4_mh_type		uli.mht.type
#define fl4_gre_key		uli.gre_key
} __attribute__((__aligned__(BITS_PER_LONG/8)));
```
有兴趣的读者可以进一步阅读或跟踪代码，如下为在[实验三中MenuOS系统](https://www.shiyanlou.com/courses/1198)上跟踪到fib_lookup函数时的调用栈。
```
2112		if (fib_lookup(net, fl4, &res)) {
(gdb) bt
#0  __ip_route_output_key (net=0xc1a08d40 <init_net>, fl4=0xc7bb8cf4)
    at net/ipv4/route.c:2112
#1  0xc161dc77 in ip_route_connect (protocol=<optimized out>, 
    sk=<optimized out>, dport=<optimized out>, sport=<optimized out>, 
    oif=<optimized out>, tos=<optimized out>, src=<optimized out>, 
    dst=<optimized out>, fl4=<optimized out>) at include/net/route.h:268
#2  tcp_v4_connect (sk=0x100007f, uaddr=<optimized out>, 
    addr_len=<optimized out>) at net/ipv4/tcp_ipv4.c:171
#3  0xc1631226 in __inet_stream_connect (sock=0xc763cd80, 
    uaddr=<optimized out>, addr_len=<optimized out>, flags=2)
    at net/ipv4/af_inet.c:592
---Type <return> to continue, or q <return> to quit---
#4  0xc163149a in inet_stream_connect (sock=0xc763cd80, uaddr=0xc7859d70, 
    addr_len=16, flags=2) at net/ipv4/af_inet.c:653
#5  0xc15a9289 in SYSC_connect (fd=<optimized out>, uservaddr=<optimized out>, 
    addrlen=16) at net/socket.c:1707
#6  0xc15aa0ae in SyS_connect (addrlen=16, uservaddr=-1080639076, fd=4)
    at net/socket.c:1688
#7  SYSC_socketcall (call=3, args=<optimized out>) at net/socket.c:2525
#8  0xc15aa90e in SyS_socketcall (call=3, args=-1080639136)
    at net/socket.c:2492
#9  <signal handler called>
#10 0xb7783c5c in ?? ()
```

## 结合IP包的首发过程从整体上理解IP协议及路由选择

IP数据包的收或发的过程是传输层协议数据收发过程的延伸，在分析TCP协议的过程中，我们涉及到数据收发的过程，同样对于IP协议，从上层传输层会调用IP协议的发送数据接口，见ip_queue_xmit；同时数据接收的过程中底层链路层会调用IP协议的接收数据的接口，见ip_rcv。

![](https://s1.51cto.com/images/blog/201901/08/62ed2fe659ad4a6d83b0e40969de6899.png?x-oss-process=image/watermark,size_16,text_QDUxQ1RP5Y2a5a6i,color_FFFFFF,t_100,g_se,x_10,y_10,shadow_90,type_ZmFuZ3poZW5naGVpdGk=)


## 参考资料

* https://people.cs.clemson.edu/~westall/853/notes/ipinit.pdf
* Behrouz A. Forouzan, Sophia Chung Fegan. TCP/IP Protocol Suite (3rd Edition)
* http://en.wikipedia.org/wiki/Routing_table 
* http://zh.wikipedia.org/wiki/%E5%86%85%E9%83%A8%E7%BD%91%E5%85%B3%E5%8D%8F%E8%AE%AE
* http://computer.bowenwang.com.cn/internet-infrastructure1.htm
* http://tools.ietf.org/html/rfc2453
* http://zh.wikipedia.org/zh-cn/%E5%BC%80%E6%94%BE%E5%BC%8F%E6%9C%80%E7%9F%AD%E8%B7%AF%E5%BE%84%E4%BC%98%E5%85%88

