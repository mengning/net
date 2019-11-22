# 网络相关命令

## 配置Linux系统连接Internet

典型的情形是我们的Linux机器在一个局域网中，局域网中有一个网关，能够让我们的Linux机器访问Internet，那么我们就需要首先给我们的Linux机器上连接到网关的网卡上设置IP地址，然后将这台网关的IP地址设置为Linux机器的默认路由，也称为默认网关。

### 使用ifconfig设置网卡

ifconfig是Linux系统中最常用的一个用来显示和设置网络设备的工具。其中“if”是“interface”的缩写。ifconfig可以用来设置网卡的状态，或是显示当前的设置。 
* 查看网卡的状态
```
ifconfig eth0   # 查看第一块网卡的状态
ifconfig        # 查看所有网卡的状态
```
* 将第一块网卡的IP地址设置为192.168.0.100
```
ifconfig eth0 192.168.0.100
```
对于以太网网卡的默认命名方式为eth0、eth1...
* 同时设置IP地址和子网掩码
```
ifconfig eth0 192.168.0.100 netmask 255.255.255.0 
```
* 暂时关闭或启用网卡
```
ifconfig eth0 down  # 关闭第一块以太网网卡
ifconfig eth0 up    # 启用第一块以太网网卡
```

### 使用route设置默认网关

route命令是用来查看和设置Linux系统的路由信息，以实现与其它网络的通讯。要实现两个不同的子网之间的网络通讯，需要一台连接两个网络路由器或者同时位于两个网络的网关来实现。这里的两个网络典型情况是：一个是我们的机器所在的局域网；另一个是外部的Internet。

* 显示出当前路由表
```
route
```
* 增加一个默认路由
```
route add 192.168.0.1 gw
```
### 配置DNS域名解析服务

我们的Linux系统实际上已经连接到Internet上了，只是我们只能通过IP地址来访问Internet上的网络服务。要想通过域名来来访问Internet上的网络服务，还需要配置DNS域名解析服务，也就是指定DNS服务器的IP地址。

* 参看系统默认DNS配置
```
cat /etc/resolv.conf
```
* 配置系统默认DNS
```
vi /etc/resolv.conf
nameserver 114.114.114.114 # 在/etc/resolv.conf文件里添加一行
```
通过文本编辑器（这里以vi为例，您也可以使用其他方式）打开/etc/resolv.conf 配置文件，添加一行nameserver 114.114.114.114 ，这个DNS服务器IP地址一般使用网络管理员指定的本地DNS服务器IP地址。

至此，我们可以通过域名来来访问Internet上的网络服务，通过浏览器打开网页http://staff.ustc.edu.cn/~mengning/ 测试一下看看吧！如果您的Linux没有图形界面，也可以通过wget命令来下载一个网页。
```
wget http://staff.ustc.edu.cn/~mengning/
```

