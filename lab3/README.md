## 编译构建调试Linux系统

自定搭建环境Based on Ubuntu 18.04 & linux-5.0.1，或使用在线环境：[实验楼虚拟机](https://www.shiyanlou.com/courses/1198)&[linux-3.18.6](http://codelab.shiyanlou.com/source/xref/linux-3.18.6/)


```
https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.0.1.tar.xz
xz -d linux-5.0.1.tar.xz
tar -xvf linux-5.0.1.tar
cd linux-5.0.1
sudo apt install build-essential flex bison libssl-dev libelf-dev libncurses-dev
make defconfig
make menuconfig # Kernel hacking—>Compile-time checks and compiler options  ---> [*] Compile the kernel with debug info 
make 或 make -j*       # *为cpu核心数
sudo apt install qemu
qemu-system-x86_64 -kernel linux-5.0.1/arch/x86/boot/bzImage
dd if=/dev/zero of=rootfs.img bs=4096 count=1024
mkfs.ext4 rootfs.img
mkdir rootfs
sudo mount -o loop rootfs.img rootfs
cd lab3
make
cp init rootfs/
sudo umount rootfs
qemu-system-x86_64 -kernel linux-5.0.1/arch/x86_64/boot/bzImage -hda rootfs.img -append "root=/dev/sda init=/init" -s -S
# 另一个shell窗口
gdb
file linux-5.0.1/vmlinux
target remote:1234 #则可以建立gdb和gdbserver之间的连接
break start_kernel
按c 让qemu上的Linux继续运行
```


