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



