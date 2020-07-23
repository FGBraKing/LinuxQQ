#ifndef _PROTOCOL_LF_H_
#define _PROTOCOL_LF_H_

#include<arpa/inet.h>


#define RL_HEAD   0xABBA    //登录注册包协议类型ID
#define OP_HEAD   0x1025    //登录后包协议ID

/*应有的函数：打包，解包，取包长，取数据长，初始化*/


/*协议类型、协议长度、数据类型、数据长度*/

/*具体，登录：包类型、包长、用户ID、其它信息*/

/*协议中数据类型ID*/
typedef enum
{
CMD_REGIS = 0x01,	//注册Registe
CMD_LOGIN = 0x02,	//登录Login

CMD_LOGOUT = 0x05,	//注销Logout
CMD_GETLIST = 0x04,	//获取用户列表getList
CMD_P2P = 0x08,	//聊天p2p
CMD_P2A = 0x09,	//请求群聊p2a
CMD_SENDFILE=0x10,	//发送文件sendfile
CMD_RECVFILE=0x20,  //接收文件recvfile

CMD_EXIT=0x11,	//退出
CMD_EXIT_DEFEAD=0x88,//退出失败

CMD_RLRESULT=0x03,  //注册登录结果回应rlresult
CMD_OPRESULT=0x06,  //命令请求结果回应opresult
CMD_ABORTRESULT=0x77,//结束聊天回应
CMD_FLOGOUT=0x55,//强制下线


CMD_FILEING=0x77, //传输文件中
CMD_FILEENDING=0x99,//文件传输结束
CMD_CHATTING=0x25, //聊天中
CMD_ABORTCHAT=0x33,//结束聊天
MAX_PACK_ID = 0xFF	//命令ID最大个数
}EnumPackID;


/*---结构体---*/
/* //用户基本信息
typedef struct user_msg{
	int account_num;		//账户数目
	struct thread_parameter addr;//用户地址
}user_msg;
//用户结构体  
typedef struct user{
	user_msg massage;
	user_account *account;//为每个连入此服务器的IP设立一个结构体储存相关信息，后发现本实验似乎并不用用到，故在此注释，以后有时间再实现
}user; */

/* typedef	char datastr[50];
typedef char(*pdatastr)[50]; */


//用户账户
typedef struct {
    char name[50];
    char passwd[50];
	int   login;    //登录状态
	int   socket;   //关联的套接字
//	int   busying;
	int   chating;
//	int   fileing;
}user_account;
//用户账户回应
typedef struct {
    char account_name[50];
	char result;
    char result_msg[50];
}user_account_response;
//登录注册包类型结构体
typedef struct {
	unsigned short head;
	unsigned char packID;
	unsigned char datalen;
	unsigned char data[252];
}RL_pack;
//登录后注销 获取用户列表 聊天 群聊 发送文件 退出类型结构体
typedef struct {
	unsigned short head;
	unsigned char packID;
	unsigned char flag;
	unsigned char datalen;
	unsigned char data[50];
}OperationPack;
//通用pack
typedef struct{
	unsigned short head;
	unsigned char  data[1024];
//	int len;
}typepack;

typedef struct{
	unsigned char name[50];
	unsigned char msg[100];
}type_chat_msg;

int packup(typepack *pack, unsigned short head, unsigned char  *data, const unsigned int datalen);
int uppack(typepack *pack, unsigned char  *buf );

int OP_packup(OperationPack *pack, const unsigned char packID,  const unsigned char flag, const unsigned char *data, const unsigned char datalen);
int OP_unpack(const OperationPack *pack, unsigned char *flag, unsigned char *buf);
int OP_get_length(const OperationPack *pack);
int OP_get_packID(const OperationPack *pack);
int OP_get_packflag(const OperationPack *pack);

int RL_packup(RL_pack *pack, const unsigned char packID,const unsigned char *data, const unsigned char datalen);
int RL_unpack(const RL_pack *pack, unsigned char *buf);
int RL_get_length(const RL_pack *pack);
int RL_get_packID(const RL_pack *pack);


int newsend(int fd, const void *buf, unsigned int size, int flag);//finished
int newrecv(int fd, void *buf, unsigned int size, int flag);//finished


#endif // QUEUE_H_INCLUDED


