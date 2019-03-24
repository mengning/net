#
# Makefile for menu program
#

CC_PTHREAD_FLAGS			 = -lpthread
CC_FLAGS                     = -c 
CC_OUTPUT_FLAGS				 = -o
CC                           = gcc
RM                           = rm
RM_FLAGS                     = -f

TARGET  =   init
OBJS    =   linktable.o  menu.o main.o

all:	$(OBJS)
	$(CC) $(CC_OUTPUT_FLAGS) $(TARGET) $(OBJS) -static -lpthread

.c.o:
	$(CC) $(CC_FLAGS) $<

clean:
	$(RM) $(RM_FLAGS)  $(OBJS) $(TARGET) *.bak init
