#
# Makefile for Workspace
#

CC_PTHREAD_FLAGS			 = -lpthread
CC_TIME_FLAGS				 = -lrt
CC_FLAGS                     = -c 
CC_OUTPUT_FLAGS				 = -o
CC                           = gcc
RM                           = rm
RM_FLAGS                     = -f

SERVER_TARGET = server
CLIENT_TARGET = client

TARGETS	 =  $(SERVER_TARGET) $(CLIENT_TARGET)

COMMON_OBJS = 
SERVER_OBJS = server.o $(COMMON_OBJS)
CLIENT_OBJS = dbtime.o client.o $(COMMON_OBJS)

OBJS     =  $(COMMON_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) 

all: $(OBJS) $(TARGETS)

$(SERVER_TARGET):$(SERVER_OBJS)
	$(CC) $(CC_OUTPUT_FLAGS) $(SERVER_TARGET) $(SERVER_OBJS) $(CC_PTHREAD_FLAGS)

$(CLIENT_TARGET):$(CLIENT_OBJS)
	$(CC) $(CC_OUTPUT_FLAGS) $(CLIENT_TARGET) $(CLIENT_OBJS) $(CC_PTHREAD_FLAGS) $(CC_TIME_FLAGS)

.c.o:
	$(CC) $(CC_FLAGS) $<

clean:
	$(RM) $(RM_FLAGS)  $(OBJS) $(TARGETS) *.bak dbtime.time
