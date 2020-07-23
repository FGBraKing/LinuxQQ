#Makefile for lab 6
EXE1=client_LF
EXE2=server_LF
EXE=client_LF  server_LF
OBJS1=client_LF.o Protocol_LF.o 
OBJS2=server_LF.o Protocol_LF.o Queue_LF.o LinkedList_LF.o

CC  =gcc
CFLAGS=-g
LDFLAGS=-lpthread

.PHONY:all
all:$(EXE) 

$(EXE1):$(OBJS1)
	$(CC) $(OBJS1) -o $(EXE1)  
$(EXE2):$(OBJS2)
	$(CC) -o $(EXE2) $(OBJS2) -lpthread 


.PHONY:clean 
clean:
	rm -rf $(OBJS1) $(OBJS2) 
	#$(EXE)
