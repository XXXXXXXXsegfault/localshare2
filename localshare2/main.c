#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/iformat.c"
int _fseeki64(FILE *,long long pos,int orig);
long long _ftelli64(FILE *);
int rand_s(unsigned int *val);
char server_root[16384];
SOCKET server_sock;
void fatal(char *str)
{
	printf("Error: %s\nPlease press ENTER.",str);
	getchar();
	WSACleanup();
	exit(1);
}
void buf_gets(char *buf,int len)
{
	int c;
	while(len>1&&(c=getchar())!='\n')
	{
		*buf=c;
		--len;
		++buf;
	}
	while(c!='\n')
	{
		c=getchar();
	}
	*buf=0;
}
int string_to_addr(char *str,struct sockaddr_in *addr)
{
	char *p,c;
	unsigned long long val;
	int digits,state;
	p=str;
	digits=0;
	state=1;
	while(c=*p)
	{
		if(c>='0'&&c<='9')
		{
			++digits;
			if(digits>5)
			{
				return 1;
			}
			state=0;
		}
		else if(c=='.'||c==':')
		{
			digits=0;
			if(state)
			{
				return 1;
			}
			state=1;
		}
		else
		{
			return 1;
		}
		++p;
	}
	p=sinputi(str,&val);
	if(*p!='.'||val>255)
	{
		return 1;
	}
	addr->sin_addr.s_addr|=val;
	p=sinputi(p+1,&val);
	if(*p!='.'||val>255)
	{
		return 1;
	}
	addr->sin_addr.s_addr|=val<<8;
	p=sinputi(p+1,&val);
	if(*p!='.'||val>255)
	{
		return 1;
	}
	addr->sin_addr.s_addr|=val<<16;
	p=sinputi(p+1,&val);
	if(*p!=':'||val>255)
	{
		return 1;
	}
	addr->sin_addr.s_addr|=val<<24;
	p=sinputi(p+1,&val);
	if(*p||val>65535)
	{
		return 1;
	}
	addr->sin_port=htons(val);
	addr->sin_family=AF_INET;
	return 0;
}
int sock_read(SOCKET sock,void *buf,int size)
{
	int ret;
	ret=recv(sock,buf,size,0);
	if(ret<0)
	{
		ret=0;
	}
	return ret;
}
int sock_write(SOCKET sock,void *buf,int size)
{
	int ret;
	ret=send(sock,buf,size,0);
	if(ret<0)
	{
		ret=0;
	}
	return ret;
}
#include "server.c"
DWORD WINAPI T_server(LPVOID arg)
{
	SOCKET csock;
	HANDLE th;
	while(1)
	{
		csock=accept(server_sock,NULL,NULL);
		if(csock!=INVALID_SOCKET)
		{
			th=CreateThread(NULL,0,T_service,(void *)csock,0,NULL);
			if(th==NULL)
			{
				closesocket(csock);
			}
			else
			{
				CloseHandle(th);
			}
		}
	}
}
void server_init(void)
{
	char addr_string[32];
	static struct sockaddr_in addr;
	WIN32_FIND_DATAA fdata;
	HANDLE fh;
	printf("Server root directory path: ");
	buf_gets(server_root,16382);
	strcat(server_root,"/*");
	if((fh=FindFirstFileA(server_root,&fdata))==INVALID_HANDLE_VALUE)
	{
		fatal("Cannot open directory");
	}
	server_root[strlen(server_root)-2]=0;
	FindClose(fh);
	printf("Server address (format: 127.0.0.1:80): ");
	buf_gets(addr_string,32);
	if(string_to_addr(addr_string,&addr))
	{
		fatal("Invalid address");
	}
	server_sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(server_sock==INVALID_SOCKET)
	{
		fatal("Failed to initialize the server");
	}
	if(bind(server_sock,(void *)&addr,sizeof(addr)))
	{
		closesocket(server_sock);
		fatal("Failed to initialize the server");
	}
	if(listen(server_sock,128))
	{
		closesocket(server_sock);
		fatal("Failed to initialize the server");
	}
	if(!CreateThread(NULL,0,T_server,NULL,0,NULL))
	{
		closesocket(server_sock);
		fatal("Failed to initialize the server");
	}
	printf("Now you can access your files through http://%s\n",addr_string);
}

int main(void)
{
	WSADATA buf;
	WSAStartup(0x202,&buf);
	server_init();
	printf("Press ENTER to exit\n");
	getchar();
	WSACleanup();
	exit(0);
}
