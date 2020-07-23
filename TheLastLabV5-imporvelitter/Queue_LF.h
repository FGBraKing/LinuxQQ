#ifndef _QUEUE_LF_H_
#define _QUEUE_LF_H_
/*#include<netinet/in.h>*/

#include<stdio.h>
#include<malloc.h>
#include<stdlib.h>
#include<arpa/inet.h>

#define OK 1			//操作成功
#define ERROR 0			//操作失败
#define OVERFLOW -2		//overflow，溢出
#define INFEASIBLE -1	//infeasible,不可行

struct thread_parameter{
	int    				cli_sock;
	struct sockaddr_in  cli_addr;
};

typedef struct QNode{
    struct thread_parameter data;
    struct QNode * next;
}QNode, *QueuePtr;

typedef struct {
    QueuePtr rear;
    QueuePtr front;
} LinkQueue;

int InitQueue ( LinkQueue *Q );
int DestoryQueue(LinkQueue *Q );
int ClearQueue(LinkQueue *Q );
int QueueEmpty ( LinkQueue Q );
int QueueLength ( LinkQueue Q );
int GetFront ( LinkQueue Q, struct thread_parameter *e );
int EnQueue ( LinkQueue *Q, struct thread_parameter e );
int DeQueue ( LinkQueue *Q, struct thread_parameter *e);
int QueueTraverse(LinkQueue Q,void (*visit)(struct thread_parameter data));
void PrintQuene(struct thread_parameter data);








#endif // QUEUE_H_INCLUDED


