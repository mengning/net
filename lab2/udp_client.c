#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 

int main(int argc, char *argv[])
{
	int client_sockfd;
	int len;
        struct sockaddr_in remote_addr; //服务器端网络地址结构体
	int sin_size;
	char buf[BUFSIZ];  //数据传送的缓冲区
	memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
	remote_addr.sin_family=AF_INET; //设置为IP通信
	remote_addr.sin_addr.s_addr=inet_addr("127.0.0.1");//服务器IP地址
	remote_addr.sin_port=htons(8000); //服务器端口号

         /*创建客户端套接字--IPv4协议，面向无连接通信，UDP协议*/
	if((client_sockfd=socket(PF_INET,SOCK_DGRAM,0))<0)
	{  
		perror("socket");
		return 1;
	}
	strcpy(buf,"This is a test message");
	printf("sending: '%s'\n",buf);
	sin_size=sizeof(struct sockaddr_in);
	
	/*向服务器发送数据包*/
	if((len=sendto(client_sockfd,buf,strlen(buf),0,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr)))<0)
	{
		perror("recvfrom"); 
		return 1;
	}
	close(client_sockfd);
	return 0;
}