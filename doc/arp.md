## 网络层与链路层的中间人——ARP协议及ARP缓存

路由选择得到输出结果是下一跳Next-Hop的IP地址和网络接口号，但是在发送IP数据包之前还需要得到目的MAC地址，这就需要用到ARP（Address Resolution Protocol）地址解析协议了。ARP用于将计算机的网络地址（IP地址32位）转化为物理地址（MAC地址48位），也就是将路由选择得到输出结果下一跳Next-Hop的IP地址通过查询ARP缓存得到对应的目的MAC地址。

![](https://s1.51cto.com/images/blog/201901/08/c76037e0cb827a22299460b85f408db5.png)

如上图，常见的MAC帧有三个类型：IP数据包、ARP和RARP。ARP请求/应答数据和IP数据包一样是由MAC帧承载的。

网络层的IP数据包（含有目的IP地址）需要封装成MAC帧（含有目的MAC地址）才能发送出去。如何得到目的MAC地址，从而将IP数据包发送到下一个中转站或最终目的地（下一跳Next-Hop）是TCP/IP网络得以有效工作的关键。从整个IP数据包传输的路径上，我们把ARP解析分解成四种典型的情况：

### 1 发送者与接收者在同一个网络

![](https://s1.51cto.com/images/blog/201901/08/b24d6f433b2f34df540cb310a7a19af5.png?x-oss-process=image/watermark,size_16,text_QDUxQ1RP5Y2a5a6i,color_FFFFFF,t_100,g_se,x_10,y_10,shadow_90,type_ZmFuZ3poZW5naGVpdGk=)

发送者与接收者在同一个网络，这时发送者查询路由表得到结果是目的IP即是下一跳的IP地址，只要解析目的IP地址对应的MAC地址，即可直接将IP数据包发送到接收者。

### 2 发送者需要首先发送给一个路由器

![](https://s1.51cto.com/images/blog/201901/08/0156e0b6f7f32a413438b0be88ade7f9.png)

发送者查询路由表得到结果是下一跳是一个路由器，需要将路由器的IP地址进行ARP解析获得路由器对应网络接口的MAC地址，即可将IP数据包发往路由器。这时IP数据包中的目的IP地址和MAC帧中的目的MAC地址并没有对应关系，只是IP数据包通过路由器所在网络作为中转站进行传输而已。

### 3 一个路由器转发给另一个路由器

![](https://s1.51cto.com/images/blog/201901/08/c02d038be419bdc7c609bcd01db41a2a.png)

一个路由器接到一个IP数据包，需要将IP数据包解包出目的IP地址，通过查询路由表得到下一跳的IP地址，这时下一跳往往还是一个路由器，将下一跳的IP地址解析出对应的MAC地址，即可将IP数据包发往另一个路由器。这时IP数据包中的目的IP地址和MAC帧中的目的MAC地址并没有对应关系，但是我们发现下一跳的IP地址始终与MAC帧中的目的MAC地址有着对应关系。

### 4 路由器将IP数据包发送给最终接收者

![](https://s1.51cto.com/images/blog/201901/08/26ffab864c3f84f2d396f5071b8019e1.png)

同样路由器接到一个IP数据包，需要将IP数据包解包出目的IP地址，通过查询路由表得到下一跳的IP地址，这时下一跳的IP地址与目的IP地址相同，因为查询路由表时目的IP地址与路由器所在网络的网络号相同。将下一跳的IP地址（这时即为目的IP地址）解析出对应的MAC地址，即可将IP数据包发往接收者。这种情况下一跳的IP地址与目的IP地址相同，这时目的IP地址与MAC帧中的目的MAC地址代表同一台主机。

## ARP协议源代码分析

### ARP缓存的数据结构及初始化过程

参照tcp和ip协议的的初始化过程类似，查找arp初始化相关代码，见[inet_init](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/af_inet.c#1730)函数：

```
1730	/*
1731	 *	Set the ARP module up
1732	 */
1733
1734	arp_init();
```
[arp_init](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/arp.c#1293)函数如下，其中包括ARP缓存的初始化。
```
1293void __init arp_init(void)
1294{
1295	neigh_table_init(&arp_tbl);
1296
1297	dev_add_pack(&arp_packet_type);
1298	arp_proc_init();
1299#ifdef CONFIG_SYSCTL
1300	neigh_sysctl_register(NULL, &arp_tbl.parms, NULL);
1301#endif
1302	register_netdevice_notifier(&arp_netdev_notifier);
1303}
```
### ARP协议如何更新ARP缓存

ARP协议的实现代码量不大，主要集中在[arp.c文件](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/arp.c#1293)中。

### 创建和发送一个ARP封包

```
692/*
693 *	Create and send an arp packet.
694 */
695void arp_send(int type, int ptype, __be32 dest_ip,
696	      struct net_device *dev, __be32 src_ip,
697	      const unsigned char *dest_hw, const unsigned char *src_hw,
698	      const unsigned char *target_hw)
699{
700	struct sk_buff *skb;
701
702	/*
703	 *	No arp on this interface.
704	 */
705
706	if (dev->flags&IFF_NOARP)
707		return;
708
709	skb = arp_create(type, ptype, dest_ip, dev, src_ip,
710			 dest_hw, src_hw, target_hw);
711	if (skb == NULL)
712		return;
713
714	arp_xmit(skb);
715}
```
### 接收并处理一个ARP封包

```
1282/*
1283 *	Called once on startup.
1284 */
1285
1286static struct packet_type arp_packet_type __read_mostly = {
1287	.type =	cpu_to_be16(ETH_P_ARP),
1288	.func =	arp_rcv,
1289};
```
其中callback函数指针arp_rcv由链路层接收到数据后根据MAC帧的类型回调arp_rcv进行ARP数据封包的处理。
```
947/*
948 *	Receive an arp request from the device layer.
949 */
950
951static int arp_rcv(struct sk_buff *skb, struct net_device *dev,
952		   struct packet_type *pt, struct net_device *orig_dev)
953{
954	const struct arphdr *arp;
955
956	/* do not tweak dropwatch on an ARP we will ignore */
957	if (dev->flags & IFF_NOARP ||
958	    skb->pkt_type == PACKET_OTHERHOST ||
959	    skb->pkt_type == PACKET_LOOPBACK)
960		goto consumeskb;
961
962	skb = skb_share_check(skb, GFP_ATOMIC);
963	if (!skb)
964		goto out_of_mem;
965
966	/* ARP header, plus 2 device addresses, plus 2 IP addresses.  */
967	if (!pskb_may_pull(skb, arp_hdr_len(dev)))
968		goto freeskb;
969
970	arp = arp_hdr(skb);
971	if (arp->ar_hln != dev->addr_len || arp->ar_pln != 4)
972		goto freeskb;
973
974	memset(NEIGH_CB(skb), 0, sizeof(struct neighbour_cb));
975
976	return NF_HOOK(NFPROTO_ARP, NF_ARP_IN, skb, dev, NULL, arp_process);
```
具体的ARP协议解析过程主要集中在[arp_process函数中](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/arp.c#arp_process) ,有兴趣的读者可以仔细研究。

```
718/*
719 *	Process an arp request.
720 */
721
722static int arp_process(struct sk_buff *skb)
723{
...
```
### 如何将IP地址解析出对应的MAC地址

通过跟踪MenuOS中connect建立连接的代码， __ipv4_neigh_lookup_noref函数负责通过查询ARP缓存，connect在内核里的调用栈一直到__ipv4_neigh_lookup_noref函数：

```
197		neigh = __ipv4_neigh_lookup_noref(dev, nexthop);
(gdb) bt
#0  ip_finish_output2 (skb=<optimized out>) at net/ipv4/ip_output.c:197
#1  ip_finish_output (skb=0xc7bb30b8) at net/ipv4/ip_output.c:271
#2  0xc1603de7 in NF_HOOK_COND (pf=<optimized out>, hook=<optimized out>, 
    in=<optimized out>, okfn=<optimized out>, cond=<optimized out>, 
    out=<optimized out>, skb=<optimized out>) at include/linux/netfilter.h:187
#3  ip_output (sk=<optimized out>, skb=0xc7bb30b8) at net/ipv4/ip_output.c:343
#4  0xc16034d2 in dst_output_sk (skb=<optimized out>, sk=<optimized out>)
    at include/net/dst.h:458
#5  ip_local_out_sk (sk=0xc7bb8ac0, skb=0xc7bb30b8) at net/ipv4/ip_output.c:110
#6  0xc16037df in ip_local_out (skb=<optimized out>) at include/net/ip.h:117
#7  ip_queue_xmit (sk=0xc7bb8ac0, skb=0xc7ae6d00, fl=<optimized out>)
    at net/ipv4/ip_output.c:439
#8  0xc1618513 in tcp_transmit_skb (sk=0xc7bb8ac0, skb=0xc7ae6d00, 
    clone_it=<optimized out>, gfp_mask=208) at net/ipv4/tcp_output.c:1012
#9  0xc1619fa0 in tcp_connect (sk=0xc7bb8ac0) at net/ipv4/tcp_output.c:3117
#10 0xc161de66 in tcp_v4_connect (sk=0xd4, uaddr=0xc7859d70, 
    addr_len=<optimized out>) at net/ipv4/tcp_ipv4.c:246
#11 0xc1631226 in __inet_stream_connect (sock=0xc763cd80, 
    uaddr=<optimized out>, addr_len=<optimized out>, flags=2)
    at net/ipv4/af_inet.c:592
#12 0xc163149a in inet_stream_connect (sock=0xc763cd80, uaddr=0xc7859d70, 
    addr_len=16, flags=2) at net/ipv4/af_inet.c:653
#13 0xc15a9289 in SYSC_connect (fd=<optimized out>, uservaddr=<optimized out>, 
    addrlen=16) at net/socket.c:1707
#14 0xc15aa0ae in SyS_connect (addrlen=16, uservaddr=-1080639076, fd=4)
    at net/socket.c:1688
#15 SYSC_socketcall (call=3, args=<optimized out>) at net/socket.c:2525
#16 0xc15aa90e in SyS_socketcall (call=3, args=-1080639136)
    at net/socket.c:2492
```

从代码的封装看arp_find是负责ARP缓存查询的，但实际上对于IPv4来讲是由__ipv4_neigh_lookup_noref函数完成ARP缓存查询的。
