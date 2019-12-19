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
