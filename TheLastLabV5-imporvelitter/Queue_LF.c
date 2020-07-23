#include "Queue_LF.h"


int InitQueue(LinkQueue *Q)
{ // 构造一个空队列Q
    Q->front = Q->rear = (QueuePtr)malloc(sizeof(QNode));
    if(!Q->front)
        exit(OVERFLOW);
    Q->front->next = NULL;
    return OK;
}

int DestoryQueue(LinkQueue *Q )
{
    while(Q->front)
    {
        Q->rear = Q->front ->next;
        free(Q->front);
        Q->front = Q->rear;
    }
    return OK;
}

int ClearQueue(LinkQueue *Q )
{
    QueuePtr p=Q->front->next;
    while(p)
    {
        Q->rear = p ->next;
        free(p);
        p=Q->rear ;
    }
    Q->rear=Q->front;
    Q->rear->next=NULL;
    return OK;
}

int QueueEmpty ( LinkQueue Q )
{
    return(Q.front == Q.rear);
}


int DeQueue(LinkQueue *Q, struct thread_parameter *e)
{ // 若队列不空，删除Q 的队头元素，用e 返回其值
    QueuePtr p;
    if(Q->front == Q->rear)
        return ERROR;
    p = Q->front->next;
    *e = p->data;
    Q->front->next = p->next;
    if(Q->rear == p)
        Q->rear = Q->front;
    free(p);
    return OK;
}

int EnQueue(LinkQueue *Q, struct thread_parameter e)
{ // 插入元素e 为Q 的新的队尾元素
    QueuePtr p = (QueuePtr)malloc(sizeof(QNode));
    if(!p)
        exit(OVERFLOW);
    p->data = e; p->next = NULL;
    Q->rear->next = p;
    Q->rear = p;
    return OK;
}

int QueueLength ( LinkQueue Q )
{
    int len=0;
    QueuePtr p=Q.front->next;
    while(p)
    {
        len++;
        p=p->next;
    }
    return len;
}

int GetFront ( LinkQueue Q, struct thread_parameter *e )
{
    if(Q.front == Q.rear)
        return ERROR;
    *e =Q.front->next->data;
    return OK;
}

int QueueTraverse(LinkQueue Q,void (*visit)(struct thread_parameter data))
{
    QueuePtr p=Q.front->next;
    while(p)
    {
        visit(p->data);
        p=p->next;
    }
    return OK;
}

void PrintQuene(struct thread_parameter data)
{
    //待补充
}
