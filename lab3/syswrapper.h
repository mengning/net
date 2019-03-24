
/********************************************************************/
/* Copyright (C) SSE-USTC, 2012                                     */
/*                                                                  */
/*  FILE NAME             :  syswraper.h                            */
/*  PRINCIPAL AUTHOR      :  Mengning                               */
/*  SUBSYSTEM NAME        :  system                                 */
/*  MODULE NAME           :  syswraper                              */
/*  LANGUAGE              :  C                                      */
/*  TARGET ENVIRONMENT    :  Linux                                  */
/*  DATE OF FIRST RELEASE :  2012/11/22                             */
/*  DESCRIPTION           :  the interface to Linux system(socket)  */
/********************************************************************/

/*
 * Revision log:
 *
 * Created by Mengning,2012/11/22
 *
 */

#ifndef _SYS_WRAPER_H_
#define _SYS_WRAPER_H_

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
//#define NDEBUG
#include<assert.h>

#define PORT                5001
#define IP_ADDR             "127.0.0.1"
#define MAX_BUF_LEN         1024

/* private macro */
#define PrepareSocket(addr,port)                        \
        int sockfd = -1;                                \
        struct sockaddr_in serveraddr;                  \
        struct sockaddr_in clientaddr;                  \
        socklen_t addr_len = sizeof(struct sockaddr);   \
        serveraddr.sin_family = AF_INET;                \
        serveraddr.sin_port = htons(port);              \
        serveraddr.sin_addr.s_addr = inet_addr(addr);   \
        memset(&serveraddr.sin_zero, 0, 8);             \
        sockfd = socket(PF_INET,SOCK_STREAM,0);
        
#define InitServer()                                    \
        int ret = bind( sockfd,                         \
                        (struct sockaddr *)&serveraddr, \
                        sizeof(struct sockaddr));       \
        if(ret == -1)                                   \
        {                                               \
            fprintf(stderr,"Bind Error,%s:%d\n",        \
                            __FILE__,__LINE__);         \
            close(sockfd);                              \
            return -1;                                  \
        }                                               \
        listen(sockfd,MAX_CONNECT_QUEUE); 

#define InitClient()                                    \
        int ret = connect(sockfd,                       \
            (struct sockaddr *)&serveraddr,             \
            sizeof(struct sockaddr));                   \
        if(ret == -1)                                   \
        {                                               \
            fprintf(stderr,"Connect Error,%s:%d\n",     \
                __FILE__,__LINE__);                     \
            return -1;                                  \
        }
/* public macro */               
#define InitializeService()                             \
        PrepareSocket(IP_ADDR,PORT);                    \
        InitServer();
        
#define ShutdownService()                               \
        close(sockfd);
         
#define OpenRemoteService()                             \
        PrepareSocket(IP_ADDR,PORT);                    \
        InitClient();                                   \
        int newfd = sockfd;
        
#define CloseRemoteService()                            \
        close(sockfd); 
              
#define ServiceStart()                                  \
        int newfd = accept( sockfd,                     \
                    (struct sockaddr *)&clientaddr,     \
                    &addr_len);                         \
        if(newfd == -1)                                 \
        {                                               \
            fprintf(stderr,"Accept Error,%s:%d\n",      \
                            __FILE__,__LINE__);         \
        }        
#define ServiceStop()                                   \
        close(newfd);
        
#define RecvMsg(buf)                                    \
       ret = recv(newfd,buf,MAX_BUF_LEN,0);             \
       if(ret > 0)                                      \
       {                                                \
            printf("recv \"%s\" from %s:%d\n",          \
            buf,                                        \
            (char*)inet_ntoa(clientaddr.sin_addr),      \
            ntohs(clientaddr.sin_port));                \
       }
       
#define SendMsg(buf)                                    \
        ret = send(newfd,buf,strlen(buf),0);            \
        if(ret > 0)                                     \
        {                                               \
            printf("send \"hi\" to %s:%d\n",            \
            (char*)inet_ntoa(clientaddr.sin_addr),      \
            ntohs(clientaddr.sin_port));                \
        }
        
#endif /* _SYS_WRAPER_H_ */


