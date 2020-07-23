#include "Protocol_LF.h"
#include <errno.h>

int packup(typepack *pack, unsigned short head, unsigned char  *data, const unsigned int datalen)
{
	int i;
	pack->head=head; 
	if(datalen>=sizeof(typepack)) return 0;
	for(i=0;i<datalen;i++) 
		if(data+i)
			(pack->data)[i]=data[i];
	return 1;
}
int uppack(typepack *pack, unsigned char  *buf )
{
	int i;
	if(pack->head==RL_HEAD)
		{for(i=0;i<sizeof(RL_pack);i++) buf[i]=(pack->data)[i]; return i;}
	else if(pack->head==OP_HEAD)
		{for(i=0;i<sizeof(OperationPack);i++) buf[i]=(pack->data)[i]; return i;}
	else return 0;
}

int OP_packup(OperationPack *pack, const unsigned char packID,  const unsigned char flag, const unsigned char *data, const unsigned char datalen)
{
	int i=1;
	pack->head = OP_HEAD; pack->packID = packID; pack->flag = flag;  pack->datalen=datalen;
	for(i=0;i<datalen;i++) 
		if(data+i)
			(pack->data)[i]=data[i];
	return 1;
}

int OP_unpack(const OperationPack *pack, unsigned char *flag, unsigned char *buf)
{
	int i; 
	if(flag)
		*flag=pack->flag;
	if(buf)
		for(i=0; i < (pack->datalen); i++) 
			buf[i]=(pack->data)[i];
	return 1;
}
int OP_get_length(const OperationPack *pack)
{
	return pack->datalen;
}

int OP_get_packID(const OperationPack *pack)
{
	return pack->packID;
}

int OP_get_packflag(const OperationPack *pack)
{
	return pack->flag;
}


int RL_packup(RL_pack *pack, const unsigned char packID,const unsigned char *data, const unsigned char datalen)
{
	int i=0;
	pack->head = RL_HEAD;
	pack->packID = packID;
	pack->datalen = datalen;
	for(i=0;i<datalen;i++) (pack->data)[i]=data[i];
//	(pack->data)[i]= '\0';
	return 1;
}

int RL_unpack(const RL_pack *pack, unsigned char *buf)
{
	int i; 
	for(i=0; i < (pack->datalen); i++) buf[i]=(pack->data)[i];
	return 1;
}

int RL_get_length(const RL_pack *pack)
{
	return pack->datalen;
}

int RL_get_packID(const RL_pack *pack)
{
	return pack->packID;
}

int newsend(int fd, const void *buf, unsigned int size, int flag)
{
   int total ,len;
   for(total = 0; total < size; total += len)
   {
		len = send(fd, buf+total, size-total, flag);
		if( len <= 0 )
		{
			if( len == -1 && errno == EINTR )
			{continue;}
			else
				return -1;
		}
		   }
	return total;
}

int newrecv(int fd, void *buf, unsigned int size, int flag)
{
	int total ,nread;
	for(total = 0; total < size; total += nread)
	{
		nread=recv(fd,buf+total,size-total,flag);
		if(nread==0)
			return total;
		if(nread == -1)
		{
			if(errno == EINTR)
			{continue;}
			else
				return -1;
		}
	}
	return total;
}




//-pthread -fno-stack-protector
