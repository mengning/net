# Linux内核初始化过程中加载TCP/IP协议栈

Linux内核初始化过程中加载TCP/IP协议栈，从start_kernel、kernel_init、do_initcalls、inet_init，找出Linux内核初始化TCP/IP的入口位置，即为inet_init函数。

## Linux内核启动过程

之前的实验中我们设置了断点start_kernel，start_kernel即是Linux内核的起点，相当于我们普通C程序的main函数，我们知道C语言代码从main函数开启动，C程序的阅读也从main函数开始。这个start_kernel也是整个Linux内核启动的起点，我们可以在内核代码路面init/main.c中找到`start_kernel`函数，这个地方就是初始化Linux内核启动的起点。


我们知道如何跟踪内核代码运行过程的话，我们应该有目的的来跟踪它。我们来跟踪内核启动过程，并重点找出初始化TCP/IP协议栈的位置。

首先我们找到内核启动的起点`start_kernel`函数所在的main.c，我们简单浏览一下`start_kernel`函数，这里有很多其他的模块初始化工作，因为这里边每一个启动的点都涉及到比较复杂的模块，因为内核非常庞大，包括很多的模块，当然如果你研究内核的某个模块的话，往往都需要了解main.c中`start_kernel`这一块，因为内核的主要模块的初始化工作，都是直接或间接从`start_kernel`函数里开始调用的。涉及到的模块太多太复杂，那我们只看我们需要了解的东西，这里边有很多setup设置的东西，这里边有一个`trap_init`函数调用，涉及到一些初始化中断向量，可以看到它在`set_intr_gate`设置到很多的中断门，很多的硬件中断，其中有一个系统陷阱门，进行系统调用的。其他还有`mm_init`内存管理模块的初始化等等。`start_kernel`中的最后一句为`rest_init`，这个比较有意思。内核启动完了之后，有一个`call_cpu_idle`，当系统没有进程需要执行时就调用idle进程。`rest_init`是0号进程，它创建了1号进程init和其他的一些服务进程。这就是内核的启动过程，我们先简单这样看，然后可以在重点找出网络初始化以及初始化TCP/IP协议栈的位置。下面我们再分析一下关键的函数。

### `start_kernel()` 

main.c 中没有 main 函数，`start_kernel()` 相当于main函数。`start_kernel`是一切的起点，在此函数被调用之前内核代码主要是用汇编语言写的，完成硬件系统的初始化工作，为C代码的运行设置环境。由调试可得`start_kernel`在[/linux-3.18.6/init/main.c#500](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/init/main.c#500)：

```
500asmlinkage __visible void __init start_kernel(void)
501{
...
679	/* Do the rest non-__init'ed, we're now alive */
680	rest_init();
681}
```

### `rest_init()`函数


rest_init在[linux-3.18.6/init/main.c#393](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/init/main.c#393)的位置：
```
393static noinline void __init_refok rest_init(void)
394{
395	int pid;
396
397	rcu_scheduler_starting();
398	/*
399	 * We need to spawn init first so that it obtains pid 1, however
400	 * the init task will end up wanting to create kthreads, which, if
401	 * we schedule it before we create kthreadd, will OOPS.
402	 */
403	kernel_thread(kernel_init, NULL, CLONE_FS);
404	numa_default_policy();
405	pid = kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES);
406	rcu_read_lock();
407	kthreadd_task = find_task_by_pid_ns(pid, &init_pid_ns);
408	rcu_read_unlock();
409	complete(&kthreadd_done);
410
411	/*
412	 * The boot idle thread must execute schedule()
413	 * at least once to get things moving:
414	 */
415	init_idle_bootup_task(current);
416	schedule_preempt_disabled();
417	/* Call into cpu_idle with preempt disabled */
418	cpu_startup_entry(CPUHP_ONLINE);
419}
```

通过`rest_init()`新建`kernel_init`、`kthreadd`内核线程。403行代码 ```kernel_thread(kernel_init, NULL, CLONE_FS);```，由注释得调用 `kernel_thread()`创建1号内核线程（在`kernel_init`函数正式启动），`kernel_init`函数启动了init用户程序。

另外405行代码 ```pid = kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES);``` 调用`kernel_thread`执行`kthreadd`，创建PID为2的内核线程。

`rest_init()`最后调用`cpu_idle()` 演变成了idle进程。

##  Linux内核是如何加载TCP/IP协议栈的？

###  `kernel_init`函数和do_basic_setup函数

 `kernel_init`函数的主要工作是夹在init用户程序，但是在加载init用户程序前通过kernel_init_freeable函数进一步做了一些初始化的工作。[`kernel_init`函数和kernel_init_freeable函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/init/main.c#934):

```
930static int __ref kernel_init(void *unused)
931{
932	int ret;
933
934	kernel_init_freeable();
935	/* need to finish all async __init code before freeing the memory */
936	async_synchronize_full();
937	free_initmem();
938	mark_rodata_ro();
939	system_state = SYSTEM_RUNNING;
940	numa_default_policy();
941
942	flush_delayed_fput();
943
944	if (ramdisk_execute_command) {
945		ret = run_init_process(ramdisk_execute_command);
946		if (!ret)
947			return 0;
948		pr_err("Failed to execute %s (error %d)\n",
949		       ramdisk_execute_command, ret);
950	}
951
952	/*
953	 * We try each of these until one succeeds.
954	 *
955	 * The Bourne shell can be used instead of init if we are
956	 * trying to recover a really broken machine.
957	 */
958	if (execute_command) {
959		ret = run_init_process(execute_command);
960		if (!ret)
961			return 0;
962		pr_err("Failed to execute %s (error %d).  Attempting defaults...\n",
963			execute_command, ret);
964	}
965	if (!try_to_run_init_process("/sbin/init") ||
966	    !try_to_run_init_process("/etc/init") ||
967	    !try_to_run_init_process("/bin/init") ||
968	    !try_to_run_init_process("/bin/sh"))
969		return 0;
970
971	panic("No working init found.  Try passing init= option to kernel. "
972	      "See Linux Documentation/init.txt for guidance.");
973}
974
975static noinline void __init kernel_init_freeable(void)
976{
977	/*
978	 * Wait until kthreadd is all set-up.
979	 */
980	wait_for_completion(&kthreadd_done);
981
...
1004	do_basic_setup();
1005
...
1033}
1034
```

kernel_init_freeable函数做的一些初始化的工作与我们网络初始化有关的主要在[do_basic_setup函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/init/main.c#867)中，其中do_initcalls用一种巧妙的方式对一些子系统进行了初始化，其中包括TCP/IP网络协议栈的初始化。
```
867/*
868 * Ok, the machine is now initialized. None of the devices
869 * have been touched yet, but the CPU subsystem is up and
870 * running, and memory and process management works.
871 *
872 * Now we can finally start doing some real work..
873 */
874static void __init do_basic_setup(void)
875{
876	cpuset_init_smp();
877	usermodehelper_init();
878	shmem_init();
879	driver_init();
880	init_irq_proc();
881	do_ctors();
882	usermodehelper_enable();
883	do_initcalls();
884	random_int_secret_init();
885}
```

### do_initcalls函数巧妙地对网络协议进行初始化

[do_initcalls函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/init/main.c#859)是table驱动的，维护了一个initcalls的table，从而可以对每一个注册进来的初始化项目进行初始化，这个巧妙的机制可以理解成观察者模式，每一个协议子系统是一个观察者，将它的初始化入口注册进来，do_initcalls函数是被观察者负责统一调用每一个子系统的初始化函数指针。
```
859static void __init do_initcalls(void)
860{
861	int level;
862
863	for (level = 0; level < ARRAY_SIZE(initcall_levels) - 1; level++)
864		do_initcall_level(level);
865}
```
以TCP/IP协议栈为例，[inet_init函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/af_inet.c#1674)是TCP/IP协议栈初始化的入口函数，通过fs_initcall(inet_init)将inet_init函数注册进initcalls的table。
```
1674static int __init inet_init(void)
1675{
...
1795}
1796
1797fs_initcall(inet_init);
```
这里do_initcalls的注册和调用机制是通过复杂的宏来实现的，代码读起来非常晦涩，这里我们换一种方法通过跟踪代码运行过程来验证它。

我们首先将端点设在kernel_init、do_initcalls、inet_init以及do_initcalls后面的random_int_secret_init，预期这四个断点会依次触发，从而可以间接验证fs_initcall(inet_init)确实将inet_init注册进了do_initcalls并被do_initcalls调用执行了。

在lab3目录下执行qemu -kernel ../../linux-3.18.6/arch/x86/boot/bzImage -initrd ../rootfs.img -s -S
```
shiyanlou:~/ $ cd LinuxKernel                                        [14:08:18]
shiyanlou:LinuxKernel/ $ git clone https://github.com/mengning/linuxnet.git
\u6b63\u514b\u9686\u5230 'linuxnet'...
remote: Enumerating objects: 175, done.
remote: Counting objects: 100% (175/175), done.
remote: Compressing objects: 100% (151/151), done.
remote: Total 175 (delta 100), reused 47 (delta 21), pack-reused 0
\u63a5\u6536\u5bf9\u8c61\u4e2d: 100% (175/175), 4.57 MiB | 2.58 MiB/s, done.
\u5904\u7406 delta \u4e2d: 100% (100/100), done.
\u68c0\u67e5\u8fde\u63a5... \u5b8c\u6210\u3002
shiyanlou:LinuxKernel/ $ cd linuxnet/lab3                            [14:08:38]
shiyanlou:lab3/ (master) $ make rootfs                               [14:08:38]
gcc -o init linktable.c menu.c main.c -m32 -static -lpthread
find init | cpio -o -Hnewc |gzip -9 > ../rootfs.img
1889 \u5757
qemu -kernel ../../linux-3.18.6/arch/x86/boot/bzImage -initrd ../rootfs.img
shiyanlou:lab3/ (master*) $ qemu -kernel ../../linux-3.18.6/arch/x86/boot/bzImage -initrd ../rootfs.img -s -S
```
在另一个窗口执行gdb并依次执行如下gdb命令：
```
(gdb) file ../../linux-3.18.6/vmlinux
Reading symbols from ../../linux-3.18.6/vmlinux...done.
(gdb) target remote:1234
Remote debugging using :1234
0x0000fff0 in ?? ()
(gdb) b kernel_init
Breakpoint 1 at 0xc1740240: file init/main.c, line 931.
(gdb) b do_initcalls
Breakpoint 2 at 0xc1a2fc2f: file init/main.c, line 851.
(gdb) b inet_init
Breakpoint 3 at 0xc1a76de3: file net/ipv4/af_inet.c, line 1675.
(gdb) b random_int_secret_init
Breakpoint 4 at 0xc132dbf0: file drivers/char/random.c, line 1712.
```
这样我们就设置好了验证的系统环境，如图：
![](http://i2.51cto.com/images/blog/201812/04/fbc983529bb40b68d56968650a9ad166.png?x-oss-process=image/watermark,size_16,text_QDUxQ1RP5Y2a5a6i,color_FFFFFF,t_100,g_se,x_10,y_10,shadow_90,type_ZmFuZ3poZW5naGVpdGk=)

依次按c让Linux内核从断点处继续执行，可以看到Linux内核依次断点在kernel_init、do_initcalls、inet_init以及do_initcalls后面的random_int_secret_init，如下输出信息与我们的预期是一致的，fs_initcall(inet_init)确实将inet_init注册进了do_initcalls并被do_initcalls调用执行了。
```
(gdb) c
Continuing.

Breakpoint 1, kernel_init (unused=0x0) at init/main.c:931
931	{
(gdb) c
Continuing.

Breakpoint 2, kernel_init_freeable () at init/main.c:1004
1004		do_basic_setup();
(gdb) c
Continuing.

Breakpoint 3, inet_init () at net/ipv4/af_inet.c:1675
1675	{
(gdb) c
Continuing.

Breakpoint 4, random_int_secret_init () at drivers/char/random.c:1712
1712	{
(gdb) 

```
到这里我们就找到了Linux内核初始化TCP/IP的入口位置，即为[inet_init函数](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/net/ipv4/af_inet.c#1674)。
