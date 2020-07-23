#include "LinkedList_LF.h"

int user_accountcmp(const user_account acct1, const user_account acct2)//比较user_account类型，相等-1，不等-0
{
//	if(strcmp(acct1->name,acct2->name)||strcmp(acct1->passwd,acct2->passwd))
/* 	if(strcmp(acct1.name,acct2.name)||strcmp(acct1.passwd,acct2.passwd))
		return 0;
	else
		return 1; */
//||strcmp(acct1.passwd,acct2.passwd)
	return !(strcmp(acct1.name,acct2.name));//账号名相等就当账号相等
}

int LNodecmp(const LNode L1, const LNode L2)//比较LNodecmp类型，相等-1，不等-0
{
	/* if(user_accountcmp(L1.data,L2.data)&&L1.next==L2.next)
		return 1;
	else
		return 0; */
	return (user_accountcmp(L1.data,L2.data)&&(L1.next==L2.next));
}

int MakeNode(Link *p,user_account e)   //分配由p指向的值为e的结点，并返回OK，如果失败，则返回ERROR
{
	(*p)=(Link)malloc(sizeof(LNode));
	if(!(*p)) return ERROR;
	(*p)->data=e;
	(*p)->next=NULL;
	return OK;
}

void FreeNode(Link *p)	//释放p所指结点
{
	Link q=*p;
	*p=(*p)->next;
	free(q);
}

int InitList(LinkList *L)	//初始化
{
	L->head=L->tail=(Link)malloc(sizeof(LNode));
	if(!L->head) return ERROR;
	L->tail->next=NULL;
	L->len=0;
	return OK;
}

int DestoryList(LinkList *L)	//销毁线性链表L
{
    Link pt;
	for (;L->head!=L->tail;L->len--)
	{
        pt = L->head;
        L->head = L->head->next;
        free(pt);
    }
    free(L->head);
    if (L->len == 0)
	{
        L->head = NULL;
        L->tail = NULL;
        free(L);
        return OK;
    }
    else return ERROR;
}

int ClearList(LinkList *L)	//重置为空表
{
    Link pt;
	Link temp = L->head;
    L->head = L->head->next;
    for (; L->len>0; L->len--)
	{
        pt = L->head;
        L->head = L->head->next;
        free(pt);
    }
    L->head=L->tail=temp;
    if (L->len == 0&&L->head==L->tail) return OK;
    else return ERROR;
}

int InsFirst(Link h, Link s)	//已知h指向线性链表得头结点，将s所指结点插入第一个结点之前
{
	if(h)
	{s->next=h->next;h->next=s; return OK;}
	else return ERROR;
}

int DelFirst(Link h,Link *q)	//已知h指向线性链表的头结点，删除链表中的第一个结点并以q返回
{
	if(h->next)
		{(*q)=h->next;h->next=h->next->next;(*q)->next=NULL;return OK;}
	else return ERROR;
}

int Append(LinkList *L,Link s)	//将指针s所指(彼此以指针相连)的一串结点链接在线性链表L的最后一个结点，之后改变链表L的尾指针指向新的尾结点
{
	if(s)
	{
		L->tail->next=s;
		do{s=s->next;L->len++;}while(s->next);
		L->tail=s;
		return OK;
	}
	else return ERROR;
}

int ListLength(LinkList L)	//求长度
{
	return L.len;
}

int ListEmpty(LinkList L) //是否空表
{
	if(!L.len&&L.head==L.tail) return 1;
	else return 0;
}

int GetElem(LinkList L,int i, user_account *e)	//查找
{
	Link p;
	if (i<1||i>L.len) return ERROR;
	p=L.head;
	for(;i>0;i--)
	{p=p->next;}
	*e=p->data;
	return OK;
}

int LocateElem(LinkList L, user_account e)	//定位
{
	int i=1;
	for(;L.head&&i<=L.len;i++)
	{
		L.head=L.head->next;
//		if(L.head->data==e) return i;
//		if(!strcmp((L.head->data).name,e.name)&&!strcmp((L.head->data).passwd,e.passwd))
		if(user_accountcmp(L.head->data,e))
			return i;
	}
	return ERROR;
}

int PriorElem(LinkList L,user_account cur_e,user_account *pre_e)
{
	//for(;L.head->next->data!=cur_e&&L.head->next->next;L.head=L.head->next) {}
	//strcmp((L.head->next->data).name,cur_e.name)||strcmp((L.head->next->data).passwd,cur_e.passwd)
	while((!user_accountcmp(L.head->next->data,cur_e))&&L.head->next->next) 
	{L.head=L.head->next;}
	//L.head->next->data==cur_e
	if(L.head->next->next||user_accountcmp(L.head->next->data,cur_e)) {*pre_e=(L.head)->data;return OK;}
	else return ERROR;
}

int NextElem(LinkList L,user_account cur_e,user_account *next_e)	//取后继
{
	//L.head->data!=cur_e
	for(L.head=L.head->next;!user_accountcmp(L.head->data,cur_e)&&L.head->next;L.head=L.head->next){}
		if(L.head->next) *next_e=L.head->next->data;
		else return ERROR;
	return OK;
}

int LocatePos(LinkList L,int i,Link *p)//返回p指示线性链表第i个结点的位置并返回Ok,i值不合法时返回ERROR
{
	for(;i>0&&L.head;i--)  {L.head=L.head->next;}
	if(L.head) {*p=L.head;return OK;}
	else return ERROR;
}

int ListInsert(LinkList *L,int i,user_account e)	//插入
{
	Link h,s;
	if(!LocatePos(*L,i-1,&h)) return ERROR;
	if(!MakeNode(&s,e)) return ERROR;
	InsFirst(h,s);
	if(i==L->len+1) L->tail=s;
	L->len++;
	return OK;
}

int ListDelete(LinkList *L,int i,user_account *e)//删除
{
	Link h,s;
	if(!LocatePos(*L,i-1,&h)) return ERROR;
	if(!h->next) return ERROR;
	DelFirst(h,&s);
	*e=s->data;
	FreeNode(&s);
	L->len--;
	return OK;
}


