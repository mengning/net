本文从用户态的socket接口API、系统调用、Linux内核中socket接口层代码，到网络协议栈TCP协议、IP协议及路由选择、ARP协议及ARP解析，然后进一步分析了数据链路层的MAC帧在二层网络上的CSMA/CD机制及交换网络上学习和过滤机制。到这里我们庖丁解牛式地逐一分析了Linux网络核心中的每一个关键部分，本文我们将以上这些整合起来，完整梳理一下数据收发过程在Linux网络核心中所必经的关键代码。您还可以以DNS和HTTP这两个最常用的应用层协议为例来进一步理解Linux网络核心

### socket接口API

以我们的MenuOS为例，在用户态执行hello命令时调用了send和recv两个socket接口API函数。这两个函数的原型声明一般在/usr/include/sys/socket.h文件中：

```
...
/* Send N bytes of BUF to socket FD.  Returns the number sent or -1.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern ssize_t send (int __fd, const void *__buf, size_t __n, int __flags);

/* Read N bytes into BUF from socket FD.
   Returns the number read or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern ssize_t recv (int __fd, void *__buf, size_t __n, int __flags);
...
```
根据实验环境MenuOS中实际跟踪发现send和recv都是通过调用socketcall函数实现的。

### socket系统调用

在我们的实验环境MenuOS中，socket接口是通过[112号系统调用](http://codelab.shiyanlou.com/xref/linux-3.18.6/arch/x86/syscalls/syscall_32.tbl#111) 进入内核的.

```
102	i386	socketcall		sys_socketcall			compat_sys_socketcall
```

![](https://s1.51cto.com/images/blog/201902/01/ab56fa133217b6edd0a2a3d4216fa9ff.png)

上图中xyz()就是一个API函数，比如socketcall函数，是系统调用对应的API接口上，其中封装了一个系统调用，会触发int $0x80的中断，对应system_call内核代码的起点，即中断向量0x80对应的中断服务程序入口，内部会有sys_xyz()系统调用处理函数，比如sys_socketcall函数。

### socket接口层数据收发代码分析

[sys_socketcall函数](http://codelab.shiyanlou.com/xref/linux-3.18.6/net/socket.c#2484)中通过参数call来指明具体是哪一个socket接口，比如send函数对应SYS_SEND = 9，recv函数对应的SYS_RECV = 10。
```
2484/*
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
2547	case SYS_SEND:
2548		err = sys_send(a0, (void __user *)a1, a[2], a[3]);
...
2554	case SYS_RECV:
2555		err = sys_recv(a0, (void __user *)a1, a[2], a[3]);
...
2595}
```
#### send函数对应执行[sys_send函数及sys_sendto函数](http://codelab.shiyanlou.com/xref/linux-3.18.6/net/socket.c#1777)
```
1777/*
1778 *	Send a datagram to a given address. We move the address into kernel
1779 *	space and check the user space data area is readable before invoking
1780 *	the protocol.
1781 */
1782
1783SYSCALL_DEFINE6(sendto, int, fd, void __user *, buff, size_t, len,
1784		unsigned int, flags, struct sockaddr __user *, addr,
1785		int, addr_len)
1786{
...
1818	err = sock_sendmsg(sock, &msg, len);
...
1824}
1825
1826/*
1827 *	Send a datagram down a socket.
1828 */
1829
1830SYSCALL_DEFINE4(send, int, fd, void __user *, buff, size_t, len,
1831		unsigned int, flags)
1832{
1833	return sys_sendto(fd, buff, len, flags, NULL, 0);
1834}
```
其中sock_sendmsg最终调用了函数指针sock->ops->sendmsg，具体代码见[net/socket.c#633](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/socket.c#633) ，如下：
```
633static inline int __sock_sendmsg_nosec(struct kiocb *iocb, struct socket *sock,
634				       struct msghdr *msg, size_t size)
635{
636	struct sock_iocb *si = kiocb_to_siocb(iocb);
637
638	si->sock = sock;
639	si->scm = NULL;
640	si->msg = msg;
641	si->size = size;
642
643	return sock->ops->sendmsg(iocb, sock, msg, size);
644}
645
646static inline int __sock_sendmsg(struct kiocb *iocb, struct socket *sock,
647				 struct msghdr *msg, size_t size)
648{
649	int err = security_socket_sendmsg(sock, msg, size);
650
651	return err ?: __sock_sendmsg_nosec(iocb, sock, msg, size);
652}
653
654int sock_sendmsg(struct socket *sock, struct msghdr *msg, size_t size)
655{
656	struct kiocb iocb;
657	struct sock_iocb siocb;
658	int ret;
659
660	init_sync_kiocb(&iocb, NULL);
661	iocb.private = &siocb;
662	ret = __sock_sendmsg(&iocb, sock, msg, size);
663	if (-EIOCBQUEUED == ret)
664		ret = wait_on_sync_kiocb(&iocb);
665	return ret;
666}
667EXPORT_SYMBOL(sock_sendmsg);
```
#### recv函数对应的[sys_recv函数及sys_recvfrom函数](http://codelab.shiyanlou.com/xref/linux-3.18.6/net/socket.c#1836)
```
1836/*
1837 *	Receive a frame from the socket and optionally record the address of the
1838 *	sender. We verify the buffers are writable and if needed move the
1839 *	sender address from kernel to user space.
1840 */
1841
1842SYSCALL_DEFINE6(recvfrom, int, fd, void __user *, ubuf, size_t, size,
1843		unsigned int, flags, struct sockaddr __user *, addr,
1844		int __user *, addr_len)
1845{
...
1871	err = sock_recvmsg(sock, &msg, size, flags);
...
1883}
1884
1885/*
1886 *	Receive a datagram from a socket.
1887 */
1888
1889SYSCALL_DEFINE4(recv, int, fd, void __user *, ubuf, size_t, size,
1890		unsigned int, flags)
1891{
1892	return sys_recvfrom(fd, ubuf, size, flags, NULL, NULL);
1893}
```
其中sock_recvmsg最终调用了函数指针sock->ops->recvmsg，具体代码见[net/socket.c#779](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/socket.c#779) ，如下：
```
779static inline int __sock_recvmsg_nosec(struct kiocb *iocb, struct socket *sock,
780				       struct msghdr *msg, size_t size, int flags)
781{
782	struct sock_iocb *si = kiocb_to_siocb(iocb);
783
784	si->sock = sock;
785	si->scm = NULL;
786	si->msg = msg;
787	si->size = size;
788	si->flags = flags;
789
790	return sock->ops->recvmsg(iocb, sock, msg, size, flags);
791}
792
793static inline int __sock_recvmsg(struct kiocb *iocb, struct socket *sock,
794				 struct msghdr *msg, size_t size, int flags)
795{
796	int err = security_socket_recvmsg(sock, msg, size, flags);
797
798	return err ?: __sock_recvmsg_nosec(iocb, sock, msg, size, flags);
799}
800
801int sock_recvmsg(struct socket *sock, struct msghdr *msg,
802		 size_t size, int flags)
803{
804	struct kiocb iocb;
805	struct sock_iocb siocb;
806	int ret;
807
808	init_sync_kiocb(&iocb, NULL);
809	iocb.private = &siocb;
810	ret = __sock_recvmsg(&iocb, sock, msg, size, flags);
811	if (-EIOCBQUEUED == ret)
812		ret = wait_on_sync_kiocb(&iocb);
813	return ret;
814}
815EXPORT_SYMBOL(sock_recvmsg);
```
### TCP协议栈数据收发代码分析

上述提及了两个函数指针sock->ops->sendmsg和sock->ops->recvmsg在我们的lab3实验中对应着tcp_sendmsg和tcp_recvmsg函数，与前面文章中介绍的connect及TCP三次握手一样，tcp_sendmsg和tcp_recvmsg函数是通过如下[struct proto tcp_prot结构体变量](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp_ipv4.c#2380)赋值给对应函数指针的:
```
2380struct proto tcp_prot = {
2381	.name			= "TCP",
2382	.owner			= THIS_MODULE,
2383	.close			= tcp_close,
2384	.connect		= tcp_v4_connect,
...
2393	.recvmsg		= tcp_recvmsg,
2394	.sendmsg		= tcp_sendmsg,
...
2426};
2427EXPORT_SYMBOL(tcp_prot);
```

由于TCP是面向连接的提供可靠的字节流传输服务，TCP的发送和接收涉及比较复杂的滑动窗口协议和确认重传机制，具体分析发送和接收的细节会比较复杂，我们这里为了简化期间只提纲挈领提及一些关键的函数。

#### [tcp_sendmsg](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp.c#1085)

```
1085int tcp_sendmsg(struct kiocb *iocb, struct sock *sk, struct msghdr *msg,
1086		size_t size)
1087{
...
1275			if (forced_push(tp)) {
1276				tcp_mark_push(tp, skb);
1277				__tcp_push_pending_frames(sk, mss_now, TCP_NAGLE_PUSH);
1278			} else if (skb == tcp_send_head(sk))
1279				tcp_push_one(sk, mss_now);
1280			continue;
1281
1282wait_for_sndbuf:
1283			set_bit(SOCK_NOSPACE, &sk->sk_socket->flags);
1284wait_for_memory:
1285			if (copied)
1286				tcp_push(sk, flags & ~MSG_MORE, mss_now,
1287					 TCP_NAGLE_PUSH, size_goal);
1288
1289			if ((err = sk_stream_wait_memory(sk, &timeo)) != 0)
1290				goto do_error;
1291
1292			mss_now = tcp_send_mss(sk, &size_goal, flags);
1293		}
1294	}
1295
1296out:
1297	if (copied)
1298		tcp_push(sk, flags, mss_now, tp->nonagle, size_goal);
...
1320}
1321EXPORT_SYMBOL(tcp_sendmsg);
```
其中的tcp_push和tcp_push_one都会最终通过如下代码调用[tcp_write_xmit函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp_output.c#1926)
```
2195/* Push out any pending frames which were held back due to
2196 * TCP_CORK or attempt at coalescing tiny packets.
2197 * The socket must be locked by the caller.
2198 */
2199void __tcp_push_pending_frames(struct sock *sk, unsigned int cur_mss,
2200			       int nonagle)
2201{
2202	/* If we are closed, the bytes will have to remain here.
2203	 * In time closedown will finish, we empty the write queue and
2204	 * all will be happy.
2205	 */
2206	if (unlikely(sk->sk_state == TCP_CLOSE))
2207		return;
2208
2209	if (tcp_write_xmit(sk, cur_mss, nonagle, 0,
2210			   sk_gfp_atomic(sk, GFP_ATOMIC)))
2211		tcp_check_probe_timer(sk);
2212}
2213
2214/* Send _single_ skb sitting at the send head. This function requires
2215 * true push pending frames to setup probe timer etc.
2216 */
2217void tcp_push_one(struct sock *sk, unsigned int mss_now)
2218{
2219	struct sk_buff *skb = tcp_send_head(sk);
2220
2221	BUG_ON(!skb || skb->len < mss_now);
2222
2223	tcp_write_xmit(sk, mss_now, TCP_NAGLE_PUSH, 1, sk->sk_allocation);
2224}
```
[tcp_write_xmit函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp_output.c#1926)进一步调用[tcp_transmit_skb函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp_output.c#876)
```
876/* This routine actually transmits TCP packets queued in by
877 * tcp_do_sendmsg().  This is used by both the initial
878 * transmission and possible later retransmissions.
879 * All SKB's seen here are completely headerless.  It is our
880 * job to build the TCP header, and pass the packet down to
881 * IP so it can do the same plus pass the packet off to the
882 * device.
883 *
884 * We are working here with either a clone of the original
885 * SKB, or a fresh unique copy made by the retransmit engine.
886 */
887static int tcp_transmit_skb(struct sock *sk, struct sk_buff *skb, int clone_it,
888			    gfp_t gfp_mask)
889{
...
946	/* Build TCP header and checksum it. */
947	th = tcp_hdr(skb);
948	th->source		= inet->inet_sport;
949	th->dest		= inet->inet_dport;
950	th->seq			= htonl(tcb->seq);
951	th->ack_seq		= htonl(tp->rcv_nxt);
...
1012	err = icsk->icsk_af_ops->queue_xmit(sk, skb, &inet->cork.fl);
...
1020}
```
其中icsk->icsk_af_ops->queue_xmit函数指针由如下结构体变量初始化为ip_queue_xmit函数，这里进入IP协议栈，我们稍后再进一步分析。
```
1766const struct inet_connection_sock_af_ops ipv4_specific = {
1767	.queue_xmit	   = ip_queue_xmit,
...
1784};
1785EXPORT_SYMBOL(ipv4_specific);
```
#### [tcp_recvmsg](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp.c#1577)

[tcp_recvmsg](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp.c#1577)函数对应用户态recv函数的表现：阻塞函数直到接收到数据（len>0）返回接收到的数据。

```
1577/*
1578 *	This routine copies from a sock struct into the user buffer.
1579 *
1580 *	Technical note: in 2.3 we work on _locked_ socket, so that
1581 *	tricks with *seq access order and skb->users are not required.
1582 *	Probably, code can be easily improved even more.
1583 */
1584
1585int tcp_recvmsg(struct kiocb *iocb, struct sock *sk, struct msghdr *msg,
1586		size_t len, int nonblock, int flags, int *addr_len)
1587{
...
1642	do {
...
1872	} while (len > 0);
1873
1874	if (user_recv) {
1875		if (!skb_queue_empty(&tp->ucopy.prequeue)) {
1876			int chunk;
1877
1878			tp->ucopy.len = copied > 0 ? len : 0;
1879
1880			tcp_prequeue_process(sk);
1881
1882			if (copied > 0 && (chunk = len - tp->ucopy.len) != 0) {
1883				NET_ADD_STATS_USER(sock_net(sk), LINUX_MIB_TCPDIRECTCOPYFROMPREQUEUE, chunk);
1884				len -= chunk;
1885				copied += chunk;
1886			}
1887		}
...
1914}
1915EXPORT_SYMBOL(tcp_recvmsg);
```
这里的[tcp_prequeue_process函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp.c#1454)是通过sk_backlog_rcv函数从接收队列中出队skb，那么按照数据接收数据流的逻辑上，一定存在将将接收到的skb数据入队的地方。

```
1454static void tcp_prequeue_process(struct sock *sk)
1455{
...
1464	while ((skb = __skb_dequeue(&tp->ucopy.prequeue)) != NULL)
1465		sk_backlog_rcv(sk, skb);
...
1470}
```
在前面分析TCP三次握手我们知道IP层接收到TCP数据是通过函数指针回调tcp_v4_rcv函数来通知TCP协议栈接收数据的。
```
1585int tcp_v4_rcv(struct sk_buff *skb)
1586{
...
1673		if (!tcp_prequeue(sk, skb))
1674			ret = tcp_v4_do_rcv(sk, skb);
...
1746}
```
其中[tcp_v4_do_rcv函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp_ipv4.c#1427)调用了tcp_child_process函数

```
1427int tcp_v4_do_rcv(struct sock *sk, struct sk_buff *skb)
1428{
...
1449	if (sk->sk_state == TCP_LISTEN) {
...
1456			if (tcp_child_process(sk, nsk, skb)) {
...
1486}
1487EXPORT_SYMBOL(tcp_v4_do_rcv);
```
[tcp_child_process函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp_minisocks.c#755)调用了__sk_add_backlog将skb入队，与前面sk_backlog_rcv出队对应。
```
755/*
756 * Queue segment on the new socket if the new socket is active,
757 * otherwise we just shortcircuit this and continue with
758 * the new socket.
759 *
760 * For the vast majority of cases child->sk_state will be TCP_SYN_RECV
761 * when entering. But other states are possible due to a race condition
762 * where after __inet_lookup_established() fails but before the listener
763 * locked is obtained, other packets cause the same connection to
764 * be created.
765 */
766
767int tcp_child_process(struct sock *parent, struct sock *child,
768		      struct sk_buff *skb)
769{
...
784		__sk_add_backlog(child, skb);
...
790}
791EXPORT_SYMBOL(tcp_child_process);
```
### IP协议栈数据收发代码分析

从TCP协议栈数据收发代码分析中我们知道IP发送数据是通过ip_queue_xmit，而二层数据链路层接收IP数据包回调IP协议栈进一步处理接收数据是通过ip_rcv函数，初始化[函数指针func](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/af_inet.c#1669) 的代码如下：
```
1669static struct packet_type ip_packet_type __read_mostly = {
1670	.type = cpu_to_be16(ETH_P_IP),
1671	.func = ip_rcv,
1672};
```
#### [ip_queue_xmit](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_output.c#362)
[ip_queue_xmit](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_output.c#362)函数摘录如下：
```
362/* Note: skb->sk can be different from sk, in case of tunnels */
363int ip_queue_xmit(struct sock *sk, struct sk_buff *skb, struct flowi *fl)
364{
...
439	res = ip_local_out(skb);
...
448}
449EXPORT_SYMBOL(ip_queue_xmit);
```
其中的[ip_local_out内联函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/include/net/ip.h#115) 如下：
```
115static inline int ip_local_out(struct sk_buff *skb)
116{
117	return ip_local_out_sk(skb->sk, skb);
118}
```
上面的[ip_local_out_sk函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_output.c#94)最终通过nf_hook提供的回调机制调用了dst_output函数。
```
94int __ip_local_out(struct sk_buff *skb)
95{
96	struct iphdr *iph = ip_hdr(skb);
97
98	iph->tot_len = htons(skb->len);
99	ip_send_check(iph);
100	return nf_hook(NFPROTO_IPV4, NF_INET_LOCAL_OUT, skb, NULL,
101		       skb_dst(skb)->dev, dst_output);
102}
103
104int ip_local_out_sk(struct sock *sk, struct sk_buff *skb)
105{
106	int err;
107
108	err = __ip_local_out(skb);
109	if (likely(err == 1))
110		err = dst_output_sk(sk, skb);
111
112	return err;
113}
114EXPORT_SYMBOL_GPL(ip_local_out_sk);
```
[dst_output函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/include/net/dst.h#455)最终调用了函数指针skb_dst(skb)->output(sk, skb)：
```
455/* Output packet to network from transport.  */
456static inline int dst_output_sk(struct sock *sk, struct sk_buff *skb)
457{
458	return skb_dst(skb)->output(sk, skb);
459}
460static inline int dst_output(struct sk_buff *skb)
461{
462	return dst_output_sk(skb->sk, skb);
463}
```
skb_dst(skb)返回的是[struct dst_entry数据结构](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/include/net/dst.h#33)指针，我们也就找到了output函数指针。
```
33struct dst_entry {
...
47	int			(*input)(struct sk_buff *);
48	int			(*output)(struct sock *sk, struct sk_buff *skb);
```
上述input和output函数指针在IP协议栈及路由表（ip_init及ip_rt_init）初始化过程被赋值如下，见[linux-3.18.6/net/ipv4/route.c#1610](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/route.c#1610)。
```
1610	rth->dst.input = ip_forward;
1611	rth->dst.output = ip_output;
```
[ip_output函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_output.c#334)通过NF_HOOK_COND方式调用ip_finish_output
```
334int ip_output(struct sock *sk, struct sk_buff *skb)
335{
336	struct net_device *dev = skb_dst(skb)->dev;
337
338	IP_UPD_PO_STATS(dev_net(dev), IPSTATS_MIB_OUT, skb->len);
339
340	skb->dev = dev;
341	skb->protocol = htons(ETH_P_IP);
342
343	return NF_HOOK_COND(NFPROTO_IPV4, NF_INET_POST_ROUTING, skb, NULL, dev,
344			    ip_finish_output,
345			    !(IPCB(skb)->flags & IPSKB_REROUTED));
346}
```
[ip_finish_output函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_output.c#ip_finish_output)又进一步调用ip_finish_output2
```
256static int ip_finish_output(struct sk_buff *skb)
257{
258#if defined(CONFIG_NETFILTER) && defined(CONFIG_XFRM)
259	/* Policy lookup after SNAT yielded a new policy */
260	if (skb_dst(skb)->xfrm != NULL) {
261		IPCB(skb)->flags |= IPSKB_REROUTED;
262		return dst_output(skb);
263	}
264#endif
265	if (skb_is_gso(skb))
266		return ip_finish_output_gso(skb);
267
268	if (skb->len > ip_skb_dst_mtu(skb))
269		return ip_fragment(skb, ip_finish_output2);
270
271	return ip_finish_output2(skb);
272}
```
[ip_finish_output2函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_output.c#166)有一段关键代码，这里获取了路由查询的结果nexthop，并将nexthop作为参数由__ipv4_neigh_lookup_noref函数进行ARP解析。
```
166static inline int ip_finish_output2(struct sk_buff *skb)
167{
...
196	nexthop = (__force u32) rt_nexthop(rt, ip_hdr(skb)->daddr);
197	neigh = __ipv4_neigh_lookup_noref(dev, nexthop);
198	if (unlikely(!neigh))
199		neigh = __neigh_create(&arp_tbl, &nexthop, dev, false);
200	if (!IS_ERR(neigh)) {
201		int res = dst_neigh_output(dst, neigh, skb);
...
212}
```
上述代码中的dst_neigh_output函数通过调用neigh_hh_output进一步调用dev_queue_xmit，将数据发送的工作交给二层数据链路层及设备驱动处理。
```
390static inline int neigh_hh_output(const struct hh_cache *hh, struct sk_buff *skb)
391{
...
409	return dev_queue_xmit(skb);
410}
```
#### [ip_rcv](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_input.c#373)

```
373/*
374 * 	Main IP Receive routine.
375 */
376int ip_rcv(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev)
377{
...
453	return NF_HOOK(NFPROTO_IPV4, NF_INET_PRE_ROUTING, skb, dev, NULL,
454		       ip_rcv_finish);
...
464}
```
[ip_rcv](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_input.c#373)函数通过NF_HOOK方式回调[ip_rcv_finish函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_input.c#312)
```
312static int ip_rcv_finish(struct sk_buff *skb)
313{
...
329	/*
330	 *	Initialise the virtual path cache for the packet. It describes
331	 *	how the packet travels inside Linux networking.
332	 */
333	if (!skb_dst(skb)) {
334		int err = ip_route_input_noref(skb, iph->daddr, iph->saddr,
335					       iph->tos, skb->dev);
...
358	rt = skb_rtable(skb);
...
366	return dst_input(skb);
...
371}
```
[ip_rcv_finish函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_input.c#312)最后调用了[dst_input函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/include/net/dst.h#465)，进一步调用了函数指针skb_dst(skb)->input(skb)。
```
465/* Input packet from network to transport.  */
466static inline int dst_input(struct sk_buff *skb)
467{
468	return skb_dst(skb)->input(skb);
469}
```
函数指针skb_dst(skb)->input(skb)的input的初始化一般有两个常见的路径：

一是当前系统作为中间设备（如交换机、路由器），input函数指针会对接收到的数据包进行转发处理，input函数指针的初始化代码见：
```
rth->dst.input = ip_forward;
```
二是当前系统是接收到的数据包的目的地，input函数指针会对接收到的数据包交给上层协议处理，input函数指针的初始化代码见：
```
rth->dst.input= ip_local_deliver;
```
本文以实验三lab3的代码为例，我们关注的是第二种情况，即[ip_local_deliver函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_input.c#242):

```
242/*
243 * 	Deliver IP Packets to the higher protocol layers.
244 */
245int ip_local_deliver(struct sk_buff *skb)
246{
...
256	return NF_HOOK(NFPROTO_IPV4, NF_INET_LOCAL_IN, skb, skb->dev, NULL,
257		       ip_local_deliver_finish);
258}
```
[ip_local_deliver函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_input.c#242)通过NF_HOOK方式回调[ip_local_deliver_finish函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_input.c#190)：
```
190static int ip_local_deliver_finish(struct sk_buff *skb)
191{
...
216			ret = ipprot->handler(skb);
...
240}
```
至此调用了函数指针ipprot->handler(skb)，前面TCP协议栈接收数据的起点tcp_v4_rcv，其被如下代码初始化为handler函数指针，留心的读者应该还记得[TCP/IP协议栈的初始化inet_init函数中有相关初始化代码](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/af_inet.c#1716)
```
1498static const struct net_protocol tcp_protocol = {
1499	.early_demux	=	tcp_v4_early_demux,
1500	.handler	=	tcp_v4_rcv,
...
1505};
```
### 二层数据链路层及设备驱动程序中数据收发代码分析

参考[9.以太网数据帧的格式及数据链路层发送和接收数据帧的处理过程分析](http://blog.51cto.com/cloumn/blog/805) 可知IP协议栈通过dev_queue_xmit向下层发送数据，netif_rx负责从网络设备驱动程序中接收数据。

由于[9.以太网数据帧的格式及数据链路层发送和接收数据帧的处理过程分析](http://blog.51cto.com/cloumn/blog/805) 文章中已经进行了详实的代码分析这里略去部分内容，仅列举loopback驱动程序的关键代码，以便将实验三lab3中本机TCP客户端和TCP服务器的通信过程串连起来。
在lo回环网络设备的情况下，dev_queue_xmit函数最终会调用loopback网络设备驱动程序中的[loopback_xmit函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/drivers/net/loopback.c#67)发送数据，[loopback_xmit函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/drivers/net/loopback.c#67)中直接调用了netif_rx负责接收数据，在如下代码中也就形成了回环，这就是loopback回环设备名称的由来。
```
67/*
68 * The higher levels take care of making this non-reentrant (it's
69 * called with bh's disabled).
70 */
71static netdev_tx_t loopback_xmit(struct sk_buff *skb,
72				 struct net_device *dev)
73{
...
90	if (likely(netif_rx(skb) == NET_RX_SUCCESS)) {
...
98}
```

## 总结

本文完整分析了从用户态socket API函数send/recv通过系统调用进入内核后，一直到网络设备驱动程序，通过发送和接收数据两条主线梳理了代码调用过程。由于设备的代码层级较深，代码也较为复杂，因为我们只是基于loopback网络设备和IPv4为例进行了梳理，如果读者对Linux内核网络核心的其他部分（如IPv6、其他网卡设备等）感兴趣可以自行进行类似的分析或拓展。
