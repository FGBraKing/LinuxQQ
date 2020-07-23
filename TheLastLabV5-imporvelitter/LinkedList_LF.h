#ifndef _LINKEDLIST_LF_H_
#define _LINKEDLIST_LF_H_

/*
#ifndef LINKEDLIST_LF_H_INCLUDED
#define LINKEDLIST_LF_H_INCLUDED
*/

#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include"Queue_LF.h"
#include "Protocol_LF.h"

#define OK 1			//操作成功
#define ERROR 0			//操作失败


typedef struct LNode{	//链表结点类
	user_account data;
	struct LNode *next;
}LNode,*Link,*Position;

typedef struct{			//链表类
	Link head,tail;
	int len;
}LinkList;

int MakeNode(Link *p,user_account e);//分配由p指向的值为e的结点，并返回OK，如果失败，则返回ERROR
int InsFirst(Link h, Link s);//已知h指向线性链表得头结点，将s所指结点插入第一个结点之前
int DelFirst(Link h,Link *q);//已知h指向线性链表的头结点，删除链表中的第一个结点并以q返回
int Append(LinkList *L,Link s);//将指针s所指(彼此以指针相连)的一串结点链接在线性链表L的最后一个结点，之后改变链表L的尾指针指向新的尾结点
void FreeNode(Link *p);//释放p所指结点
int DestoryList(LinkList *L);//销毁线性链表L
int LocatePos(LinkList L,int i,Link *p);//返回p指示线性链表第i个结点的位置并返回Ok,i值不合法时返回ERROR

int InitList(LinkList *L);//初始化
int ClearList(LinkList *L);//重置为空表
int ListLength(LinkList L);//求长度
int ListEmpty(LinkList L);//是否空表
int GetElem(LinkList L,int i, user_account *e);//查找
int LocateElem(LinkList L, user_account e);//定位
int PriorElem(LinkList L,user_account cur_e,user_account *pre_e);//取前驱
int NextElem(LinkList L,user_account cur_e,user_account *next_e);//取后继
int ListInsert(LinkList *L,int i,user_account e);//插入
int ListDelete(LinkList *L,int i,user_account *e);//删除

int user_accountcmp(const user_account acct1, const user_account acct2);//比较user_account类型，相等-1，不等-0
int LNodecmp(const LNode L1, const LNode L2);//比较LNodecmp类型，相等-1，不等-0

#endif


