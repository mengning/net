# 通过网络命令学习计算机网络

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
如果无法打开网页怎么办？在确保IP地址、默认网关和DNS配置正确的情况下，就需要通过网络诊断工具来跟踪网络发现问题。
## 网络诊断工具

## iptables

iptables有5种chain：
* PREROUTING:数据包进入路由表之前
* INPUT:通过路由表后目的地为本机
* FORWARDING:通过路由表后，目的地不为本机
* OUTPUT:由本机产生，向外转发
* POSTROUTIONG:发送到网卡接口之前。

将报文的处理过程大致分成三类，每类报文会经过不同的chain：

* 到本机某进程的报文：PREROUTING --> INPUT
* 由本机转发的报文：PREROUTING --> FORWARD --> POSTROUTING
* 由本机的某进程发出报文（通常为响应报文）：OUTPUT --> POSTROUTING

![iptables的5种chain和3类处理过程](images/iptables-chains.png)
图：iptables的5种chain和3类处理过程（图片需要修改，将“web服务终点/原点”改为“网络应用程序”）

为了方便管理，把具有相同功能的规则的集合叫做table，不同功能的规则，放置在不同的table中进行管理，iptables中定义了4种table。四种表的功能为
* filter表：负责过滤功能，防火墙；内核模块：iptables_filter
* nat表：network address translation，网络地址转换功能；内核模块：iptable_nat
* mangle表：拆解报文，做出修改，并重新封装 的功能；iptable_mangle
* raw表：关闭nat表上启用的连接追踪机制；iptable_raw
四种table处理顺序由先到后为：raw→mangle→nat→filter。但除了OUTPUT外有全部的四种表规则外，其他的chain只有两三个表规则。
* PREROUTING ：raw表，mangle表，nat表
* INPUT ：mangle表，filter表，（centos7中还有nat表，centos6中没有）
* FORWARD：mangle表，filter表
* OUTPUT：raw表，mangle表，nat表，filter表
* POSTROUTING：mangle表，nat表

![iptables的5种chain和4种tables](images/iptables-tables.png)
图：iptables的5种chain和4种tables （图片来源于网络，考虑重新配色）
