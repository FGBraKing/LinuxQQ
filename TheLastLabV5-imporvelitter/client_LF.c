#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
//发送文件头文件
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "Protocol_LF.h"
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
/*-----------------------------------全局变量-------------------------------------*/
int login_success=0;
char local_name[50];

/*-------------------------------------------实现--------------------------------------------------------------------*/
void cmd_switch(int *server_sock);//finished
int client_init(const char* ip, int port);//finished
void client_login(int *server_sock); //finished
void client_Registe(int *server_sock); //finished
void client_Eixt(int *server_sock);//finished
void client_Help1();//finished
void client_Help2();//finished
void logout(int* server_sock);	//finished	
void getlist(int *server_sock);	//finished	
void chat(int *server_sock);//请求私聊	//finished		
void ChatGroup(int *server_sock);	//finished	
void SendFile(int *server_sock);
void RecvFile(int *server_sock);     
void chatting(int *server_sock);//正在聊天	
void chat_ending(int *server_sock);//finished
int chat_send_massage(int *server_sock, type_chat_msg msg);//finished
int chat_recv_massage(int *server_sock, type_chat_msg *msg, int *chat_recv_flag);//finished
void getmsgfrompack(typepack bigpack,type_chat_msg *msg);//finished
void sendingfile(int *server_sock, char filename[]);
void recvingfile(int *server_sock, char filename[]);
void trim_newline (char *text);//finished
void get_username (char *username);//finished


void get_username (char *username)
{
    while (1)
      {
          printf ("Please enter a username: " );
          fflush (stdout);
          memset (username, 0, 50);
          fgets (username, 50, stdin);
          trim_newline (username);
          if (strlen (username) > 48)
            {
                puts (KRED"Username must be 48 characters or less."RESET);
            }
          else
            {
                break;
            }
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
void getmsgfrompack(typepack bigpack,type_chat_msg *msg)//从包中解析出消息
{
	int i;
	unsigned char *buf=(unsigned char *)msg;
	for(i=0;i<sizeof(type_chat_msg);i++) buf[i]=bigpack.data[i];
}
int chat_recv_massage(int *server_sock, type_chat_msg *msg, int *chat_recv_flag)//聊天接收
{
	typepack bigpack;//收发的包类型
	memset(&bigpack,0,sizeof(bigpack));
	if((newrecv(*server_sock, &bigpack, sizeof(bigpack), 0))==-1) 
		{fprintf(stderr,"recv error:%s\n\a",strerror(errno));}
	if(bigpack.head!=CMD_CHATTING) 
	{
		*chat_recv_flag=0;
		if(bigpack.head==CMD_ABORTRESULT) 
		{
			printf(KRED "server massage:%s\n" RESET,(char*)bigpack.data);
		//	*chat_recv_flag=0;
			return 1;//服务器已终止聊天模式
		}
		else if(bigpack.head==CMD_FLOGOUT)
		{
		//	*chat_recv_flag=0;
			login_success=0;return 3;
		}
		else
		{printf(KRED "recv a bad bigpack.head of code:%d,stop chat-recving" RESET "\n\a",bigpack.head); return 2;}
	}
	getmsgfrompack(bigpack,msg);
	return 0;
}
int chat_send_massage(int *server_sock, type_chat_msg msg)//聊天发送
{
	typepack bigpack;//收发的包类型
	memset(&bigpack,0,sizeof(bigpack));	
	packup(&bigpack, CMD_CHATTING, (unsigned char *)&msg, sizeof(type_chat_msg));
	if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
	return 1;
}
void sendingfile(int *server_sock, char filename[])//读file，传入server_sock
{
	typepack bigpack;//收发的包类型
	char buf[BUFFSIZE];
	FILE* fd;
	int wlen,rlen;
	char respon_msg[50];//返回信息
	int *plen=(int*)(bigpack.data+BUFFSIZE);

	memset(&bigpack,0,sizeof(bigpack));	
	memset(buf,0,BUFFSIZE);	

	printf(KGRN "File sending start!" RESET "\n");
	fd=fopen(filename,"r");//rb
	if(fd==NULL)
	{
		printf(KWHT"%s" RESET " is non-exist!ending!\n",filename);
		bigpack.head=CMD_FILEENDING;strcpy(respon_msg,"file non-exist!ending!");
	//	printf("The file is non-exist!ending!");
		packup(&bigpack, CMD_FILEENDING, (unsigned char *)respon_msg, sizeof(char)*50);
		if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
		{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
		return;
	}
	else
	{
		bigpack.head=CMD_FILEING;
		if((rlen=fread(buf,1,BUFFSIZE,fd))<0)
			{fprintf(stderr,"fread error:%s\n\a",strerror(errno));}
		//printf("testing sendingfile,rlen:%d",rlen);
		//*plen=rlen;
		while(rlen>0)
		{
			//sleep(1);
			//buf[rlen]='\0';
			packup(&bigpack, CMD_FILEING, (unsigned char *)buf, rlen+1);
			*plen=rlen;
			if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
				{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
			memset(buf,0,BUFFSIZE);	
			if((rlen=fread(buf,1,BUFFSIZE,fd))<0)
				{fprintf(stderr,"fread error:%s\n\a",strerror(errno));}	
		//	printf("testing sendingfile,rlen:%d",rlen);
		}
		bigpack.head=CMD_FILEENDING;strcpy(respon_msg,"file sending end!");
		packup(&bigpack, CMD_FILEENDING, (unsigned char *)respon_msg, sizeof(char)*50);
		if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
		{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
	}
	if(fd)
		fclose(fd);
	printf(KGRN "File sending finish!" RESET "\n");
}
void recvingfile(int *server_sock, char savepath[])//从server_sock读，写入savepath
{
	typepack bigpack;//收发的包类型
	char buf[BUFFSIZE];
	FILE* fd;
	int rlen,wlen;
	int i;
	int len_data=0;
	int total=0;
	char respon_msg[50];//返回信息
	unsigned char  *temp_buf=(unsigned char *)bigpack.data;
	int *plen=(int*)(bigpack.data+BUFFSIZE);

	memset(&bigpack,0,sizeof(bigpack));	
	memset(buf,0,BUFFSIZE);
	fd=fopen(savepath,"w");//ab
	printf(KGRN "File recving start!" RESET "\n");
	while(1)
	{
		
			if((newrecv(*server_sock, &bigpack, sizeof(typepack), 0))==-1) 
				{fprintf(stderr,"recv error:%s\n\a",strerror(errno));}
			//sleep(1);
			if(bigpack.head!=CMD_FILEING)
			{
				if(bigpack.head==CMD_FILEENDING)
				{
					printf("recving end  massage:%s\n",(char*)bigpack.data);
				}
				break;
			}
			else
			{
				total=0;
				len_data=*plen;//strlen(bigpack.data);
				//printf("recvingfile:len_data=%d",len_data);
				for(i=0;i<len_data;i++)
				{ temp_buf[i]=bigpack.data[i];//'\0'xubuxuyao?
				buf[i]=temp_buf[i];
				//printf("%c",temp_buf[i]);
				}
				wlen=fwrite(buf,1,len_data,fd);
				//printf("testing recvingfile,wlen:%d",wlen);
				total+=wlen;
				while(wlen!=len_data)
				{
					if(wlen>len_data||wlen<0) {fclose(fd);return;}
					else
					{
						len_data=len_data-wlen;
						wlen=fwrite(buf+total,1,len_data,fd);
						total+=wlen;
						if(wlen==0) {break;}
					}		
				}
			}
			memset(buf,0,BUFFSIZE);
			
	}
	fclose(fd);
	printf(KGRN "File recving finish!" RESET "\n");

}
void chatting(int *server_sock)
{
	pid_t pid;
	type_chat_msg mine,friend;
	int runningstatecode;
	int chat_recv_flag=1;
//	char massageg[100];
	strcpy(mine.name,local_name);
	printf("------------------------------------------------------------\n");
	printf("Welcome to the Chatting room!\n");
	pid=fork();
	if(pid<0)
	{
		printf("There is an unknown error happened\n");
		exit(1);
	}
	else if(pid==0)
	{
		while(1)
		{
			runningstatecode=chat_recv_massage(server_sock, &friend, &chat_recv_flag);
			if(runningstatecode==0)
				printf(KWHT "%s:" KCYN "%s\n" RESET,friend.name,friend.msg);
			else if(runningstatecode==1||runningstatecode==3)
				exit(1);
			else
				exit(1);//continue;
		}
	}
	else
	{
		//wait(NULL);
		while(1)
		{
			//printf("%s(P):",mine.name);
			if(chat_recv_flag==0) break;
			fgets(mine.msg,100,stdin);
			trim_newline(mine.msg);
			if(strcmp(mine.msg,"/exit"))
				chat_send_massage(server_sock, mine);
			else
			{chat_ending(server_sock);break;}
		}
	}
}
void chat_ending(int *server_sock)
{
	typepack bigpack;//收发的包类型
	memset(&bigpack,0,sizeof(bigpack));	
	packup(&bigpack, CMD_ABORTCHAT, NULL, 0);
	if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
	printf(KBLU "leaving the Chatting room!\n" RESET);
}
void logout(int* server_sock)
{
	user_account account;
	user_account_response account_response;
	RL_pack sendpack,recvpack;
	typepack bigpack;//收发的包类型
	int slen,rlen;
	char temp_password[50];
	
	memset(&bigpack,0,sizeof(bigpack));
	memset(&account,0,sizeof(user_account));
	memset(&sendpack,0,sizeof(RL_pack));
	memset(&recvpack,0,sizeof(RL_pack));
	memset(&account_response,0,sizeof(user_account_response));
	
	get_username(account.name);
/* 	printf("Please input your account passwd:");
	fgets(account.passwd,50,stdin);
	trim_newline(account.passwd); */
	printf("Please wait a minute...\n");
	RL_packup(&sendpack, CMD_LOGOUT,(const unsigned char *)&account, sizeof(account));
	packup(&bigpack, sendpack.head, (unsigned char*)&sendpack, sizeof(sendpack));
	if((slen = newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno)); return;}
/*----------------------------------------------打包发送完成-----------------------*/

	if((rlen = newrecv(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"recv error:%s\n\a",strerror(errno)); return;}

	switch(bigpack.head)
	{
		case RL_HEAD:{uppack(&bigpack, (unsigned char  *)&recvpack );break;}
		case OP_HEAD:{printf(KRED "It is not expected to use OP_pack data!\n"  RESET);return;}
		case CMD_FLOGOUT:{printf(KRED "Force logout !\n" RESET);login_success=0;return;}
		default: {printf(KRED "you had used an unknown pack type:%d!\n" RESET,bigpack.head);return;}
	}
	switch(recvpack.packID)
	{
		case CMD_REGIS: {printf(KRED "Get a registe pack,wrong!\n" RESET);return;}
		case CMD_LOGIN: {printf(KRED "Get a login pack,wrong!\n" RESET);return;}
		case CMD_RLRESULT:{RL_unpack(&recvpack, (unsigned char *)&account_response);break;}
		default: {printf(KRED "Get a illegal pack,wrong!\n" RESET);return;}	
	}
	if(account_response.result)
	{
		printf(KGRN "%s logout susseed\n" RESET,account_response.account_name);
		login_success=0;
	}
	else
	{
		printf(KRED "%s logout defeated with the massage:%s\n\a" RESET,account_response.account_name,account_response.result_msg);
	}
}
void getlist(int *server_sock)
{
	OperationPack sendpack;
	typepack bigpack;//收发的包类型
	char uselist[20][50];
	int i;
	unsigned char  *buf;
	int usernum;
	type_chat_msg msg;
	memset(&bigpack,0,sizeof(typepack));
	memset(&sendpack,0,sizeof(OperationPack));
//静态初始化sendpack，数据域不使用，因为暂时没想到获取用户列表还需要什么信息，以后可扩充	
	sendpack.head = OP_HEAD;
	sendpack.packID = CMD_GETLIST;
	sendpack.flag = 1;//1-在线用户，0-所有用户
	sendpack.datalen=0;
	packup(&bigpack, sendpack.head, (unsigned char *)&sendpack, sizeof(OperationPack));
	if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno)); return ;}
	//printf("sizeof(bigpack):%d\n",sizeof(bigpack));
	if((newrecv(*server_sock, &bigpack, sizeof(bigpack), 0))==-1) 
		{fprintf(stderr,"recv error:%s\n\a",strerror(errno));}
	
	buf=(unsigned char*)uselist;
	for(i=0;i<sizeof(char)*50*20;i++)
		buf[i]=bigpack.data[i];
	usernum=bigpack.data[i];
	for(i=0;i<usernum;i++) 
		printf(KMAG "the %d account'name:%s\n" RESET,i+1,uselist[i]);
}
void chat(int *server_sock)
{
	OperationPack sendpack,recvpack;
	typepack bigpack;//收发的包类型
	char username[50];
	char respon_msg[50];//返回信息
	type_chat_msg msg;
	
	printf("Please input your friend's name:");
	fflush(stdout);
	fgets(username,50,stdin);
	trim_newline(username);
	
	memset(&bigpack,0,sizeof(typepack));
	memset(&sendpack,0,sizeof(OperationPack));
	memset(&recvpack,0,sizeof(OperationPack));
	OP_packup(&sendpack, CMD_P2P, 0, (const unsigned char *)username, strlen(username)+1);//把 \0带上
	packup(&bigpack, sendpack.head, (unsigned char *)&sendpack, sizeof(OperationPack));
	if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno)); return ;}

	while(1){
	if((newrecv(*server_sock, &bigpack, sizeof(bigpack), 0))==-1) 
		{fprintf(stderr,"recv error:%s\n\a",strerror(errno));}
	if(bigpack.head==CMD_CHATTING) 
	{
		getmsgfrompack(bigpack,&msg);
		printf("%s:%s\n",msg.name,msg.msg);continue;
	}
	else if(bigpack.head==OP_HEAD) 
	{uppack(&bigpack, (unsigned char  *)&recvpack );break;}
	switch(bigpack.head)
	{
		case RL_HEAD:{printf("It is not expected to use RL_pack data!\n");return;}
	//	case OP_HEAD:{uppack(&bigpack, (unsigned char  *)&recvpack );break;}
		case CMD_FLOGOUT:{printf("fause logout !\n");login_success=0;return;}
		default: {printf("in chat:you had used an unknown pack type:%d!\n",bigpack.head);return;}
	}
	}
	
	if(recvpack.packID!=CMD_OPRESULT) {printf("receive a wrong pack type!,code:%d\n",recvpack.packID);return;}
	OP_unpack(&recvpack, NULL, (unsigned char *)&respon_msg);
	if(recvpack.flag==0){printf("the return massage is %s\n",respon_msg);return;}
	else {printf(KGRN "Success connect %s!\n" RESET,username);	chatting(server_sock);}
}
void ChatGroup(int *server_sock)
{
	OperationPack sendpack,recvpack;
	typepack bigpack;//收发的包类型
	char username[20][50];
	char respon_msg[50];//返回信息
	type_chat_msg msg;//聊天信息	
	int number;
	int i;
	memset(&username,'\0',sizeof(char)*20*50);
	memset(&bigpack,0,sizeof(typepack));
	memset(&sendpack,0,sizeof(OperationPack));
	memset(&recvpack,0,sizeof(OperationPack));
	while(1)
	{
		printf("Please input the num (<20)you want to send:");
		fflush(stdout);
		scanf("%d",&number);
		if(number<=20&&number>=0) break; 
	}
	for(i=0;i<number;i++)//指定用户发送消息
	{
		printf("The %d name:",i+1);
		fgets(username[i],50,stdin);
		trim_newline(username[i]);
	//	printf("what you input is :%s\n",username[i]);
	}
	packup(&bigpack, CMD_P2A, (unsigned char *)username, sizeof(char)*20*50);
	if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno));return;}
	while(1){
	if((newrecv(*server_sock, &bigpack, sizeof(bigpack), 0))==-1) 
		{fprintf(stderr,"recv error:%s\n\a",strerror(errno));return;}
	if(bigpack.head==CMD_CHATTING) 
	{
		getmsgfrompack(bigpack,&msg);
		printf("%s:%s\n",msg.name,msg.msg);continue;
	}
	break;
	}
	switch(bigpack.head)
	{
		case RL_HEAD:{printf(KRED "It is not expected to use OP_pack data!\n" RESET);return;}
		case OP_HEAD:{uppack(&bigpack, (unsigned char  *)&recvpack );break;}
		case CMD_FLOGOUT:{printf(KRED "force logout !\n" RESET);login_success=0;return;}
		default: {printf(KRED "you had used an unknown pack type:%d!\n" RESET,bigpack.head);return;}
	}

	switch(recvpack.packID)
	{
		case CMD_OPRESULT: {OP_unpack(&recvpack, NULL, (unsigned char *)&respon_msg);break;}
		case CMD_LOGIN: {printf(KRED "Get a login pack,wrong!\n" RESET);return;}
		case CMD_RLRESULT:{printf(KRED "Get a RLRESULT pack,wrong!\n" RESET);return;}
		default: {printf(KRED "receive a wrong pack type:%d!\n" RESET,recvpack.packID);return;}	
	}		
	if(recvpack.flag==0){printf("chat false,the return msg is %s\n",respon_msg);return;}
	else {printf(KGRN "Success connect your friend!\n" RESET);	chatting(server_sock);}	
}
void RecvFile(int *server_sock)
{
	OperationPack sendpack,recvpack;
	typepack bigpack;//收发的包类型
	//char cmd[10];
	char savepath[100];
	char filename[100];
	char respon_msg[50];//返回信息
	char flag;//是否指定保存路径？
	type_chat_msg msg;
	
	memset(&bigpack,0,sizeof(typepack));
	memset(&sendpack,0,sizeof(OperationPack));
	memset(&recvpack,0,sizeof(OperationPack));
	
	printf("Please enter the filename:");
	fflush (stdout);
    fgets (filename, 100, stdin);
    trim_newline (filename);
	
/* 	while(1)
	{
		printf("would you like to point the savepath? yes or no:");
		fflush (stdout);
		fgets (cmd, 10, stdin);
		trim_newline (cmd);
		if (strcmp(cmd,"yes")==0) 
		{
			printf("Please enter the savepath:");
			fflush (stdout);
			fgets (savepath, 100, stdin);
			trim_newline (savepath);	
			break;
		}
		else if(strcmp(cmd,"no")==0)
			{strcpy(savepath,"~/");break;}
		else 
		{printf("What you input is illegel,please input again!\n");}			
	} */
	printf("Please enter the savepath:");
	fflush (stdout);
	fgets (savepath, 100, stdin);
	trim_newline (savepath);	
	flag=0;
	
	OP_packup(&sendpack, CMD_RECVFILE, flag, filename, strlen(filename)+1);//flag不用
	packup(&bigpack, sendpack.head, (unsigned char *)&sendpack, sizeof(OperationPack));
	if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno)); return ;}
	//sleep(1);

//	while(1){
	if((newrecv(*server_sock, &bigpack, sizeof(bigpack), 0))==-1) 
		{fprintf(stderr,"recv error:%s\n\a",strerror(errno));return;}
//	if(bigpack.head==CMD_CHATTING) 
//	{
//		getmsgfrompack(bigpack,&msg);
//		printf("%s:%s\n",msg.name,msg.msg);continue;
//	}
//	break;
//	}
	switch(bigpack.head)
	{
		case RL_HEAD:{printf("It is not expected to use RL_pack data!\n");return;}
		case OP_HEAD:{uppack(&bigpack, (unsigned char  *)&recvpack );break;}
		case CMD_FLOGOUT:{printf("fause logout !\n");login_success=0;return;}
		default: {printf("you had used an unknown pack type!\n");return;}
	}
	while(1)
	{
		if(recvpack.packID!=CMD_OPRESULT) 
			{printf(KRED "receive a wrong pack type!:%d\n" RESET,recvpack.packID);return;}
		else
		{
			OP_unpack(&recvpack, NULL, (unsigned char *)&respon_msg);break;
		}
	}
	if(recvpack.flag==0){printf("the return msg is %s\n",respon_msg);return;}
	else {printf(KGRN "Start to recv file!\n" RESET);	recvingfile(server_sock,savepath);}
}
void SendFile(int *server_sock)
{
	OperationPack sendpack,recvpack;
	typepack bigpack;//收发的包类型
	//char cmd[10];
	char savepath[100];
	char filename[100];
	char respon_msg[50];//返回信息
	char flag;//是否指定保存路径？
	type_chat_msg msg;
	
	memset(&bigpack,0,sizeof(typepack));
	memset(&sendpack,0,sizeof(OperationPack));
	memset(&recvpack,0,sizeof(OperationPack));
	
	printf("Please enter the filename:");
	fflush (stdout);
    fgets (filename, 100, stdin);
    trim_newline (filename);
	
/* 	while(1)
	{
		printf("would you like to point the savepath? yes or no:");
		fflush (stdout);
		fgets (cmd, 10, stdin);
		trim_newline (cmd);
		if (strcmp(cmd,"yes")==0) 
		{
			printf("Please enter the savepath:");
			fflush (stdout);
			fgets (savepath, 100, stdin);
			trim_newline (savepath);	
			break;
		}
		else if(strcmp(cmd,"no")==0)
			{strcpy(savepath,"~/");break;}
		else 
		{printf("What you input is illegel,please input again!\n");}			
	} */
	printf("Please enter the savepath:");
	fflush (stdout);
	fgets (savepath, 100, stdin);
	trim_newline (savepath);	
	flag=0;
	OP_packup(&sendpack, CMD_SENDFILE, flag, savepath, strlen(savepath)+1);
	packup(&bigpack, sendpack.head, (unsigned char *)&sendpack, sizeof(OperationPack));
	if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno)); return ;}
	//sleep(1);

	while(1){
	if((newrecv(*server_sock, &bigpack, sizeof(bigpack), 0))==-1) 
		{fprintf(stderr,"recv error:%s\n\a",strerror(errno));return;}
	if(bigpack.head==CMD_CHATTING) 
	{
		getmsgfrompack(bigpack,&msg);
		printf("%s:%s\n",msg.name,msg.msg);continue;
	}
	break;
	}
	switch(bigpack.head)
	{
		case RL_HEAD:{printf("It is not expected to use RL_pack data!\n");return;}
		case OP_HEAD:{uppack(&bigpack, (unsigned char  *)&recvpack );break;}
		case CMD_FLOGOUT:{printf("fause logout !\n");login_success=0;return;}
		default: {printf("you had used an unknown pack type:%d!\n",bigpack.head);return;}
	}
	while(1)
	{
		if(recvpack.packID!=CMD_OPRESULT) 
			{printf(KRED "receive a wrong pack type:%d!\n" RESET,recvpack.packID);return;}
		else
		{
			OP_unpack(&recvpack, NULL, (unsigned char *)&respon_msg);break;
		}
	}
	if(recvpack.flag==0){printf("the return msg is %s\n",respon_msg);return;}
	else {printf(KYEL "Start to send file!\n" RESET);	sendingfile(server_sock,filename);}
}
void client_login(int *server_sock)
{
	user_account account;
	user_account_response account_response;
	RL_pack sendpack,recvpack;
	int slen,rlen;
	char temp_password[50];
	memset(&account,0,sizeof(user_account));
	memset(&sendpack,0,sizeof(RL_pack));
	memset(&recvpack,0,sizeof(RL_pack));
	memset(&account_response,0,sizeof(user_account_response));
	//getchar();
	while(1)
	{	
		get_username(account.name);
		//printf("account.name:%s\n",account.name);
		printf("Please input your password:");
		fflush(stdout);
		fgets(account.passwd,50,stdin);
		trim_newline(account.passwd);
		//printf("account.passwd:%s\n",account.passwd);
		printf("Please retype your password again:");
		fflush(stdout);
		fgets(temp_password,50,stdin);
		trim_newline(temp_password);
		if(strcmp(temp_password,account.passwd))
			printf(KRED "what you input is diffient of the last passwd!\n\a" RESET);
		else
			break;
	}
	account.login=1;
	printf("Please wait a minute...\n");
	RL_packup(&sendpack, CMD_LOGIN,(const unsigned char *)&account, sizeof(account));
	if((slen = newsend(*server_sock, &sendpack, sizeof(sendpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno)); return;}

	if((rlen = newrecv(*server_sock, &recvpack, sizeof(recvpack), 0))==-1)
	{{fprintf(stderr,"recv error:%s\n\a",strerror(errno)); return;}}
	if(recvpack.head!=RL_HEAD) return;
	switch(recvpack.packID)
	{
		case CMD_REGIS: {printf("Get a registe pack,wrong!\n");return;}
		case CMD_LOGIN: {printf("Get a login pack,wrong!\n");return;}
		case CMD_RLRESULT:{RL_unpack(&recvpack, (unsigned char *)&account_response);break;}
		default: {printf("Get a illegal pack,wrong!\n");return;}	
	}
	if(account_response.result)
	{
		printf(KYEL "%s login susseed\n" RESET,account_response.account_name);
		login_success=1;
		strcpy(local_name,account.name);
		client_Help2();
	}
	else
	{
		printf(KRED "%s login defeated with the massage:%s\n" RESET,account_response.account_name,account_response.result_msg);
		login_success=0;
	}
}
void client_Registe(int *server_sock)
{
	user_account account;
	user_account_response account_response;
	RL_pack sendpack,recvpack;
	int slen,rlen;
	char temp_password[50];
	memset(&account,0,sizeof(user_account));
	memset(&sendpack,0,sizeof(RL_pack));
	memset(&recvpack,0,sizeof(RL_pack));
	memset(&account_response,0,sizeof(user_account_response));
	//getchar();
	while(1)
	{	
		get_username(account.name);
		//printf("account.name:%s\n",account.name);
		printf("Please input your password:");
		fflush(stdout);
		fgets(account.passwd,50,stdin);
		trim_newline(account.passwd);
		//printf("account.passwd:%s\n",account.passwd);
		printf("Please retype your password again:");
		fflush(stdout);
		fgets(temp_password,50,stdin);
		trim_newline(temp_password);
		if(strcmp(temp_password,account.passwd))
			printf(KRED "what you input is diffient of the last passwd!\n\a" RESET);
		else
			break;
	}
	account.login=0;
	printf("Please wait a minute...\n");
	RL_packup(&sendpack, CMD_REGIS,(const unsigned char *)&account, sizeof(account));
	if((slen = newsend(*server_sock, &sendpack, sizeof(sendpack), 0))==-1)
	{fprintf(stderr,"Send error:%s\n\a",strerror(errno)); return;}

	if((rlen = newrecv(*server_sock, &recvpack, sizeof(recvpack), 0))==-1)
	{{fprintf(stderr,"recv error:%s\n\a",strerror(errno)); return;}}
	if(recvpack.head!=RL_HEAD) return;
	switch(recvpack.packID)
	{
		case CMD_REGIS: {printf("Get a registe pack,wrong!\n");return;}
		case CMD_LOGIN: {printf("Get a login pack,wrong!\n");return;}
		case CMD_RLRESULT:{RL_unpack(&recvpack, (unsigned char *)&account_response);break;}
		default: {printf("Get a illegal pack with the ID of %d,wrong!\n",recvpack.packID);return;}	
	}
	if(account_response.result)
	{
		printf(KGRN "%s Registe susseed\n" RESET,account_response.account_name);
	}
	else
	{
		printf(KRED "%s Registe defeated with the massage:%s\n" RESET,account_response.account_name,account_response.result_msg);
	}
}
void client_Eixt(int *server_sock)
{
	if(!login_success)
	{
		user_account account;
		user_account_response account_response;
		RL_pack sendpack,recvpack;
	
		memset(&account,0,sizeof(user_account));
		memset(&sendpack,0,sizeof(RL_pack));
		memset(&recvpack,0,sizeof(RL_pack));
		memset(&account_response,0,sizeof(user_account_response));
		//getchar();
	
		strcpy(account.name,"HandsomeBoy");
		account.login=0;
		
		printf("process exiting...\n");
		RL_packup(&sendpack, CMD_EXIT,(const unsigned char *)&account, sizeof(account));
		if((newsend(*server_sock, &sendpack, sizeof(sendpack), 0))==-1)
		{fprintf(stderr,"Send error:%s\n\a",strerror(errno)); return;}
	
		if((newrecv(*server_sock, &recvpack, sizeof(recvpack), 0))==-1)
		{{fprintf(stderr,"recv error:%s\n\a",strerror(errno)); return;}}
		if(recvpack.head!=RL_HEAD) return;
		switch(recvpack.packID)
		{
			case CMD_REGIS: {printf("Get a registe pack,wrong!\n");return;}
			case CMD_LOGIN: {printf("Get a login pack,wrong!\n");return;}
			case CMD_RLRESULT:{RL_unpack(&recvpack, (unsigned char *)&account_response);break;}
			default: {printf("Get a illegal pack with the ID of %d,wrong!\n",recvpack.packID);return;}	
		}
		if(account_response.result)
		{
			printf("%s exit susseed\n",account_response.account_name);
			close(*server_sock);
			exit(1);
		}
		else
		{
			printf(KRED "%s exit defeated with the massage:%s\n" RESET,account_response.account_name,account_response.result_msg);
		}
	}
	else
	{
		typepack bigpack;	//通用传输包
		memset(&bigpack,0,sizeof(typepack));
		bigpack.head=CMD_EXIT;
		if((newsend(*server_sock, &bigpack, sizeof(bigpack), 0))==-1)
		{fprintf(stderr,"Send error:%s\n\a",strerror(errno));}
		
		if((newrecv(*server_sock, &bigpack, sizeof(bigpack), 0))==-1) 
		{fprintf(stderr,"recv error:%s\n\a",strerror(errno));}
		if(bigpack.head)
		{printf("success:%s\n",bigpack.data);close(*server_sock);
		exit(1);}
		else
			printf("defeated:%s\n",bigpack.data);
	}
	
	
}
void client_Help1()
{
	printf(KBLU "--------------------------waiting for your login----------------------------------\n" RESET);
	printf("Welcome to the Linux QQ!\n");
	printf("When you want to \"login\", you should input 'L/l'\n");
	printf("When you want to \"registe\", you should input 'R/r'\n");
	printf("When you want to \"exit\", you should input 'E/e'\n");
	printf("You can type h/H for help.\n");
	printf(KBLU "-------------------------------------------------------------------------------\n" RESET);
}
void client_Help2()
{
	printf(KBLU "-------------------------welcome you! %s--------------------------------\n" RESET,local_name);
	printf("WHen you what to logout, you should input 'logout'\n");
	printf("WHen you what to see the online people, you should input 'getlist'\n");
	printf("WHen you what to chat with someone, you should input 'chat'\n");
	printf("WHen you what to chat with few man, you should input 'ChatGroup'\n");
	printf("WHen you what to send file, you should input 'SendFile'\n");
	printf("WHen you what to recv file, you should input 'RecvFile'\n");
	printf("WHen you what to exit the progrom, you should input 'exit'\n");
	//printf("WHen you what to Register an account, you should input 'Register'\n");
	printf("WHen you what to get the help, you should input 'help'\n");
	printf(KBLU "-------------------------------------------------------------------------------\n" RESET);
}
//登录或注册的选择界面
void cmd_switch(int *server_sock)
{
	char cmd;
	char command[15];
	if(!login_success)
		client_Help1();
	else
		client_Help2();
	while(1)
	{
		if(!login_success)
		{
			printf("Please input a letter to select a function\n");
			cmd=getchar();
			//scanf("%c",&cmd);
			printf("the cmd is %c\n",cmd);
			fflush(stdin);
			getchar();
			switch(cmd)
			{
				case 'L': client_login(server_sock);break;
				case 'l': client_login(server_sock);break;
				case 'R': client_Registe(server_sock);break;
				case 'r': client_Registe(server_sock);break;
				case 'E': client_Eixt(server_sock);break;
				case 'e': client_Eixt(server_sock);break;
				case 'H': client_Help1();break;
				case 'h': client_Help1();break;
				default: printf("you had inputed a wrong letter,you can type H/h for help.please input again!\n");break;
			}
		}
		else
		{
			printf("Please input the command to select a function\n");
			fgets(command,15,stdin);
			trim_newline (command);
			if(!strcmp(command,"logout")) 			{ logout(server_sock);			} 
			else if(!strcmp(command,"getlist")) 	{ getlist(server_sock);		} 
			else if(!strcmp(command,"chat")) 		{ chat(server_sock);			} 
			else if(!strcmp(command,"ChatGroup")) 	{ ChatGroup(server_sock);		} 
			else if(!strcmp(command,"SendFile")) 	{ SendFile(server_sock);		} 
			else if(!strcmp(command,"help")) 		{ client_Help2();	} 
		//	else if(!strcmp(command,"chatting")) 	{ chatting(server_sock);	} 
			else if(!strcmp(command,"exit")) 		{ client_Eixt(server_sock);	} 
			else if(!strcmp(command,"RecvFile")) 	{ RecvFile(server_sock);	} 
			else {printf("what you input is wrong, please input again!\n");}
		}
	}
}
//套接字初始化，连接远程服务器
int client_init(const char* ip, int port)
{
	int conn_sock;
	struct sockaddr_in server_addr;
	if((conn_sock=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		fprintf(stderr,"Socket error:%s\n\a",strerror(errno));
		exit(1);
	}
    memset(&server_addr, 0, sizeof(struct sockaddr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
	
	//sleep(1);
	if(connect(conn_sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1)//将sfd连接至制定的服务器网络地址serveraddr
    {
		fprintf(stderr,"connect error:%s\n\a",strerror(errno));
		close(conn_sock);
		exit(-1);
    }
	printf(KYEL "Success connect %s %d \n" RESET,inet_ntoa(server_addr.sin_addr),ntohs(server_addr.sin_port));
	return conn_sock;
}
//创建远程套接字，并进入命令选择界面
int main(int argc, char *argv[])
{
	int server_sock;
	if(argc!=2)
	{
		fprintf(stderr,"%s :The number of parameters specified does not match the expected number.\a\n",argv[0]);
        exit(1);//int atoi(char*)
	}
	server_sock=client_init(argv[1],SERVPORT);
	cmd_switch(&server_sock);
	return 0;
}
