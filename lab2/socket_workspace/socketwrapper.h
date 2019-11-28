
/********************************************************************/
/* Copyright (C) SSE-USTC, 2010                                     */
/*                                                                  */
/*  FILE NAME             :  socketwraper.h                         */
/*  PRINCIPAL AUTHOR      :  Mengning                               */
/*  SUBSYSTEM NAME        :  ChatSys                                */
/*  MODULE NAME           :  ChatSys                                */
/*  LANGUAGE              :  C                                      */
/*  TARGET ENVIRONMENT    :  ANY                                    */
/*  DATE OF FIRST RELEASE :  2010/10/18                             */
/*  DESCRIPTION           :  the interface to socket layer.         */
/********************************************************************/

/*
 * Revision log:
 *
 * Created by Mengning,2010/10/18
 *
 */

#ifndef _SOCKET_WRAPER_H_
#define _SOCKET_WRAPER_H_

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>	/* gethostbyname */

/* ChatSys Socket  - Standard Socket Call Mapping Definition */
#define  Socket(x,y,z)    		 socket(x,y,z)                 
#define  Bind(x,y,z)     		 bind(x,y,z)                   
#define  Connect(x,y,z)          connect(x,y,z)         
#define  Listen(x,y)             listen(x,y)            
#define  Read(x,y,z )            read(x,y,z)            
#define  Accept(x,y,z )          accept(x,y,(socklen_t *)z)          
#define  Recv(w,x,y,z)           recv(w,x,y,z)          
#define  Recvfrom(a,b,c,d,e,f)   recvfrom(a,b,c,d,e,f ) 
#define  Recvmsg(a,b,c)          recvmsg(a,b,c)         
#define  Write(a,b,c)            write(a,b,c)           
#define  Send(a,b,c,d)           send(a,b,c,d)          
#define  Sendto(a,b,c,d,e,f)     sendto(a,b,c,d,e,f)    
#define  Sendmsg(a,b,c)          sendmsg(a,b,c)         
#define  Close(a)                close(a) 
/* ¸ñÊ½×ª»» */
#define  Htons(a)				 htons(a)
#define	 Inet_ntoa(a)			 inet_ntoa(a)

/* Name */
#define	 Gethostbyname(a)		 gethostbyname(a)	 
#endif /* _SOCKET_WRAPER_H_ */


