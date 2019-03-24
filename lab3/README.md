## 编译构建调试Linux系统

Based on Ubuntu 18.04 & linux-5.0.1


```
https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.0.1.tar.xz
xz -d linux-5.0.1.tar.xz
tar -xvf linux-5.0.1.tar
cd linux-5.0.1
sudo apt install build-essential flex bison libssl-dev libelf-dev libncurses-dev
make defconfig
make menuconfig # Kernel hacking—>[*] Kernel debugging
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
qemu-system-x86_64 -kernel linux-5.0.1/arch/x86/boot/bzImage -hda rootfs.img -append "root=/dev/sda init=/init"
```


