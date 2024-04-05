#include "../include/windows.c"
#include "../include/mem.c"
#include "../include/iformat.c"
char server_root[16384];
void *server_sock;
void fatal(char *str)
{
	puts("Error: ");
	puts(str);
	puts("\nPress any key to continue\n");
	getch();
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
	long val;
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
	addr->addr|=val;
	p=sinputi(p+1,&val);
	if(*p!='.'||val>255)
	{
		return 1;
	}
	addr->addr|=val<<8;
	p=sinputi(p+1,&val);
	if(*p!='.'||val>255)
	{
		return 1;
	}
	addr->addr|=val<<16;
	p=sinputi(p+1,&val);
	if(*p!=':'||val>255)
	{
		return 1;
	}
	addr->addr|=val<<24;
	p=sinputi(p+1,&val);
	if(*p||val>65535)
	{
		return 1;
	}
	addr->port=htons(val);
	addr->family=AF_INET;
	return 0;
}
int sock_read(void *sock,void *buf,int size)
{
	struct fd_set sfd;
	long timeout[2];
	int ret;
	timeout[0]=15;
	timeout[1]=0;
	sfd.nfds=1;
	sfd.pad=0;
	sfd.sock[0]=sock;
	if(select(1,&sfd,NULL,&sfd,timeout)!=1)
	{
		return 0;
	}
	ret=recv(sock,buf,size,0);
	if(ret<0)
	{
		ret=0;
	}
	return ret;
}
int sock_read_nowait(void *sock,void *buf,int size)
{
	struct fd_set sfd;
	long timeout[2];
	int ret;
	timeout[0]=0;
	timeout[1]=0;
	sfd.nfds=1;
	sfd.pad=0;
	sfd.sock[0]=sock;
	if(select(1,&sfd,NULL,&sfd,timeout)!=1)
	{
		return 0;
	}
	ret=recv(sock,buf,size,0);
	if(ret<0)
	{
		ret=0;
	}
	return ret;
}
int sock_write(void *sock,void *buf,int size)
{
	struct fd_set sfd;
	long timeout[2];
	int ret;
	timeout[0]=15;
	timeout[1]=0;
	sfd.nfds=1;
	sfd.pad=0;
	sfd.sock[0]=sock;
	if(select(1,NULL,&sfd,&sfd,timeout)!=1)
	{
		return 0;
	}
	ret=send(sock,buf,size,0);
	if(ret<0)
	{
		ret=0;
	}
	return ret;
}
void sock_clean(void *sock)
{
	struct fd_set sfd;
	long timeout[2];
	char buf[4096];
	while(1)
	{
		timeout[0]=10;
		timeout[1]=0;
		sfd.nfds=1;
		sfd.pad=0;
		sfd.sock[0]=sock;
		if(select(1,&sfd,NULL,&sfd,timeout)!=1)
		{
			return;
		}
		if(recv(sock,buf,4096,0)<=0)
		{
			return;
		}
	}
}
#include "server.c"
void _T_service(void); // SCC uses a different calling convention
asm "@_T_service"
asm "push %rcx"
asm "call @T_service"
asm "pop %rcx"
asm "ret"
void T_server(void)
{
	void *csock,*th;
	while(1)
	{
		csock=accept(server_sock,NULL,NULL);
		if(csock!=INVALID_SOCKET)
		{
			th=CreateThread(NULL,0,_T_service,csock,0,NULL);
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
	struct win32_find_data_a fdata;
	void *fh;
	puts("Server root directory path: ");
	buf_gets(server_root,16382);
	strcat(server_root,"/*");
	if((fh=FindFirstFileA(server_root,&fdata))==INVALID_HANDLE_VALUE)
	{
		fatal("Cannot open directory");
	}
	server_root[strlen(server_root)-2]=0;
	FindClose(fh);
	puts("Server address (format: 127.0.0.1:80): ");
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
	if(bind(server_sock,&addr,sizeof(addr)))
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
	puts("Now you can access your files through http://");
	puts(addr_string);
	puts("\n");
}

int main(void)
{
	server_init();
	puts("Press any key to exit\n");
	getch();
	return 0;
}
