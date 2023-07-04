char *read_header(void *sock,int *status)
{
	char *buf;
	int x,state;
	buf=malloc(8192);
	if(buf==NULL)
	{
		*status=2; // server error
		return NULL;
	}
	x=0;
	state=0;
	while(1)
	{
		if(sock_read(sock,buf+x,1)!=1)
		{
			free(buf);
			*status=3;
			return NULL;
		}
		if(buf[x]=='\r')
		{
			++state;
		}
		else if(buf[x]=='\n')
		{
			if(state==2)
			{
				buf[x+1]=0;
				return buf;
			}
		}
		else
		{
			state=0;
		}
		++x;
		if(x==8191)
		{
			free(buf);
			*status=1; // client error
			return NULL;
		}
	}
}
void send_page_302(void *sock)
{
	char *msg;
	msg="HTTP/1.1 302 Redirect\r\n\
Connection: close\r\n\
Location: /\r\n\
Content-Length: 0\r\n\
\r\n";
	sock_write(sock,msg,strlen(msg));
}
void send_page_400(void *sock)
{
	char *msg;
	msg="HTTP/1.1 400 Bad Request\r\n\
Connection: close\r\n\
Content-Type: text/html\r\n\
Content-Length: 94\r\n\
\r\n\
<html><head><title>400 Bad Request</title></head><body><h1>400 Bad Request</h1></body></html>\n";
	sock_write(sock,msg,strlen(msg));
}
void send_page_404(void *sock)
{
	char *msg;
	msg="HTTP/1.1 404 Not Found\r\n\
Connection: close\r\n\
Content-Type: text/html\r\n\
Content-Length: 90\r\n\
\r\n\
<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>\n";
	sock_write(sock,msg,strlen(msg));
}
void send_page_500(void *sock)
{
	char *msg;
	msg="HTTP/1.1 500 Internal Server Error\r\n\
Connection: close\r\n\
Content-Type: text/html\r\n\
Content-Length: 114\r\n\
\r\n\
<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>\n";
	sock_write(sock,msg,strlen(msg));
}
int is_valid_char(char c)
{
	if(c>='A'&&c<='Z'||c>='a'&&c<='z'||c>='0'&&c<='9'||c=='.'||c=='/'||c=='-'||c=='_')
	{
		return 1;
	}
	return 0;
}
int is_valid_name(char *str)
{
	char c;
	while(c=*str)
	{
		if(!is_valid_char(c))
		{
			return 0;
		}
		++str;
	}
	return 1;
}
char *get_real_path(char *path,int pathlen,int *status)
{
	char *real_path,*p;
	int x,x1,i;
	char name[256];
	if(pathlen<=0)
	{
		*status=1;
		return NULL;
	}
	real_path=malloc(pathlen+strlen(server_root)+5);
	if(real_path==NULL)
	{
		*status=2;
		return NULL;
	}
	strcpy(real_path,server_root);
	strcat(real_path,"/");
	p=real_path+strlen(real_path);
	x=0;
	while(x<pathlen)
	{
		while(x<pathlen&&path[x]=='/')
		{
			++x;
		}
		x1=0;
		while(x<pathlen&&path[x]&&path[x]!='/')
		{
			name[x1]=path[x];
			++x;
			++x1;
			if(x1==256)
			{
				free(real_path);
				*status=1;
				return NULL;
			}
		}
		name[x1]=0;
		if(x1)
		{
			if(!strcmp(name,".."))
			{
				i=strlen(p);
				while(i&&p[i-1]!='/')
				{
					--i;
				}
				p[i]=0;
			}
			else if(strcmp(name,"."))
			{
				if(*p)
				{
					strcat(p,"/");
				}
				strcat(p,name);
			}
		}
	}
	return real_path;
}
struct file_entry
{
	struct file_entry *next;
	char name[768];
	long isdir;
};
void release_file_entries(struct file_entry *entries)
{
	struct file_entry *p;
	while(p=entries)
	{
		entries=p->next;
		free(p);
	}
}
void handle_get(void *sock,char *header)
{
	int i,status;
	char c;
	char *real_path;
	char *rpath;
	char buf[4096];
	void *fp;
	long size;
	void *fh;
	struct win32_find_data_a fdata;
	struct file_entry *head,*node,*p,*pp;
	i=4;
	while(c=header[i])
	{
		if(!is_valid_char(c))
		{
			if(i==4||strncmp(header+i," HTTP/1.",8))
			{
				send_page_404(sock);
				return;
			}
			break;
		}
		++i;
	}
	real_path=get_real_path(header+4,i-4,&status);
	if(real_path==NULL)
	{
		if(status==1)
		{
			send_page_404(sock);
		}
		else
		{
			send_page_500(sock);
		}
		return;
	}
	rpath=real_path+strlen(server_root);
	strcat(rpath,"/*");
	if((fh=FindFirstFileA(real_path,&fdata))!=INVALID_HANDLE_VALUE)
	{
//<html><head><title>[LocalShare] ...</title></head>\n
//<body><h1>[LocalShare] ...</h1>\n
//<form action="/$PUTF" enctype="multipart/form-data" method="post">\n
//Password <input type="password" name="PW"/><br/><input type="file" name="F"/><br/>Upload To <input type="text" name="FP"/><br/><input type="submit" name="SM" value="Upload"/><br/></form>\n
//<p><a href=".../..">PARENT</a></p>\n
//...
//</body></html>\n

//<p>[FILE] <a href=".../file">file</a></p>\n
//<p>[DIR ] <a href=".../dir">dir</a></p>\n
		head=NULL;
		rpath[strlen(rpath)-2]=0;
		size=48+29+67+187+32+15+3*strlen(rpath);
		if(rpath[1]==0)
		{
			--size;
		}
		do
		{
			if(strcmp(fdata.name,".")&&strcmp(fdata.name,"..")&&is_valid_name(fdata.name))
			{
				node=malloc(sizeof(*node));
				if(node==NULL)
				{
					release_file_entries(head);
					FindClose(fh);
					free(real_path);
					send_page_500(sock);
					return;
				}
				strcpy(node->name,fdata.name);
				node->isdir=fdata.attr&FILE_ATTRIBUTE_DIRECTORY;
				p=head;
				pp=NULL;
				while(p)
				{
					if(strcmp(p->name,fdata.name)>0)
					{
						break;
					}
					pp=p;
					p=p->next;
				}
				if(pp)
				{
					pp->next=node;
				}
				else
				{
					head=node;
				}
				node->next=p;
				size+=31+2*strlen(fdata.name)+strlen(rpath);
				if(rpath[1]==0)
				{
					--size;
				}
			}
		}
		while(FindNextFileA(fh,&fdata));
		FindClose(fh);
		strcpy(buf,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: ");
		sprinti(buf,size,1);
		strcat(buf,"\r\n\r\n");
		sock_write(sock,buf,strlen(buf));
		strcpy(buf,"<html><head><title>[LocalShare] ");
		sock_write(sock,buf,strlen(buf));
		sock_write(sock,rpath,strlen(rpath));
		strcpy(buf,"</title></head>\n<body><h1>[LocalShare] ");
		sock_write(sock,buf,strlen(buf));
		sock_write(sock,rpath,strlen(rpath));
		strcpy(buf,"</h1>\n<form action=\"/$PUTF\" enctype=\"multipart/form-data\" method=\"post\">\n\
Password <input type=\"password\" name=\"PW\"/><br/><input type=\"file\" name=\"F\"/><br/>Upload To <input type=\"text\" name=\"FP\"/><br/>\
<input type=\"submit\" name=\"SM\" value=\"Upload\"/><br/></form>\n\
<p><a href=\"");
		sock_write(sock,buf,strlen(buf));
		sock_write(sock,rpath,strlen(rpath));
		if(rpath[1])
		{
			sock_write(sock,"/",1);
		}
		strcpy(buf,"..\">PARENT</a></p>\n");
		sock_write(sock,buf,strlen(buf));
		p=head;
		while(p)
		{
			if(p->isdir)
			{
				strcpy(buf,"<p>[DIR ] <a href=\"");
			}
			else
			{
				strcpy(buf,"<p>[FILE] <a href=\"");
			}
			sock_write(sock,buf,strlen(buf));
			sock_write(sock,rpath,strlen(rpath));
			if(rpath[1])
			{
				sock_write(sock,"/",1);
			}
			sock_write(sock,p->name,strlen(p->name));
			sock_write(sock,"\">",2);
			sock_write(sock,p->name,strlen(p->name));
			strcpy(buf,"</a></p>\n");
			sock_write(sock,buf,strlen(buf));
			p=p->next;
		}
		strcpy(buf,"</body></html>\n");
		sock_write(sock,buf,strlen(buf));
		release_file_entries(head);
	}
	else 
	{
		rpath[strlen(rpath)-2]=0;
		if(fp=fopen(real_path,"rb"))
		{
			fseek(fp,0,2);
			size=ftell(fp);
			fseek(fp,0,0);
			strcpy(buf,"HTTP/1.1 200 OK\r\nContent-Type: image\r\nConnection: close\r\nContent-Length: ");
			sprinti(buf,size,1);
			strcat(buf,"\r\n\r\n");
			sock_write(sock,buf,strlen(buf));
			while(size>0)
			{
				i=fread(buf,1,4096,fp);
				if(i==0)
				{
					memset(buf,0,4096);
					i=4096;
				}
				if(i>size)
				{
					i=size;
				}
				if(sock_write(sock,buf,i)==0)
				{
					break;
				}
				size-=i;
			}
			fclose(fp);
		}
		else
		{
			send_page_404(sock);
		}
	}
	free(real_path);
}
int mem_match(void *data,int datalen,void *target,int targetlen)
{
	int i;
	i=0;
	while(i<=datalen-targetlen)
	{
		if(((char *)data)[i]==*(char *)target)
		{
			if(!memcmp((char *)data+i,target,targetlen))
			{
				return i;
			}
		}
		++i;
	}
	return -1;
}
char *get_tmp_file(void)
{
	char *name;
	char buf[32];
	int i;
	unsigned int val;
	buf[0]='#';
	buf[31]=0;
	name=malloc(strlen(server_root)+48);
	if(name==NULL)
	{
		return NULL;
	}
	strcpy(name,server_root);
	strcat(name,"/");
	i=1;
	while(i<31)
	{
		while(rand_s(&val));
		val%=62;
		if(val<10)
		{
			val+='0';
		}
		else if(val<36)
		{
			val+='A'-10;
		}
		else
		{
			val+='a'-36;
		}
		buf[i]=val;
		++i;
	}
	strcat(name,buf);
	return name;
}
void handle_post(void *sock,char *header)
{
	char val;
	char *bound;
	char *real_path;
	char *tmp_file;
	char buf[8192];
	int i,bound_len,status,ret,size,read_size,count,len;
	int stage;
	void *fp,*fp2;
	if(strncmp(header+5,"/$PUTF HTTP/1.",14))
	{
		send_page_400(sock);
		return;
	}
	bound=header+18;
	while(*bound)
	{
		if(*bound=='\r'&&!strncmp(bound,"\r\nContent-Type: multipart/form-data; boundary=",46))
		{
			bound+=46;
			break;
		}
		++bound;
	}
	if(!*bound)
	{
		send_page_400(sock);
		return;
	}
	bound_len=0;
	while(bound[bound_len]&&bound[bound_len]!='\r')
	{
		++bound_len;
	}
	if(bound_len<8||bound_len>2048)
	{
		send_page_400(sock);
		return;
	}
	tmp_file=get_tmp_file();
	if(tmp_file==NULL)
	{
		send_page_500(sock);
		return;
	}
	fp=fopen(tmp_file,"w+b");
	if(fp==NULL)
	{
		send_page_500(sock);
		return;
	}
	send_page_302(sock);
	real_path=NULL;
	ret=0;
	stage=0;
	size=0;
	while(1)
	{
		do
		{
			read_size=sock_read(sock,buf+size,8192-size);
			if(read_size==0)
			{
				break;
			}
			size+=read_size;
		}
		while(size<8192);
		i=-1;
		if(size>2)
		{
			i=mem_match(buf,size-2,bound,bound_len);
		}
		if(i==-1)
		{
			if(stage!=2)
			{
				ret=1;
				break;
			}
			if(size>bound_len+10)
			{
				fwrite(buf,size-bound_len-10,1,fp);
				memmove(buf,buf+size-bound_len-10,bound_len+10);
				size=bound_len+10;
			}
			else
			{
				ret=1;
				break;
			}
		}
		else
		{
			if(stage==2)
			{
				fwrite(buf,i-4,1,fp);
			}
			memmove(buf,buf+i+bound_len,size-i-bound_len);
			size-=i+bound_len;
			++stage;
			if(stage==4)
			{
				break;
			}
			do
			{
				read_size=sock_read(sock,buf+size,8192-size);
				if(read_size==0)
				{
					break;
				}
				size+=read_size;
			}
			while(size<8192);
			if(size<=50||memcmp(buf,"\r\nContent-Disposition: form-data; name=\"",40))
			{
				ret=1;
				break;
			}
			if(stage==1)
			{
				if(memcmp(buf+40,"PW\"",3))
				{
					ret=1;
					break;
				}
			}
			else if(stage==2)
			{
				if(memcmp(buf+40,"F\"",2))
				{
					ret=1;
					break;
				}
			}
			else
			{
				if(memcmp(buf+40,"FP\"",3))
				{
					ret=1;
					break;
				}
			}
			count=0;
			i=0;
			while(i<size)
			{
				if(buf[i]=='\r'||buf[i]=='\n')
				{
					++count;
					if(count==4)
					{
						++i;
						break;
					}
				}
				else
				{
					count=0;
				}
				++i;
			}
			if(i==size)
			{
				ret=1;
				break;
			}
			if(stage==1)
			{
				if(upload_password[0]==0||size-i<=strlen(upload_password)||
buf[i+strlen(upload_password)]!='\r'||memcmp(upload_password,buf+i,strlen(upload_password)))
				{
					ret=1;
					break;
				}
			}
			else if(stage==2)
			{
				memmove(buf,buf+i,size-i);
				size-=i;
			}
			else if(stage==3)
			{
				len=0;
				while(i+len<size&&buf[i+len]!='\r')
				{
					++len;
				}
				real_path=get_real_path(buf+i,len,&status);
				if(!real_path)
				{
					ret=1;
					break;
				}
			}
		}
	}
	if(!ret)
	{
		fp2=fopen(real_path,"rb");
		if(fp2)
		{
			fclose(fp2);
		}
		else
		{
			fp2=fopen(real_path,"wb");
			if(fp2)
			{
				fseek(fp,0,0);
				while(size=fread(buf,1,8192,fp))
				{
					fwrite(buf,1,size,fp2);
				}
				fclose(fp2);
			}
		}
	}
	fclose(fp);
	remove(tmp_file);
	free(real_path);
}
int T_service(void *sock)
{
	int status;
	char *header;
	header=read_header(sock,&status);
	if(header==NULL)
	{
		if(status==1)
		{
			send_page_400(sock);
		}
		else if(status==2)
		{
			send_page_500(sock);
		}
		closesocket(sock);
		return 0;
	}
	if(!strncmp(header,"GET ",4))
	{
		handle_get(sock,header);
	}
	else if(!strncmp(header,"POST ",5))
	{
		handle_post(sock,header);
	}
	else
	{
		send_page_400(sock);
	}
	free(header);
	closesocket(sock);
	return 0;
}