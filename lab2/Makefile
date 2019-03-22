#
# Makefile for Socket Program
#

CC_FLAGS                     = -c 
CC_OUTPUT_FLAGS				 = -o
CC                           = gcc
RM                           = rm
RM_FLAGS                     = -f


all:
	$(CC) $(CC_OUTPUT_FLAGS) tcp_server tcp_server.c
	$(CC) $(CC_OUTPUT_FLAGS) tcp_client tcp_client.c
	$(CC) $(CC_OUTPUT_FLAGS) udp_server udp_server.c
	$(CC) $(CC_OUTPUT_FLAGS) udp_client udp_client.c

clean:
	$(RM) $(RM_FLAGS)  *.o tcp_server tcp_client udp_server udp_client
