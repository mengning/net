# 网络相关命令

### ifconfig

ifconfig是Linux系统中最常用的一个用来显示和设置网络设备的工具。其中“if”是“interface”的缩写。ifconfig可以用来设置网卡的状态，或是显示当前的设置。 

* 将第一块网卡的IP地址设置为192.168.0.1
```
ifconfig eth0 192.168.0.1
```
对于以太网网卡的默认命名方式为eth0、eth1...
