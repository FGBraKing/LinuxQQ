#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
/*自定义头文件*/
#include "Queue_LF.h"
#include "Protocol_LF.h"
#include "LinkedList_LF.h"

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"
#define BUFFSIZE 1000
#define SERVPORT 10258 /*server listening port*/
#define BACKLOG 10 /*allow total number of connection*/

/*---server.c---by LF--- 2018-06-23---
功能:
1.C/S模式，服务器支持多个用户同时在线（>3)。 
2.注册Registe
3.登录Login
4.获取用户列表List
5.聊天p2p Chat
6.聊天p2a 
7.文件传输 SendFile
8.退出   Bye
*/

/*---全局变量，所有线程共享的资源---*/
int running_thread_num=0; //主线程已创建的线程数目
int serversock;//服务器的套接字
LinkQueue ClientQueue;//客户端地址队列，每连接一个客户端，入队一次，创建一个线程函数，出队一次
LinkList user_list;//账号列表
pthread_mutex_t mylock;//互斥锁1
/*--------------------函数列表-------------------------*/
int server_init(const char* ip, int port);
void* pthfunc(void* arg);
void trim_newline (char *text);
int server_exit(int aimsock);
int server_Registe(user_account acot);
int server_Login(user_account acot, int sock);
int server_logout(user_account acot, int sock);
void send_logout_massage(int aimsock);
void get_accountname(int socket, char *name);//you套接字得到账户名
int server_getlist(int aimsock, int flag);
int server_p2a(int aimsock, unsigned char *data);
int server_p2p(int aimsock,short len,const unsigned char *data);
int server_chatting(int aimsock,int *friendsocket, int num);
int server_sendfile(int aimsock,char flag, short len,const unsigned char *filepath);
int server_recvfile(int aimsock,char flag, short len,const unsigned char *filename);
void server_sending(int aimsock, FILE *fd);
void server_recving(int aimsock, FILE *fd);
int setchating(int socket);
int resetchating(int socket);
int ischating(int socket);

/*--------------------------------------实现------------------------------------- */
int setchating(int socket)
{
	Link p=user_list.head;
	while(p)
	{
		if(p->data.socket==socket)
		{
			p->data.chating=socket;
			break;
		}
		p=p->next;
	}
	if(p)
		return 1;
	else
		return 0;
}
int resetchating(int socket)
{
	Link p=user_list.head;
	while(p)
	{
		if(p->data.socket==socket)
		{
			p->data.chating=0;
			break;
		}
		p=p->next;
	}
	if(p)
		return 1;
	else
		return 0;
}
int ischating(int socket)
{
	Link p=user_list.head;
	while(p)
	{
		if(p->data.socket==socket&&p->data.chating==socket)
		{
				break;
		}
		p=p->next;
	}
	if(p)
		return 1;
	else
		return 0;
}
void send_logout_massage(int aimsock)
{
	typepack bigpack;	//通用传输包
	memset(&bigpack,0,sizeof(typepack));
	bigpack.head=CMD_FLOGOUT;
	if((newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
		{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
}
void server_sending(int aimsock, FILE *fd)//从aimsock读，写入fd
{
	typepack bigpack;
	char buf[BUFFSIZE];
	int rlen,wlen;
	int i;
	int len_data=0;
	int total=0;
	unsigned char  *temp_buf=(unsigned char *)bigpack.data;
	int *plen=(int*)(bigpack.data+BUFFSIZE);
	
	memset(&bigpack,0,sizeof(typepack));
	
	memset(buf,0,BUFFSIZE);
	while(1)
	{
			if((newrecv(aimsock, &bigpack, sizeof(typepack), 0))==-1) 
				{fprintf(stderr,"recv error:%s\n\a",strerror(errno));}
			//sleep(1);
			if(bigpack.head!=CMD_FILEING)
			{
				if(bigpack.head==CMD_FILEENDING)
				{
					printf("sending end  massage:%s\n",(char*)bigpack.data);
				}
				return;
			}
			else
			{
				total=0;
				len_data=*plen;//strlen(bigpack.data);
				for(i=0;i<len_data;i++) 
				{
					temp_buf[i]=bigpack.data[i];//
					buf[i]=temp_buf[i];
				//	printf("data start:\n");
				//	printf("%c",temp_buf[i]);
				//	printf("data end");
				}
				wlen=fwrite(buf,1,len_data,fd);
				//printf("testing server_sending,wlen:%d",wlen);
				total+=wlen;
				while(wlen!=len_data)
				{
					if(wlen>len_data||wlen<0) return;
					else
					{
						len_data=len_data-wlen;
						wlen=fwrite(buf+total,1,len_data,fd);
						total+=wlen;
						if(wlen==0) break;
					}		
				}
			}
			memset(buf,0,sizeof(char)*BUFFSIZE);
	}
}
void server_recving(int aimsock, FILE *fd)//从fd读，然后发送
{
	typepack bigpack;//收发的包类型
	char buf[BUFFSIZE];
	int *plen=(int*)(bigpack.data+BUFFSIZE);
	
	int wlen,rlen;
	char respon_msg[50];//返回信息
	
	memset(buf,0,BUFFSIZE);
	memset(&bigpack,0,sizeof(bigpack));	
	if((rlen=fread(buf,1,BUFFSIZE,fd))<0)
		{fprintf(stderr,"fread error:%s\n\a",strerror(errno));}
	//printf("testing server_recving,rlen:%d",rlen);
	while(rlen>0)
		{
			//sleep(1);
			//buf[rlen]='\0';
			packup(&bigpack, CMD_FILEING, (unsigned char *)buf, rlen+1);
			*plen=rlen;
			if((newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
				{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
			memset(buf,0,BUFFSIZE);
			if((rlen=fread(buf,1,BUFFSIZE,fd))<0)
				{fprintf(stderr,"fread error:%s\n\a",strerror(errno));}	
			//printf("testing server_recving,rlen:%d",rlen);
		}
	bigpack.head=CMD_FILEENDING;strcpy(respon_msg,"file sending end!");
	packup(&bigpack, CMD_FILEENDING, (unsigned char *)respon_msg, sizeof(char)*50);
	if((newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
}
void get_accountname(int socket, char *name)
{
	Link p=user_list.head;
	while(p&&(p->data.socket!=socket)) {p=p->next;}
	if(p) strcpy(name,p->data.name);
}
int server_exit(int aimsock)
{
	int pos=1;
	Link p=user_list.head;
	typepack bigpack;	//通用传输包
	char msg[100];
	user_account temp_accout;

	memset(&bigpack,0,sizeof(typepack));	
	while(p&&pos<=user_list.len) 
	{p=p->next; if(p->data.socket== aimsock) break;pos++;}
	if(pos>user_list.len)
		{
			printf("未找到套接字对应的账户\n");
			bigpack.head=0;
			sprintf(msg,"%s exit: 未找到套接字对应的账户",p->data.name);
			strncpy((char*)bigpack.data,msg,100);
			if((newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
				{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}	
			
			return 0;
		}
	else
	{
		(p->data).login=0;(p->data).socket=0;
		bigpack.head=1;
		sprintf(msg,"%s exiting",p->data.name);
		strncpy((char*)bigpack.data,msg,100);
		if((newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
			{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
		close(aimsock);
		return 1;
	}
}
void trim_newline (char *text)
{
    int len = strlen (text) - 1;
    if (text[len] == '\n')
      {
          text[len] = '\0';
      }
}
int server_sendfile(int aimsock,char flag,short len,const unsigned char *filepath)//服务器接收文件并保存
{
	OperationPack sendpack;
	typepack bigpack;				//收发的包类型
	char respon_msg[50];			//返回信息
	char buf[BUFFSIZE];
	char username[50];
	char sflag;
	FILE *fd;
	int i;
	memset(&bigpack,0,sizeof(typepack));
	memset(&sendpack,0,sizeof(OperationPack));
	memset(buf,0,sizeof(char)*BUFFSIZE);
	get_accountname(aimsock,username);
	
	printf("server start to recv a file!\n");
	fd=fopen(filepath,"w");//ab
	if(fd==NULL)
	{fprintf(stderr,"fopen error:%s\n\a",strerror(errno));
	strcpy(respon_msg,"open file error!");sflag=0;}
	else{strcpy(respon_msg,"open file success!");sflag=1;}

	OP_packup(&sendpack, CMD_OPRESULT, sflag, (const unsigned char *)respon_msg, strlen(respon_msg)+1);
	packup(&bigpack, sendpack.head, (unsigned char *)&sendpack, sizeof(OperationPack));
	if((newsend(aimsock, &bigpack, sizeof(typepack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}

	if(sflag==1)
		server_sending(aimsock,fd);
	if(fd)
		fclose(fd);
	return 1;
}
int server_recvfile(int aimsock,char flag,short len,const unsigned char *filename)//服务器读文件然后发送
{
	OperationPack sendpack;
	typepack bigpack;				//收发的包类型
	char respon_msg[50];			//返回信息
	char buf[BUFFSIZE];
	char username[50];
	char sflag;
	FILE *fd;
	int i;
	memset(&bigpack,0,sizeof(typepack));
	memset(&sendpack,0,sizeof(OperationPack));
	get_accountname(aimsock,username);
	
	printf("server start to send a file!\n");
	fd=fopen(filename,"r");//rb
	if(fd==NULL)
	{fprintf(stderr,"fopen error:%s\n\a",strerror(errno));
	strcpy(respon_msg,"file non-enisten!");sflag=0;}
	else{strcpy(respon_msg,"open file success!");sflag=1;}

	OP_packup(&sendpack, CMD_OPRESULT, sflag, (const unsigned char *)respon_msg, strlen(respon_msg)+1);
	packup(&bigpack, sendpack.head, (unsigned char *)&sendpack, sizeof(OperationPack));
	if((newsend(aimsock, &bigpack, sizeof(typepack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
	if(sflag==1)
	{server_recving(aimsock,fd);}
	if(fd)
		fclose(fd);
	return 1;	
}
int server_p2p(int aimsock,short len,const unsigned char *data)
{
	OperationPack sendpack;
	typepack bigpack;				//收发的包类型
	user_account temp_account;
	char username[50];				//账号名
	char respon_msg[50];			//返回信息
	int friendsocket=0;
	int chattingflag=0;  //聊天标志
	int i,j;
	//local_account local_accout;
	
	
	//get_accountname(aimsock,local_accout.name);
	//j=LocateElem(user_list,local_accout);//j为位置
	memset(&temp_account,0,sizeof(user_account));	
	memset(&bigpack,0,sizeof(typepack));
	memset(&sendpack,0,sizeof(OperationPack));
	for(i=0;i<len;i++) 
		username[i]=data[i]; 
	username[i]='\0';//这步多余，实际长度为len-1
	strcpy(temp_account.name,username);
	i=LocateElem(user_list,temp_account);//i为位置
	if(i<=0) //账号不存在
	{
		strcpy(respon_msg,"The account'name is non-existent!"); friendsocket=0;
	}
	else
	{
		GetElem(user_list,i, &temp_account);//查找
		if(temp_account.login==0)
		{
			strcpy(respon_msg,"The account is logout!"); friendsocket=0;
		}
		else
		{
			chattingflag=1;
			strcpy(respon_msg,"Success connect your friend!"); friendsocket=temp_account.socket;
		}
	}
	OP_packup(&sendpack, CMD_OPRESULT, friendsocket, (const unsigned char *)respon_msg, strlen(respon_msg)+1);
	packup(&bigpack, sendpack.head, (unsigned char *)&sendpack, sizeof(OperationPack));
	if((newsend(aimsock, &bigpack, sizeof(typepack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
	if(chattingflag==1)
		server_chatting(aimsock,&friendsocket, 1);
	return 1;
}
int server_p2a(int aimsock, unsigned char *data)//以后再实现指定用户的群聊，其实多几个用户名参数就很容易实现，只是打包麻烦一点
{
	OperationPack sendpack;
	typepack bigpack;				//收发的包类型
	user_account temp_account;
	char username[20][50];				//账号名
	char respon_msg[50];			//返回信息
	int chattingflag=0;
	int pos[20];//存在线的用户的套接字
	int i;
	int online_num;
	memset(&bigpack,0,sizeof(typepack));
	memset(&sendpack,0,sizeof(OperationPack));
	for(i=0,online_num=0;i<user_list.len;i++)//找出所有在线成员的套接字
	{
		if(GetElem(user_list,i+1, &temp_account))
		{
			if(temp_account.login&&temp_account.socket!=aimsock)//去除自己
			{pos[online_num]=temp_account.socket; online_num++;chattingflag=1;}
		}
	}
	strcpy(respon_msg,"success connect all user!\n");
	OP_packup(&sendpack, CMD_OPRESULT, chattingflag, (const unsigned char *)respon_msg, strlen(respon_msg)+1);
	packup(&bigpack, sendpack.head, (unsigned char *)&sendpack, sizeof(OperationPack));
	if((newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
		{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
	if(chattingflag==1)
		server_chatting(aimsock,pos,online_num);
	return 1;
}
int server_chatting(int aimsock,int *friendsocket, int num)//聊天中
{
	typepack bigpack;
	int i;
	memset(&bigpack,0,sizeof(typepack));
	if(setchating(aimsock)==0) return 0;
	while(1)
	{
		if((newrecv(aimsock, &bigpack, sizeof(typepack), 0))==-1) 
			{fprintf(stderr,"recv error:%s\n\a",strerror(errno));}
		if(bigpack.head==CMD_CHATTING)
		{
			for(i=0;i<num;i++)
				if(friendsocket[i]!=aimsock&&ischating(friendsocket[i]))
					if((newsend(friendsocket[i], &bigpack, sizeof(bigpack), 0))==-1)
					{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}	
		}
		else if(bigpack.head==CMD_ABORTCHAT)
		{
			bigpack.head=CMD_ABORTRESULT; strcpy((char*)bigpack.data,"stopping chat!");
	//		for(i=0;i<num;i++)
				if((newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)//friendsocket[i]
					{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}	 break;}
		else
			continue;
	}
	i=0;
	while(!resetchating(aimsock))
	{
		i++;
		if(i>50) break;
	}
	return 1;
}
int server_getlist(int aimsock, int flag)
{
	typepack bigpack;					//通用传输包
	user_account data;
	char uselist[20][50];
	int i,num;//num要小于20
	memset(&bigpack,0,sizeof(typepack));
	memset(&uselist,'\0',sizeof(char)*50*20);
	if(user_list.len==0) 
	{
		num=0;
	}
	else
	{
		if(flag)//在线用户
			{
				for(i=0,num=0;i<user_list.len&&num<20;i++)
				{
					if(GetElem(user_list,i+1, &data))
					{
						if(data.login)
						{strcpy(uselist[num],data.name); num++;}
					}
					else 
					{printf("GetElem Error!\n"); return 1;}
				}
			}
		else
			{
				for(i=0;i<user_list.len&&i<20;i++) 
					if(GetElem(user_list,i+1, &data))
						{strcpy(uselist[i],data.name);}
				num=user_list.len;
				if(num>=20) num=20;
			}
	}
	packup(&bigpack, CMD_OPRESULT, (unsigned char *)uselist, sizeof(char)*50*20);
	bigpack.data[sizeof(char)*50*20]=num;
	if((newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno)); return 2;}
	return 0;
}
/*--注销--*/
int server_logout(user_account acot, int sock)
{
	int pos;
	Link p=user_list.head;
	printf("%s want to logout!\n",acot.name);
	//printf("Logouting! acot'name:%s,passwd:%s\n",acot.name,acot.passwd);
	if(pos=LocateElem(user_list,acot))
	{
		while(pos>0) {p=p->next; pos--;}
		//printf("local'name:%s,passwd:%s\n",p->data.name,p->data.passwd);
		if(p->data.login)
		{
			if(p->data.socket==sock)
			{(p->data).login=0;(p->data).socket=0;return 0;}
			else
				return 3;
		}
		else
		{
			return 1;//账号早已经注销
		}
	}
	else
	{
		return 2;//账号不存在
	}
}
/*--登录--*/
int server_Login(user_account acot, int sock)
{
	int pos;
	Link p=user_list.head;
	printf("Logining! acot'name:%s,passwd:%s\n",acot.name,acot.passwd);
	if(pos=LocateElem(user_list,acot))
	{
		while(pos>0) {p=p->next; pos--;}
		printf("local'name:%s,passwd:%s\n",p->data.name,p->data.passwd);
		if(!strcmp(p->data.passwd,acot.passwd))
		{
			(p->data).login=1; (p->data).socket=sock;
			return 0;
		}
		else
		{
			return 2;//密码错误
		}
	}
	else
	{
		return 1;//账号不存在
	}
}
/*--注册 --*/
int server_Registe(user_account acot)
{
	Link s;
	printf("user_list.len:%d\n",user_list.len);
	printf("in server_Registe,the name is:%s\n",acot.name);
	if(LocateElem(user_list,acot)) //在账号列表中找到了账号
		return 1;//账号重复
	else
	{
		//printf("LocateElem shibai\n");
		acot.login=0;
		if(!MakeNode(&s,acot)) return 2;//创建用户节点失败
		//printf("MakeNode success!\n");
		if(!InsFirst(user_list.head,s)) return 3;//添加用户节点链表失败
		else 
		{(user_list.len)++; printf("user_list.len:%d\n",user_list.len); return 0;}//添加成功}
	}
}
//初始化server，创建套接字，绑定网络地址并监听，返回创建的套接字
//关键数据：套接字，套接字地址，监听数量
int server_init(const char* ip, int port)
{
	int server_sock;
	struct sockaddr_in server_addr;
	if((server_sock=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		fprintf(stderr,"Socket error:%s\n\a",strerror(errno));
		exit(1);
	}
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);//或INADDR_ANY
	if(bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr)))
	{
		fprintf(stderr,"bind error:%s\n\a",strerror(errno));
		close(server_sock); exit(1);
	}
	printf("my addr is %s:%hu\n",inet_ntoa(server_addr.sin_addr),ntohs(server_addr.sin_port));	
	if(listen(server_sock,BACKLOG))
	{
		fprintf(stderr,"listen error:%s\n\a",strerror(errno));
		close(server_sock); exit(1);
	}
	return server_sock;
}
/*--线程函数-*/
void* pthfunc(void* arg)
{
	int aimsock;
	struct sockaddr_in aimaddr;
	struct thread_parameter aim_para;
	int log_success=0;
	int slen,rlen;
	RL_pack sendpack,recvpack; //登录注册包类型
	user_account_response account_response;//用户账户回应类型
	user_account account;      //用户账户类型
	OperationPack beenpack;  //命令操作包接收
	typepack bigpack;					//通用传输包
	int runningmsg;  //指示运行状态信息返回值
	memset(&account,0,sizeof(user_account));
	memset(&sendpack,0,sizeof(RL_pack));
	memset(&recvpack,0,sizeof(RL_pack));
	memset(&account_response,0,sizeof(user_account_response));
	memset(&beenpack,0,sizeof(OperationPack));
	memset(&bigpack,0,sizeof(typepack));
	while(1)//获取线程参数，就是客户端的套接字和地址
	{
		printf("test Dequeue!\n");
		if(!QueueEmpty(ClientQueue))
		{
			if(DeQueue(&ClientQueue, &aim_para))
				{
					aimsock=aim_para.cli_sock;
					aimaddr=aim_para.cli_addr;
					break;
				}
				else
					continue;
		}
	}
//now you can use 	aimsock AND aimaddr
	printf(KRED "%s %d success connect..." RESET "\n",inet_ntoa(aimaddr.sin_addr),ntohs(aimaddr.sin_port));
	printf(KRED "the socket is %d" RESET "\n",aimsock);
	if(pthread_detach(pthread_self()))
	{printf("Error on pthread_detach!\n\a");}
	while(1)
	{
		while(!log_success)
	{
		if((rlen = newrecv(aimsock, &recvpack, sizeof(recvpack), 0))==-1)
			{{fprintf(stderr,"recv error:%s\n\a",strerror(errno)); continue;}}
		RL_unpack(&recvpack,(unsigned char *)&account);
		if(recvpack.head!=RL_HEAD)
		{printf("user do not login but server recv not RL_HEAD pack of packID:%d\n",recvpack.head);
		send_logout_massage(aimsock);break;
		/* running_thread_num--;close(aimsock); pthread_exit(NULL); */ }
		switch(recvpack.packID)
		{
			case CMD_REGIS:{if((runningmsg=server_Registe(account)))
								{
									switch(runningmsg)
									{
										case 1:{strcpy(account_response.result_msg,"The account is repeated!"); 
												account_response.result=0;break;}
										case 2:{strcpy(account_response.result_msg,"Defeated when MakeNode!"); 
												account_response.result=0;break;}
										case 3:{strcpy(account_response.result_msg,"Defeated when Append node to LinkedList!"); 
												account_response.result=0;break;}
										default:{strcpy(account_response.result_msg,"Unexpected error hanpenned!"); 
												account_response.result=0;break;}			
									}
								}
							else
							{	
								printf("%s:%s success registe!\n",account.name,account.passwd);
								strcpy(account_response.result_msg,"Registe Success!"); 
								account_response.result=1;
							}break;}
			case CMD_LOGIN: {if((runningmsg=server_Login(account,aimsock)))
								{
									switch(runningmsg)
									{
										case 1:{strcpy(account_response.result_msg,"The account is non-existent!"); 
												account_response.result=0;break;}
										case 2:{strcpy(account_response.result_msg,"Password Error!"); 
												account_response.result=0;break;}
										default:{strcpy(account_response.result_msg,"Unexpected error hanpenned!"); 
												account_response.result=0;break;}			
									}
								}
							else
							{	
								printf(KBLU "%s:%s" RESET "success login!\n",account.name,account.passwd);
								strcpy(account_response.result_msg,"Login Success!"); 
								account_response.result=1;
								log_success=1;
							}break;}
			case CMD_EXIT:{strcpy(account_response.result_msg,"Exit Success!"); account_response.result=1;
							strcpy(account_response.account_name,account.name); 
							RL_packup(&sendpack, CMD_RLRESULT,(const unsigned char *)&account_response, sizeof(account_response));
							if((slen = newsend(aimsock, &sendpack, sizeof(sendpack), 0))==-1)
								{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
							running_thread_num--;close(aimsock);pthread_exit(NULL); break;}
			default:{printf("get a wrong packID in !log_success!\n"); 
					strcpy(account_response.result_msg,"Your packID is wrong!"); 
					account_response.result=0;break;}	
		}	
		printf("account.name:%s,account.passwd:%s,\n",account.name,account.passwd);
		strcpy(account_response.account_name,account.name); 
		RL_packup(&sendpack, CMD_RLRESULT,(const unsigned char *)&account_response, sizeof(account_response));
		if((slen = newsend(aimsock, &sendpack, sizeof(sendpack), 0))==-1)
			{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
	}
	while(log_success)
	{	
		if((rlen = newrecv(aimsock, &bigpack, sizeof(bigpack), 0))==-1) 
		{{fprintf(stderr,"recv error:%s\n\a",strerror(errno)); break;}}
		switch(bigpack.head)
		{
			case RL_HEAD:{uppack(&bigpack, (unsigned char  *)&recvpack );break;}
			case OP_HEAD:{uppack(&bigpack, (unsigned char  *)&beenpack );break;}
			case CMD_CHATTING:{printf("Wrong:Get an CMD_CHATTING pack type!\n");continue;}
			case CMD_ABORTCHAT:{printf("Wrong:Get an CMD_ABORTCHAT pack type!\n");continue;}
			case CMD_P2A:{server_p2a(aimsock,bigpack.data);break;}
			case CMD_EXIT:{if(server_exit(aimsock)) 
								{running_thread_num--;pthread_exit(NULL); break;}}
			default: {printf("Wrong:Get an unknown pack type!\n");continue;}
		}
		if(bigpack.head == RL_HEAD)
		{
			RL_unpack(&recvpack,(unsigned char *)&account);
			switch(recvpack.packID)
			{
				case CMD_REGIS:{strcpy(account_response.result_msg,"you had cannot regis when you login!"); account_response.result=0;break;}							
				case CMD_LOGIN:{strcpy(account_response.result_msg,"you had already login!"); account_response.result=0;break;}			
				case CMD_LOGOUT:{if(runningmsg=server_logout(account,aimsock))
									switch(runningmsg)
									{
										case 1:{strcpy(account_response.result_msg,"The account have already logout!"); 
												account_response.result=0;break;}
										case 2:{strcpy(account_response.result_msg,"There is no the account!"); 
												account_response.result=0;break;}
										case 3:{strcpy(account_response.result_msg,"This is not your account!"); 
												account_response.result=0;break;}		
										default:{strcpy(account_response.result_msg,"Unexpected error hanpenned!"); 
												account_response.result=0;break;}	
									}
								else
									{	
										printf(KBLU "%s:%s" RESET "success logout!\n",account.name,account.passwd);
										strcpy(account_response.result_msg,"logout Success!"); 
										account_response.result=1;log_success=0;
									} break;}
				//case CMD_RLRESULT:{ }
				default:{strcpy(account_response.result_msg,"Your packID is wrong!"); account_response.result=0;break;}
			}
			strcpy(account_response.account_name,account.name);
			RL_packup(&sendpack, CMD_RLRESULT,(const unsigned char *)&account_response, sizeof(account_response));
			packup(&bigpack, sendpack.head, (unsigned char*)&sendpack, sizeof(sendpack));
			if((slen = newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
				{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}	
		}
		else if(bigpack.head == OP_HEAD)
		{
			switch(beenpack.packID)
			{
				case CMD_GETLIST:{if(runningmsg=server_getlist(aimsock, beenpack.flag))
									{
										switch(runningmsg)
										{case 1:  printf("GetElem Error!\n");break;
										case 2:  printf("send error!\n");break;
										default: printf("unexpected error!\n");break;}
									}break;}
				case CMD_P2P:{server_p2p(aimsock,beenpack.datalen,beenpack.data);break;}//
				case CMD_SENDFILE:{server_sendfile(aimsock,beenpack.flag,beenpack.datalen,beenpack.data);break;}//待补充状态信息
				case CMD_RECVFILE:{server_recvfile(aimsock,beenpack.flag,beenpack.datalen,beenpack.data);break;}
				case CMD_EXIT:{;}
				case CMD_P2A:{;}
				default:{printf("beenpack.packID is wrong!\n");
					beenpack.packID=CMD_OPRESULT; strcpy((char*)beenpack.data,"the beenpack.packID is wrong!\n"); beenpack.datalen=strlen((const char*)beenpack.data);
					packup(&bigpack, OP_HEAD, (unsigned char*)&beenpack, sizeof(OperationPack));
					if((slen = newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
						{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}break;}
			}
		//	OP_packup(&responpack, CMD_OPRESULT,  0, (const unsigned char *)&data, const unsigned char datalen);
		}
		else if(bigpack.head == CMD_P2A) {break;}
		else if(bigpack.head == CMD_EXIT) {break;}
		else
		{
			beenpack.packID=CMD_OPRESULT; strcpy((char*)beenpack.data,"the bigpack.head is wrong!\n"); beenpack.datalen=strlen((const char*)beenpack.data);
			packup(&bigpack, OP_HEAD, (unsigned char*)&beenpack, sizeof(OperationPack));
			if((slen = newsend(aimsock, &bigpack, sizeof(bigpack), 0))==-1)
				{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}	
		}
	}
	}
}
int main(int argc, char *argv[])
{
	int connsock;
	struct sockaddr_in conn_addr;
	int conn_sin_size;
	pthread_t pthid ;
	int rc=0;
	struct thread_parameter thfunc_para;
	if(argc!=2)
	{
		fprintf(stderr,"%s :The number of parameters specified does not match the expected number.\a\n",argv[0]);
        exit(1);//int atoi(char*)
	}
	if(!InitQueue(&ClientQueue))
	{
		printf("Error of InitQueue!\n\a");
		exit(1);
	}	
/* 	if(rc=pthread_mutex_init(&mylock,NULL))
	{
		fprintf(stderr,"Initialize mylock error:%s\a\n",strerror(rc));
		exit(1);
	} */
	if(!InitList(&user_list))
	{
		printf("Error of InitList!\n\a");
		exit(1);
	}
	serversock=server_init(argv[1],SERVPORT);
	memset(&conn_addr,0,sizeof(struct sockaddr));
	while(1)
	{	
		printf(KGRN "running_thread_num:" RESET "%d\n",running_thread_num);
		connsock=accept(serversock,(struct sockaddr*)&conn_addr,(socklen_t*)&conn_sin_size);
		if(connsock<0)
		{
			if(errno==EINTR)//accept在接收一个有效连接前被信号中断
			{continue;}
			else
			{
				fprintf(stderr,"Accept error:%s\n\a",strerror(errno));
                continue;//不停止
			}
		}
		printf(KYEL "%s:%d" RESET "success connect...\n",inet_ntoa(conn_addr.sin_addr),ntohs(conn_addr.sin_port));
		thfunc_para.cli_sock=connsock;
		thfunc_para.cli_addr=conn_addr;
		if(EnQueue(&ClientQueue,thfunc_para ))
		{
			if(rc=pthread_create(&pthid, NULL, pthfunc, NULL))//未完成
			{
				printf("Create thread error! the code is %d\n",rc);
				printf(KRED "close %s %d" RESET "\n",inet_ntoa(conn_addr.sin_addr),ntohs(conn_addr.sin_port));
				close(connsock);
				continue;
			}
			running_thread_num++;
			printf("pthread_create %d success!\n",running_thread_num);
		/*	if(rc=pthread_join(pthid, NULL))//未完成,NULL代表线程状态返回值不需要
				{
					printf("Join thread error! the code is %d\n",rc);
					printf("close %s %d \n",inet_ntoa(conn_addr.sin_addr),ntohs(conn_addr.sin_port));
					close(connsock);
					continue;
				}
			printf("pthread_join success!\n");*/
		}
		else
		{
			printf("Error on EnQueue\n");
			close(connsock); 
			break;
		}
	}
	close(serversock);
/* 	if(rc=pthread_mutex_destroy(&mylock))
	{
		fprintf(stderr,"Destory mylock error:%s\a\n",strerror(rc));
		exit(1);
	} */
	pthread_exit(NULL);
	return 0;
}
//int pthread_detach(pthread_t thread);
//int pthread_join(pthread_t thread,void **retval);














