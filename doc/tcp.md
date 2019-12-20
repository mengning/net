# TCP协议源代码跟踪分析
本文从TCP的基本概念和TCP三次握手的过程入手，结合客户端connect和服务端accept建立起连接时背后的完成的工作，在内核socket接口层这两个socket API函数对应着sys_connect和sys_accept函数，进一步对应着sock->opt->connect和sock->opt->accept两个函数指针，在TCP协议中这两个函数指针对应着tcp_v4_connect函数和inet_csk_accept函数，进一步触及TCP数据收发过程tcp_transmit_skb和tcp_v4_rcv函数，从整体上理解TCP协议栈向上提供的接口和向下提供的接口。

## TCP的基本概念

TCP协议提供提供一种面向连接的、可靠的字节流服务。所谓“面向连接”即是通过三次握手建立的一个通信连接；所谓“可靠的字节流服务”即是数据发送方等待数据接收方发送确认才清除发送数据缓存，如果没有收到接收方的确认信息则等待超时重发没有得到确认的部分字节。这是TCP协议提供的服务的基本概念。

TCP将用户数据打包（分割）成报文段；发送后启动一个定时器；另一端对收到的数据进行确认，对失序的重新排序，丢弃重复数据；TCP提供端到端的流量控制，并计算和验证一个强制性的端到端检验和。

### TCP协议在TCP/IP协议族中的位置

![](http://i2.51cto.com/images/blog/201811/29/480cfc34ee454e0ab0ffcc215893b25c.png)

### 以telnet应用为例看TCP协议提供的服务

![](http://i2.51cto.com/images/blog/201811/29/b507fbecf352bcd2de5463767310729d.png)

上图可以清晰地看到TCP是进程到进程之间的传输协议，主机使用端口来区分不同的进程。

### TCP三次握手的过程

![](http://i2.51cto.com/images/blog/201811/29/a4cd02d6787341822b781dfa03b79808.png)

上图将服务端和客户端socket接口的调用顺序与TCP三次握手的机制结合起来展示了连接的建立过程，同时通过SYN/ACK的机制展示了客户端到服务端和服务端到客户端两条可靠的字节流的实现原理。

## TCP协议源代码概览

TCP协议相关的代码主要集中在linux-3.18.6/net/ipv4/目录下，其中[linux-3.18.6/net/ipv4/tcp_ipv4.c#2380](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp_ipv4.c#2380) 文件中的结构体变量struct proto tcp_prot指定了TCP协议栈的访问接口函数，socket接口层里sock->opt->connect和sock->opt->accept对应的接口函数即是在这里制定的，sock->opt->connect实际调用的是tcp_v4_connect函数，sock->opt->accept实际调用的是inet_csk_accept函数。

在创建socket套接字描述符时sys_socket内核函数会根据指定的协议（例如socket(PF_INET,SOCK_STREAM,0)）挂载上对应的协议处理函数。

除了TCP协议的访问接口，还有TCP协议的初始化工作，见[linux-3.18.6/net/ipv4/tcp.c#3046](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp.c#3046)，其中关键的工作就是[tcp_tasklet_init();](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp.c#3124)初始化了负责发送字节流进行滑动窗口管理的tasklet，这里读者可以简单的理解为创建了线程来专门负责这个工作。

### TCP的三次握手的源代码分析

TCP的三次握手从用户程序的角度看就是客户端connect和服务端accept建立起连接时背后的完成的工作，在内核socket接口层这两个socket API函数对应着sys_connect和sys_accept函数，进一步对应着sock->opt->connect和sock->opt->accept两个函数指针，在TCP协议中这两个函数指针对应着tcp_v4_connect函数和inet_csk_accept函数。

分析到这里，读者应该能够想到我们可以通过MenuOS的内核调试环境设置断点跟踪tcp_v4_connect函数和inet_csk_accept函数来进一步验证三次握手的过程。

### tcp_v4_connect函数

[tcp_v4_connect函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp_ipv4.c#141)的主要作用就是发起一个TCP连接，建立TCP连接的过程自然需要底层协议的支持，因此我们从这个函数中可以看到它调用了IP层提供的一些服务，比如ip_route_connect和ip_route_newports从名称就可以简单分辨，这里我们关注在TCP层面的三次握手，不去深究底层协议提供的功能细节。我们可以看到这里设置了 TCP_SYN_SENT并进一步调用了 tcp_connect(sk)来实际构造SYN并发送出去。

```
140/* This will initiate an outgoing connection. */
141int tcp_v4_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len)
142{
...
171	rt = ip_route_connect(fl4, nexthop, inet->inet_saddr,
172			      RT_CONN_FLAGS(sk), sk->sk_bound_dev_if,
173			      IPPROTO_TCP,
174			      orig_sport, orig_dport, sk);
...
215	/* Socket identity is still unknown (sport may be zero).
216	 * However we set state to SYN-SENT and not releasing socket
217	 * lock select source port, enter ourselves into the hash tables and
218	 * complete initialization after this.
219	 */
220	tcp_set_state(sk, TCP_SYN_SENT);
...
227	rt = ip_route_newports(fl4, rt, orig_sport, orig_dport,
228			       inet->inet_sport, inet->inet_dport, sk);
...
246	err = tcp_connect(sk);
...
264}
265EXPORT_SYMBOL(tcp_v4_connect);
```
[tcp_connect函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/tcp_output.c#3091)具体负责构造一个携带SYN标志位的TCP头并发送出去，同时还设置了计时器超时重发。
```
3090/* Build a SYN and send it off. */
3091int tcp_connect(struct sock *sk)
3092{
...
3111	tcp_init_nondata_skb(buff, tp->write_seq++, TCPHDR_SYN);
3112	tp->retrans_stamp = tcp_time_stamp;
3113	tcp_connect_queue_skb(sk, buff);
3114	tcp_ecn_send_syn(sk, buff);
3115
3116	/* Send off SYN; include data in Fast Open. */
3117	err = tp->fastopen_req ? tcp_send_syn_data(sk, buff) :
3118	      tcp_transmit_skb(sk, buff, 1, sk->sk_allocation);
3119	if (err == -ECONNREFUSED)
3120		return err;
3121
3122	/* We change tp->snd_nxt after the tcp_transmit_skb() call
3123	 * in order to make this packet get counted in tcpOutSegs.
3124	 */
3125	tp->snd_nxt = tp->write_seq;
3126	tp->pushed_seq = tp->write_seq;
3127	TCP_INC_STATS(sock_net(sk), TCP_MIB_ACTIVEOPENS);
3128
3129	/* Timer for repeating the SYN until an answer. */
3130	inet_csk_reset_xmit_timer(sk, ICSK_TIME_RETRANS,
3131				  inet_csk(sk)->icsk_rto, TCP_RTO_MAX);
3132	return 0;
3133}
```
其中tcp_transmit_skb函数负责将tcp数据发送出去，这里调用了icsk->icsk_af_ops->queue_xmit函数指针，实际上就是在TCP/IP协议栈初始化时设定好的IP层向上提供数据发送接口[ip_queue_xmit函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/ip_output.c#363)，这里TCP协议栈通过调用这个icsk->icsk_af_ops->queue_xmit函数指针来触发IP协议栈代码发送数据，感兴趣的读者可以查找queue_xmit函数指针是如何初始化为ip_queue_xmit函数的。具体ip_queue_xmit函数内部的实现我们在后续内容中会专题研究，本文聚焦在TCP协议的三次握手。
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
1012	err = icsk->icsk_af_ops->queue_xmit(sk, skb, &inet->cork.fl);
...
1020}
```

### inet_csk_accept函数

另一头服务端调用[inet_csk_accept函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/inet_connection_sock.c#292)会请求队列中取出一个连接请求，如果队列为空则通过inet_csk_wait_for_connect阻塞住等待客户端的连接。
```
289/*
290 * This will accept the next outstanding connection.
291 */
292struct sock *inet_csk_accept(struct sock *sk, int flags, int *err)
293{
294	struct inet_connection_sock *icsk = inet_csk(sk);
295	struct request_sock_queue *queue = &icsk->icsk_accept_queue;
296	struct sock *newsk;
297	struct request_sock *req;
298	int error;
299
300	lock_sock(sk);
301
302	/* We need to make sure that this socket is listening,
303	 * and that it has something pending.
304	 */
305	error = -EINVAL;
306	if (sk->sk_state != TCP_LISTEN)
307		goto out_err;
308
309	/* Find already established connection */
310	if (reqsk_queue_empty(queue)) {
311		long timeo = sock_rcvtimeo(sk, flags & O_NONBLOCK);
...
318		error = inet_csk_wait_for_connect(sk, timeo);
319		if (error)
320			goto out_err;
321	}
322	req = reqsk_queue_remove(queue);
323	newsk = req->sk;
324
325	sk_acceptq_removed(sk);
326	if (sk->sk_protocol == IPPROTO_TCP && queue->fastopenq != NULL) {
327		spin_lock_bh(&queue->fastopenq->lock);
328		if (tcp_rsk(req)->listener) {
329			/* We are still waiting for the final ACK from 3WHS
330			 * so can't free req now. Instead, we set req->sk to
331			 * NULL to signify that the child socket is taken
332			 * so reqsk_fastopen_remove() will free the req
333			 * when 3WHS finishes (or is aborted).
334			 */
335			req->sk = NULL;
336			req = NULL;
337		}
...
344	return newsk;
...
350}
```
[inet_csk_wait_for_connect函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/inet_connection_sock.c#241)就是无限for循环，一旦有连接请求进来则跳出循环。
```
241/*
242 * Wait for an incoming connection, avoid race conditions. This must be called
243 * with the socket locked.
244 */
245static int inet_csk_wait_for_connect(struct sock *sk, long timeo)
246{
247	struct inet_connection_sock *icsk = inet_csk(sk);
248	DEFINE_WAIT(wait);
249	int err;
250
251	/*
252	 * True wake-one mechanism for incoming connections: only
253	 * one process gets woken up, not the 'whole herd'.
254	 * Since we do not 'race & poll' for established sockets
255	 * anymore, the common case will execute the loop only once.
256	 *
257	 * Subtle issue: "add_wait_queue_exclusive()" will be added
258	 * after any current non-exclusive waiters, and we know that
259	 * it will always _stay_ after any new non-exclusive waiters
260	 * because all non-exclusive waiters are added at the
261	 * beginning of the wait-queue. As such, it's ok to "drop"
262	 * our exclusiveness temporarily when we get woken up without
263	 * having to remove and re-insert us on the wait queue.
264	 */
265	for (;;) {
266		prepare_to_wait_exclusive(sk_sleep(sk), &wait,
267					  TASK_INTERRUPTIBLE);
268		release_sock(sk);
269		if (reqsk_queue_empty(&icsk->icsk_accept_queue))
270			timeo = schedule_timeout(timeo);
271		lock_sock(sk);
272		err = 0;
273		if (!reqsk_queue_empty(&icsk->icsk_accept_queue))
274			break;
275		err = -EINVAL;
276		if (sk->sk_state != TCP_LISTEN)
277			break;
278		err = sock_intr_errno(timeo);
279		if (signal_pending(current))
280			break;
281		err = -EAGAIN;
282		if (!timeo)
283			break;
284	}
285	finish_wait(sk_sleep(sk), &wait);
286	return err;
287}
288
```
如果读者按如上思路跟踪调试代码的话，会发现connect之后将连接请求发送出去，accept等待连接请求，connect启动到返回和accept返回之间就是所谓三次握手的时间。

### 三次握手中携带SYN/ACK的TCP头数据的发送和接收

以上的分析我们都是按照用户程序调用socket接口、通过系统调用陷入内核进入内核态的socket接口层代码，然后根据创建socket时指定协议选择适当的函数指针，比如sock->opt->connect和sock->opt->accept两个函数指针，从而进入协议处理代码中，本专栏是以TCP/IPv4为例（net/ipv4目录下），则是分别进入tcp_v4_connect函数和inet_csk_accept函数中。总之，主要的思路是call-in的方式逐级进行函数调用，但是接收数据放入accept队列的代码我们还没有跟踪到，接下来我们需要换一个思路，网卡接收到数据需要通知上层协议来接收并处理数据，那么应该有TCP协议的接收数据的函数被底层网络驱动callback方式进行调用，针对这个思路我们需要回过头来看TCP/IP协议栈的初始化过程中是不是有将recv的函数指针发布给网络底层代码。


TCP/IP协议栈的初始化过程是在[inet_init函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/af_inet.c#1674)，其中有段代码中提到的tcp_protocol结构体变量很像：
```
1498static const struct net_protocol tcp_protocol = {
1499	.early_demux	=	tcp_v4_early_demux,
1500	.handler	=	tcp_v4_rcv,
1501	.err_handler	=	tcp_v4_err,
1502	.no_policy	=	1,
1503	.netns_ok	=	1,
1504	.icmp_strict_tag_validation = 1,
1505};
...
1708	/*
1709	 *	Add all the base protocols.
1710	 */
1711
1712	if (inet_add_protocol(&icmp_protocol, IPPROTO_ICMP) < 0)
1713		pr_crit("%s: Cannot add ICMP protocol\n", __func__);
1714	if (inet_add_protocol(&udp_protocol, IPPROTO_UDP) < 0)
1715		pr_crit("%s: Cannot add UDP protocol\n", __func__);
1716	if (inet_add_protocol(&tcp_protocol, IPPROTO_TCP) < 0)
1717		pr_crit("%s: Cannot add TCP protocol\n", __func__);
```
其中的handler被赋值为tcp_v4_rcv，符合底层更一般化上层更具体化的协议设计的一般规律，暂时我们聚焦在TCP协议不准备深入到网络底层代码，但我们可以想象底层网络代码接到数据需要找到合适的处理数据的上层代码来负责处理，那么用handler函数指针来处理就很符合代码逻辑。到这里我们就找到TCP协议中负责接收处理数据的入口，接收TCP连接请求及进行三次握手处理过程应该也是在这里为起点，那么从tcp_v4_rcv应该能够找到对SYN/ACK标志的处理（三次握手），连接请求建立后并将连接放入accept的等待队列。

接下来读者可以进一步深入到三次握手过程中携带SYN/ACK标志的数据收发过程（tcp_transmit_skb和tcp_v4_rcv）以及连接建立成功后放到accpet队列的代码，乃至正常数据的收发过程和关闭连接的过程，这些都需要深入理解TCP协议标准的细节才能读懂代码，读者可以可以对照[TCP协议标准](https://www.ietf.org/rfc/rfc793.txt)深入理解代码。


* [tcp_v4_rcv](https://github.com/torvalds/linux/blob/386403a115f95997c2715691226e11a7b5cffcfd/net/ipv4/tcp_ipv4.c#L1806)
```
int tcp_v4_rcv(struct sk_buff *skb)
{
  ...
  th = (const struct tcphdr *)skb->data;
	iph = ip_hdr(skb);
lookup:
	sk = __inet_lookup_skb(&tcp_hashinfo, skb, __tcp_hdrlen(th), th->source,
			       th->dest, sdif, &refcounted);
  ...
process:
	if (sk->sk_state == TCP_TIME_WAIT)
		goto do_time_wait;

	if (sk->sk_state == TCP_NEW_SYN_RECV) {
    ...
  }
  ...
	if (sk->sk_state == TCP_LISTEN) {
		ret = tcp_v4_do_rcv(sk, skb);
		goto put_and_return;
	}
  ...
	if (!sock_owned_by_user(sk)) {
		skb_to_free = sk->sk_rx_skb_cache;
		sk->sk_rx_skb_cache = NULL;
		ret = tcp_v4_do_rcv(sk, skb);
	} else {
		if (tcp_add_backlog(sk, skb))
			goto discard_and_relse;
		skb_to_free = NULL;
	}
  ...
do_time_wait:
	if (!xfrm4_policy_check(NULL, XFRM_POLICY_IN, skb)) {
		inet_twsk_put(inet_twsk(sk));
		goto discard_it;
	}

	tcp_v4_fill_cb(skb, iph, th);

	if (tcp_checksum_complete(skb)) {
		inet_twsk_put(inet_twsk(sk));
		goto csum_error;
	}
	switch (tcp_timewait_state_process(inet_twsk(sk), skb, th)) {
	case TCP_TW_SYN: {
		struct sock *sk2 = inet_lookup_listener(dev_net(skb->dev),
							&tcp_hashinfo, skb,
							__tcp_hdrlen(th),
							iph->saddr, th->source,
							iph->daddr, th->dest,
							inet_iif(skb),
							sdif);
		if (sk2) {
			inet_twsk_deschedule_put(inet_twsk(sk));
			sk = sk2;
			tcp_v4_restore_cb(skb);
			refcounted = false;
			goto process;
		}
	}
		/* to ACK */
		/* fall through */
	case TCP_TW_ACK:
		tcp_v4_timewait_ack(sk, skb);
		break;
	case TCP_TW_RST:
		tcp_v4_send_reset(sk, skb);
		inet_twsk_deschedule_put(inet_twsk(sk));
		goto discard_it;
	case TCP_TW_SUCCESS:;
	}
	goto discard_it;
}
```
* [tcp_rcv_state_process](https://github.com/torvalds/linux/blob/386403a115f95997c2715691226e11a7b5cffcfd/net/ipv4/tcp_input.c#L6129)
```
/*
 *	This function implements the receiving procedure of RFC 793 for
 *	all states except ESTABLISHED and TIME_WAIT.
 *	It's called from both tcp_v4_rcv and tcp_v6_rcv and should be
 *	address independent.
 */

int tcp_rcv_state_process(struct sock *sk, struct sk_buff *skb)
{
	...
	switch (sk->sk_state) {
	...
	case TCP_LISTEN:
		if (th->ack)
			return 1;

		if (th->rst)
			goto discard;

		if (th->syn) {
			if (th->fin)
				goto discard;
			/* It is possible that we process SYN packets from backlog,
			 * so we need to make sure to disable BH and RCU right there.
			 */
			rcu_read_lock();
			local_bh_disable();
			acceptable = icsk->icsk_af_ops->conn_request(sk, skb) >= 0;
			local_bh_enable();
			rcu_read_unlock();

			if (!acceptable)
				return 1;
			consume_skb(skb);
			return 0;
		}
		goto discard;

	case TCP_SYN_SENT:
		tp->rx_opt.saw_tstamp = 0;
		tcp_mstamp_refresh(tp);
		queued = tcp_rcv_synsent_state_process(sk, skb, th);
		if (queued >= 0)
			return queued;

		/* Do step6 onward by hand. */
		tcp_urg(sk, skb, th);
		__kfree_skb(skb);
		tcp_data_snd_check(sk);
		return 0;
	}
	...
	switch (sk->sk_state) {
	case TCP_SYN_RECV:
		tp->delivered++; /* SYN-ACK delivery isn't tracked in tcp_ack */
		if (!tp->srtt_us)
			tcp_synack_rtt_meas(sk, req);

		if (req) {
			tcp_rcv_synrecv_state_fastopen(sk);
		} else {
			tcp_try_undo_spurious_syn(sk);
			tp->retrans_stamp = 0;
			tcp_init_transfer(sk, BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB);
			WRITE_ONCE(tp->copied_seq, tp->rcv_nxt);
		}
		smp_mb();
		tcp_set_state(sk, TCP_ESTABLISHED);
		sk->sk_state_change(sk);

		/* Note, that this wakeup is only for marginal crossed SYN case.
		 * Passively open sockets are not waked up, because
		 * sk->sk_sleep == NULL and sk->sk_socket == NULL.
		 */
		if (sk->sk_socket)
			sk_wake_async(sk, SOCK_WAKE_IO, POLL_OUT);

		tp->snd_una = TCP_SKB_CB(skb)->ack_seq;
		tp->snd_wnd = ntohs(th->window) << tp->rx_opt.snd_wscale;
		tcp_init_wl(tp, TCP_SKB_CB(skb)->seq);

		if (tp->rx_opt.tstamp_ok)
			tp->advmss -= TCPOLEN_TSTAMP_ALIGNED;

		if (!inet_csk(sk)->icsk_ca_ops->cong_control)
			tcp_update_pacing_rate(sk);

		/* Prevent spurious tcp_cwnd_restart() on first data packet */
		tp->lsndtime = tcp_jiffies32;

		tcp_initialize_rcv_mss(sk);
		tcp_fast_path_on(tp);
		break;
		...
```
其中TCP_LISTEN状态时接到SYN就可以通过acceptable = icsk->icsk_af_ops->conn_request(sk, skb) >= 0;将连接加入accpet队列

* [ipv4_specific](https://github.com/torvalds/linux/blob/386403a115f95997c2715691226e11a7b5cffcfd/net/ipv4/tcp_ipv4.c#L2050)
```
const struct inet_connection_sock_af_ops ipv4_specific = {
	.queue_xmit	   = ip_queue_xmit,
	.send_check	   = tcp_v4_send_check,
	.rebuild_header	   = inet_sk_rebuild_header,
	.sk_rx_dst_set	   = inet_sk_rx_dst_set,
	.conn_request	   = tcp_v4_conn_request,
	.syn_recv_sock	   = tcp_v4_syn_recv_sock,
	.net_header_len	   = sizeof(struct iphdr),
	.setsockopt	   = ip_setsockopt,
	.getsockopt	   = ip_getsockopt,
	.addr2sockaddr	   = inet_csk_addr2sockaddr,
	.sockaddr_len	   = sizeof(struct sockaddr_in),
#ifdef CONFIG_COMPAT
	.compat_setsockopt = compat_ip_setsockopt,
	.compat_getsockopt = compat_ip_getsockopt,
#endif
	.mtu_reduced	   = tcp_v4_mtu_reduced,
};
EXPORT_SYMBOL(ipv4_specific);
```
* [tcp_v4_init_sock](https://github.com/torvalds/linux/blob/386403a115f95997c2715691226e11a7b5cffcfd/net/ipv4/tcp_ipv4.c#L2082)
```
/* NOTE: A lot of things set to zero explicitly by call to
 *       sk_alloc() so need not be done here.
 */
static int tcp_v4_init_sock(struct sock *sk)
{
	struct inet_connection_sock *icsk = inet_csk(sk);

	tcp_init_sock(sk);

	icsk->icsk_af_ops = &ipv4_specific;

#ifdef CONFIG_TCP_MD5SIG
	tcp_sk(sk)->af_specific = &tcp_sock_ipv4_specific;
#endif

	return 0;
}
```
* [tcp_v4_conn_request](https://github.com/torvalds/linux/blob/386403a115f95997c2715691226e11a7b5cffcfd/net/ipv4/tcp_ipv4.c#L1391)
```
int tcp_v4_conn_request(struct sock *sk, struct sk_buff *skb)
{
	/* Never answer to SYNs send to broadcast or multicast */
	if (skb_rtable(skb)->rt_flags & (RTCF_BROADCAST | RTCF_MULTICAST))
		goto drop;

	return tcp_conn_request(&tcp_request_sock_ops,
				&tcp_request_sock_ipv4_ops, sk, skb);

drop:
	tcp_listendrop(sk);
	return 0;
}
EXPORT_SYMBOL(tcp_v4_conn_request);
```
* [tcp_conn_request](https://github.com/torvalds/linux/blob/386403a115f95997c2715691226e11a7b5cffcfd/net/ipv4/tcp_input.c#L6552)
```
int tcp_conn_request(struct request_sock_ops *rsk_ops,
		     const struct tcp_request_sock_ops *af_ops,
		     struct sock *sk, struct sk_buff *skb)
{
	...
	if (fastopen_sk) {
		af_ops->send_synack(fastopen_sk, dst, &fl, req,
				    &foc, TCP_SYNACK_FASTOPEN);
		/* Add the child socket directly into the accept queue */
		if (!inet_csk_reqsk_queue_add(sk, req, fastopen_sk)) {
			reqsk_fastopen_remove(fastopen_sk, req, false);
			bh_unlock_sock(fastopen_sk);
			sock_put(fastopen_sk);
			goto drop_and_free;
		}
		sk->sk_data_ready(sk);
		bh_unlock_sock(fastopen_sk);
		sock_put(fastopen_sk);
	} else {
		tcp_rsk(req)->tfo_listener = false;
		if (!want_cookie)
			inet_csk_reqsk_queue_hash_add(sk, req,
				tcp_timeout_init((struct sock *)req));
		af_ops->send_synack(sk, dst, &fl, req, &foc,
				    !want_cookie ? TCP_SYNACK_NORMAL :
						   TCP_SYNACK_COOKIE);
		if (want_cookie) {
			reqsk_free(req);
			return 0;
		}
	}
	...
```
* [inet_csk_reqsk_queue_hash_add](https://github.com/torvalds/linux/blob/386403a115f95997c2715691226e11a7b5cffcfd/net/ipv4/inet_connection_sock.c#L765)和[inet_csk_reqsk_queue_added](https://github.com/torvalds/linux/blob/81160dda9a7aad13c04e78bb2cfd3c4630e3afab/include/net/inet_connection_sock.h#L272)
```
void inet_csk_reqsk_queue_hash_add(struct sock *sk, struct request_sock *req,
				   unsigned long timeout)
{
	reqsk_queue_hash_req(req, timeout);
	inet_csk_reqsk_queue_added(sk);
}
```
```
static inline void inet_csk_reqsk_queue_added(struct sock *sk)
{
	reqsk_queue_added(&inet_csk(sk)->icsk_accept_queue);
}
```
