# 构建调试Linux内核网络代码的环境MenuOS系统

您可以自定搭建环境，下文将基于Ubuntu 18.04 & linux-5.0.1提供简要指南，以便您能自行构建调试Linux内核网络代码的环境MenuOS系统。您也可以选择使用已经构建好的在线实验环境：[实验楼虚拟机https://www.shiyanlou.com/courses/1198](https://www.shiyanlou.com/courses/1198)，只是在线环境构建的比较早，是基于[linux-3.18.6](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/)内核的。

## 编译运行Linux内核

您需要有一台带图形界面的Ubuntu Linux桌面主机，为了方便起见一般通过VMware Workstation或者Virtualbox安装Ubuntu Linux虚拟机。以下命令将以64位X86 CPU且带图形界面的Ubuntu Linux桌面主机环境为例。

```
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.0.1.tar.xz
xz -d linux-5.0.1.tar.xz
tar -xvf linux-5.0.1.tar
cd linux-5.0.1
sudo apt install build-essential flex bison libssl-dev libelf-dev libncurses-dev
make defconfig
make menuconfig # Kernel hacking—>Compile-time checks and compiler options  ---> [*] Compile the kernel with debug info 
make # 或者使用make -j4，其中4为多核CPU核心数，可以加快编译过程
sudo apt install qemu
qemu-system-x86_64 -kernel linux-5.0.1/arch/x86/boot/bzImage
```
## 构建跟文件系统并运行调试Linux内核

光有Linux内核还不是一个完整的Linux系统，还需要有根文件系统及用户态的程序，用户态至少要有一个init可执行程序。
```
dd if=/dev/zero of=rootfs.img bs=1M count=128
mkfs.ext4 rootfs.img
mkdir rootfs
sudo mount -o loop rootfs.img rootfs
git clone https://github.com/mengning/net.git
cd lab3
make
cp init rootfs/
sudo umount rootfs
# KASLR是kernel address space layout randomization的缩写
qemu-system-x86_64 -kernel linux-5.0.1/arch/x86_64/boot/bzImage -hda rootfs.img -append "root=/dev/sda init=/init nokaslr" -s -S
# 另一个shell窗口
gdb
file linux-5.0.1/vmlinux
target remote:1234 #则可以建立gdb和gdbserver之间的连接
break start_kernel
按c 让qemu上的Linux继续运行
```
## 使用实验楼在线环境运行MenuOS系统

实验楼在线环境中已经在LinuxKernel目录下构建好了基于Linux-3.18.6的内核环境，可以使用实验楼的虚拟机打开Xfce终端（Terminal）, 运行MenuOS系统。

```
shiyanlou:~/ $ cd LinuxKernel/
shiyanlou:LinuxKernel/ $ qemu -kernel linux-3.18.6/arch/x86/boot/bzImage -initrd rootfs.img
```

![](http://i2.51cto.com/images/blog/201811/05/3c8b6968d7384d176a67f35765e371ea.png?x-oss-process=image/watermark,size_16,text_QDUxQ1RP5Y2a5a6i,color_FFFFFF,t_100,g_se,x_10,y_10,shadow_90,type_ZmFuZ3poZW5naGVpdGk=)

内核启动完成后进入[menu](https://github.com/mengning/menu)程序，支持三个命令help、version和quit，您也可以添加更多的命令。

# 跟踪分析Linux内核的启动过程的具体操作方法

使用gdb跟踪调试内核首先添加-s和-S选项启动MenuOS系
    
```
qemu -kernel linux-3.18.6/arch/x86/boot/bzImage -initrd rootfs.img -s -S
```

关于-s和-S选项的说明：
> -S freeze CPU at startup (use ’c’ to start execution)
> -s shorthand for -gdb tcp::1234 若不想使用1234端口，则可以使用-gdb tcp:xxxx来取代-s选项

右击水平分割或者另外打开一个Xfce终端（Terminal），执行gdb

```
gdb
    （gdb）file linux-3.18.6/vmlinux # 在gdb界面中targe remote之前加载符号表
    （gdb）target remote:1234 # 建立gdb和gdbserver之间的连接
    （gdb）break start_kernel # 断点的设置可以在target remote之前，也可以在之后
    （gdb）c                  # 按c 让qemu上的Linux继续运行
```
注意：按Ctrl+Alt从QEMU窗口里的MenuOS系统返回到当前系统，否则会误以为卡死在那里。

## 将网络通信程序的服务端集成到MenuOS系统中

接下来我们需要将C/S方式的网络通信程序的服务端集成到MenuOS系统中,成为MenuOS系统的命令replyhi，实际上我们已经给大家集成好了,我们git clone 克隆一个linuxnet.git；进入lab2目录执行make可以将我们集成好的代码copy到menu项目中。然后进入menu,我们写了一个脚本rootfs,运行make rootfs,脚本就可以帮助我们自动编译、自动生成根文件系统,还会帮我们运行起来MenuOS系统。详细命令如下：

```
cd LinuxKernel  
git clone https://github.com/mengning/linuxnet.git
cd linuxnet/lab2
make
cd ../../menu/
make rootfs
```
执行效果大致如下：
![](http://i2.51cto.com/images/blog/201811/07/cbff044523e47cad65fb363625df0b42.png?x-oss-process=image/watermark,size_16,text_QDUxQ1RP5Y2a5a6i,color_FFFFFF,t_100,g_se,x_10,y_10,shadow_90,type_ZmFuZ3poZW5naGVpdGk=)

其中我们增加了命令replyhi，功能是回复hi的TCP服务,具体是怎么来做这个事情的呢？实现起来还是比较简单的,我们来看linuxnet/lab2/test_reply.c,里面我们从main()开始读发现里面增加了一行代码


```
    MenuConfig("replyhi", "Reply hi TCP Service", StartReplyhi);
```

其中的StartReplyhi代码如下：

```
 int StartReplyhi(int argc, char *argv[])
{
	int pid;
	/* fork another process */
	pid = fork();
	if (pid < 0)
	{
		/* error occurred */
		fprintf(stderr, "Fork Failed!");
		exit(-1);
	}
	else if (pid == 0)
	{
		/*	 child process 	*/
		Replyhi();
		printf("Reply hi TCP Service Started!\n");
	}
	else
	{
		/* 	parent process	 */
		printf("Please input hello...\n");
	}
}
```

显然StartReplyhi代码使用fork创建了一个子进程，子进程调用了Replyhi函数：

```
#include"syswrapper.h"
#define MAX_CONNECT_QUEUE   1024
int Replyhi()
{
	char szBuf[MAX_BUF_LEN] = "\0";
	char szReplyMsg[MAX_BUF_LEN] = "hi\0";
	InitializeService();
	while (1)
	{
		ServiceStart();
		RecvMsg(szBuf);
		SendMsg(szReplyMsg);
		ServiceStop();
	}
	ShutdownService();
	return 0;
}
```

Replyhi函数的内容和之前的C/S网络通信程序的server.c基本一样。
如果还要给MenuOS增加新的命令只需要按这种方式MenuConfig增加一行,再增加对应的函数就可以了。

接下来您就可以参照前面“跟踪分析Linux内核的启动过程的具体操作方法”进行跟踪调试了，只是我们socket接口使用的是系统sys_socketcall，可以将sys_socketcall设为断点跟踪看看。

## 初始化MenuOS系统的网络功能

之前我们已经将TCP网络程序的服务端replyhi集成到MenuOS中了，而且可以正常的启动TCP服务，方便我们跟踪socket、bind、listen、accept几个API接口到内核处理函数，但是我们启动的TCP服务并不能正常对外提供服务，因为MenuOS没有初始化网络设备（包括本地回环loopback设备），因此它无法接收到任何网络请求。
接下来我们需要搞清楚如何激活Linux网络设备，并将MenuOS系统的网络设备用简便的方式配置好，使我们将TCP客户端也集成进去后可以完整的运行TCP网络程序的服务端和客户端程序。

### 如何激活Linux网络设备接口

#### Linux发行版一般在启动过程中自动激活网络设备接口的方式
一般Linux系统中的/etc/network/interfaces文件里会看到大致如下的代码：
```
auto lo 
iface lo inet loopback
auto eth0 
iface eth0 inet static 
auto wlan1 
iface wlan1 inet dhcp 
```
auto lo/eth0/wlan1都是表示系统启动时自动激活该网络设备接口，一般使用ifup来激活网络设备接口。
inet是指定该网络设备接口使用互联网地址，其中loopback表示使用IP地址127.0.0.1(网络地址127.0.0.0/8)，RFC 1700中定义本地回环网络的地址；static表示手工设置地址；dhcp表示通过DHCP协议自动获取地址。

Linux系统的启动脚本一般会通过执行ifconfig来激活网络设备接口，ifconfig内部通过调用socket和ioctl来通知内核给网络设备接口做适当的设置工作，例如在实验楼虚拟机环境下跟踪ifconfig配置IP并激活本地回环lo网络接口如下：
```
shiyanlou:~/ $ sudo strace ifconfig lo 127.0.0.1 up                  [18:04:53]
execve("/sbin/ifconfig", ["ifconfig", "lo", "127.0.0.1", "up"], [/* 30 vars */]) = 0
...
socket(PF_INET, SOCK_DGRAM, IPPROTO_IP) = 4
...
ioctl(4, SIOCSIFADDR, {ifr_name="lo", ???}) = -1 EPERM (Operation not permitted)
...
ioctl(4, SIOCGIFFLAGS, {ifr_name="lo", ifr_flags=IFF_UP|IFF_LOOPBACK|IFF_RUNNING}) = 0
ioctl(4, SIOCSIFFLAGS, {ifr_name="lo", ???}) = -1 EPERM (Operation not permitted)
...
ioctl(4, SIOCGIFFLAGS, {ifr_name="lo", ifr_flags=IFF_UP|IFF_LOOPBACK|IFF_RUNNING}) = 0
ioctl(4, SIOCSIFFLAGS, {ifr_name="lo", ???}) = -1 EPERM (Operation not permitted)
...
shiyanlou:~/ $ 
```

#### 在MenuOS中手工编码激活网络设备接口lo

仿照Linux系统一般在启动过程中自动激活网络设备接口使用的ifconfig程序中调用socket和ioctl接口的方式撰写代码来激活网络接口lo，[手工编码激活网络设备接口lo](https://github.com/mengning/linuxnet/blob/master/lab3/main.c#L188)具体摘录代码如下：

```
    struct sockaddr_in sa;
    struct ifreq ifreqlo;
    int fd;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    strncpy(ifreqlo.ifr_name, "lo",sizeof("lo"));
    memcpy((char *) &ifreqlo.ifr_addr, (char *) &sa, sizeof(struct sockaddr));
    ioctl(fd, SIOCSIFADDR, &ifreqlo);
    ioctl(fd, SIOCGIFFLAGS, &ifreqlo);
    ifreqlo.ifr_flags |= IFF_UP|IFF_LOOPBACK|IFF_RUNNING;
    ioctl(fd, SIOCSIFFLAGS, &ifreqlo);
    close(fd);
```
其中创建了socket描述符fd，指明网络接口lo、地址族AF_INET和IP地址127.0.0.1，通过ioctl利用预定义的指令SIOCSIFADDR（Socket IO Configuration Set IF ADDR）设置网络接口配置信息，然后利用SIOCSIFFLAGS设置IFF_UP|IFF_LOOPBACK|IFF_RUNNING激活该网络接口。

## 将TCP网络通信程序的客户端也集成到MenuOS系统中

接下来我们需要将C/S方式的网络通信程序的客户端也集成到MenuOS系统中,成为MenuOS系统的命令hello。首先增加Hello，可以直接拷贝原来client.c中的代码如下：
```
int Hello(int argc, char *argv[])
{
	char szBuf[MAX_BUF_LEN] = "\0";
	char szMsg[MAX_BUF_LEN] = "hello\0";
	OpenRemoteService();
	SendMsg(szMsg);
	RecvMsg(szBuf);
	CloseRemoteService();
	return 0;
}
```
然后在main函数中和repplyhi方式一样增加一行MenuConfig("hello", "Hello TCP Client", Hello)如下：
```
    MenuConfig("replyhi", "Reply hi TCP Service", StartReplyhi);
    MenuConfig("hello", "Hello TCP Client", Hello);
```
实际上我们已经给大家集成好了,我们git clone 克隆一个linuxnet.git；进入lab3目录执行make可以将我们集成好的代码copy到menu项目中。然后进入menu,我们写了一个脚本rootfs,运行make rootfs,脚本就可以帮助我们自动编译、自动生成根文件系统,还会帮我们运行起来MenuOS系统。详细命令如下：
```
cd LinuxKernel  
git clone https://github.com/mengning/linuxnet.git
cd linuxnet/lab3
make rootfs
```
运行起来的MenuOS中执行help命令可以看到其中不止有replyhi，也有了hello命令，我们可以先执行replyhi，然后执行hello。具体效果如下：

![](http://i2.51cto.com/images/blog/201811/12/f70b42b94f0bb72f74de1acaf51cfcf1.png)

这样我们构建好了带网络功能的MenoOS系统，方便我们后续跟踪Linux内核代码。
