# TCP/IP协议栈的初始化

* TCP/IP协议栈的初始化的函数入口[inet_init](https://github.com/mengning/linux/blob/master/net/ipv4/af_inet.c#L1899)
```
static int __init inet_init(void)
{
	...
	rc = proto_register(&tcp_prot, 1);
	if (rc)
		goto out;

	...

	/*
	 *	Tell SOCKET that we are alive...
	 */

	(void)sock_register(&inet_family_ops);

	...
	/*
	 *	Add all the base protocols.
	 */

	...
	if (inet_add_protocol(&tcp_protocol, IPPROTO_TCP) < 0)
		pr_crit("%s: Cannot add TCP protocol\n", __func__);
	...
	/* Register the socket-side information for inet_create. */
	for (r = &inetsw[0]; r < &inetsw[SOCK_MAX]; ++r)
		INIT_LIST_HEAD(r);

	for (q = inetsw_array; q < &inetsw_array[INETSW_ARRAY_LEN]; ++q)
		inet_register_protosw(q);

	...

	/* Setup TCP slab cache for open requests. */
	tcp_init();

	...

}

fs_initcall(inet_init);
```
接下来我们以TCP协议为例来看TCP/IP协议栈的初始化过程。

### TCP协议的初始化
* [tcp_prot](https://github.com/mengning/linux/blob/master/net/ipv4/tcp_ipv4.c#L2536)
``` 
struct proto tcp_prot = {
	.name			= "TCP",
	.owner			= THIS_MODULE,
	.close			= tcp_close,
	.pre_connect		= tcp_v4_pre_connect,
	.connect		= tcp_v4_connect,
	.disconnect		= tcp_disconnect,
	.accept			= inet_csk_accept,
	.ioctl			= tcp_ioctl,
	.init			= tcp_v4_init_sock,
	.destroy		= tcp_v4_destroy_sock,
	.shutdown		= tcp_shutdown,
	.setsockopt		= tcp_setsockopt,
	.getsockopt		= tcp_getsockopt,
	.keepalive		= tcp_set_keepalive,
	.recvmsg		= tcp_recvmsg,
	.sendmsg		= tcp_sendmsg,
	.sendpage		= tcp_sendpage,
	.backlog_rcv		= tcp_v4_do_rcv,
	.release_cb		= tcp_release_cb,
	...
};
EXPORT_SYMBOL(tcp_prot);
```
* [tcp_protocol](https://github.com/mengning/linux/blob/master/net/ipv4/tcp_ipv4.c#L2536)
```
/* thinking of making this const? Don't.
 * early_demux can change based on sysctl.
 */
static struct net_protocol tcp_protocol = {
	.early_demux	=	tcp_v4_early_demux,
	.early_demux_handler =  tcp_v4_early_demux,
	.handler	=	tcp_v4_rcv,
	.err_handler	=	tcp_v4_err,
	.no_policy	=	1,
	.netns_ok	=	1,
	.icmp_strict_tag_validation = 1,
};
```
* [tcp_init](https://github.com/mengning/linux/blob/master/net/ipv4/tcp.c#L3837)
```
void __init tcp_init(void)
{
	...
	tcp_v4_init();
	tcp_metrics_init();
	BUG_ON(tcp_register_congestion_control(&tcp_reno) != 0);
	tcp_tasklet_init();
}
```
