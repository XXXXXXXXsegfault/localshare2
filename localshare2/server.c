char *read_header(SOCKET sock,int *status)
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
void send_page_302(SOCKET sock)
{
	char *msg;
	msg="HTTP/1.1 302 Redirect\r\n\
Connection: close\r\n\
Location: /\r\n\
Content-Length: 0\r\n\
\r\n";
	sock_write(sock,msg,strlen(msg));
}
void send_page_400(SOCKET sock)
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
void send_page_404(SOCKET sock)
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
void send_page_500(SOCKET sock)
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
int check_file_name(char *name,int len,int allow_slash)
{
	int c;
	while(len&&(c=*name))
	{
		if(c=='#'||c=='\\'||c=='*'||c=='?'||c=='\"'||c==':'||c=='<'||c=='>'||c=='|')
		{
			return 0;
		}
		if(!allow_slash&&c=='/')
		{
			return 0;
		}
		++name;
		--len;
	}
	return 1;
}
int strlen_encoded(char *str)
{
	char c;
	int ret;
	ret=0;
	while(c=*str)
	{
		if(!is_valid_char(c))
		{
			ret+=2;
		}
		ret+=1;
		++str;
	}
	return ret;
}
void url_encode(char *dst,char *src)
{
	char c;
	char *hexc;
	hexc="0123456789ABCDEF";
	while(c=*src)
	{
		if(is_valid_char(c))
		{
			*dst=c;
			++dst;
		}
		else
		{
			*dst='%';
			dst[1]=hexc[c>>4&0xf];
			dst[2]=hexc[c&0xf];
			dst+=3;
		}
		++src;
	}
	*dst=0;
}
int hexdig_to_val(char hexdig)
{
	if(hexdig>='0'&&hexdig<='9')
	{
		return hexdig-'0';
	}
	else if(hexdig>='A'&&hexdig<='F')
	{
		return hexdig-'A'+10;
	}
	else if(hexdig>='a'&&hexdig<='f')
	{
		return hexdig-'a'+10;
	}
	else
	{
		return -1;
	}
}
int decode_char(char *str,int *size)
{
	int ret,val;
	*size=0;
	if(is_valid_char(*str))
	{
		ret=*str&0xff;
		*size=1;
		return ret;
	}
	else if(*str=='%')
	{
		val=hexdig_to_val(str[1]);
		if(val<0)
		{
			return -1;
		}
		ret=val<<4;
		val=hexdig_to_val(str[2]);
		if(val<0)
		{
			return -1;
		}
		ret|=val;
		*size=3;
		return ret;
	}
	else if(*str=='+')
	{
		*size=1;
		return 32;
	}
	return -1;
}
char *get_real_path(char *path,int pathlen,int *status,int decode)
{
	char *real_path,*p;
	int x,x1,i,csize,cval;
	char name[256];
	if(pathlen<=0)
	{
		*status=1;
		return NULL;
	}
	real_path=malloc(pathlen+strlen(server_root)+1040);
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
		while(x<pathlen)
		{
			if(decode)
			{
				cval=decode_char(path+x,&csize);
			}
			else
			{
				cval=path[x];
				csize=1;
			}
			if(cval=='/')
			{
				x+=csize;
			}
			else
			{
				break;
			}
		}
		x1=0;
		while(x<pathlen&&path[x])
		{
			if(decode)
			{
				cval=decode_char(path+x,&csize);
			}
			else
			{
				cval=path[x];
				csize=1;
			}
			if(cval<0||x1==0&&cval=='#'||cval=='\\')
			{
				free(real_path);
				*status=1;
				return NULL;
			}
			name[x1]=cval;
			x+=csize;
			++x1;
			if(x1==256)
			{
				free(real_path);
				*status=1;
				return NULL;
			}
			if(cval=='/')
			{
				--x1;
				name[x1]=0;
				break;
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
	char encoded_name[768*3];
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
void handle_get(SOCKET sock,char *header)
{
	int i,status;
	char c;
	char *real_path;
	char *rpath,*encoded_rpath;
	char buf[4096];
	FILE *fp;
	long long size;
	HANDLE fh;
	WIN32_FIND_DATAA fdata;
	struct file_entry *head,*node,*p,*pp;
	i=4;
	while(c=header[i])
	{
		if(c==32)
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
	real_path=get_real_path(header+4,i-4,&status,1);
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
	encoded_rpath=malloc(strlen(rpath)*3+10);
	if(encoded_rpath==NULL)
	{
		free(real_path);
		send_page_500(sock);
		return;
	}
	url_encode(encoded_rpath,rpath);
	if(!check_file_name(rpath,strlen(rpath),1))
	{
		free(real_path);
		free(encoded_rpath);
		send_page_400(sock);
		return;
	}
	strcat(rpath,"/*");
	if((fh=FindFirstFileA(real_path,&fdata))!=INVALID_HANDLE_VALUE)
	{
//<html><head><title>[LocalShare] ...</title></head>\n
//<body><h1>[LocalShare] ...</h1>\n
//<form action="/$PUTF" enctype="multipart/form-data" method="post">\n
//Upload To (Directory) <input type="text" name="FP"/><br/><input type="file" name="F" multiple/><br/><input type="submit" name="SM" value="Upload"/><br/></form>\n
//<p><a href=".../..">PARENT</a></p>\n
//...
//</body></html>\n

//<p>[FILE] <a href=".../file">file</a></p>\n
//<p>[DIR ] <a href=".../dir">dir</a></p>\n
		head=NULL;
		rpath[strlen(rpath)-2]=0;
		size=48+29+67+160+32+15+3*strlen(rpath);
		if(rpath[1]==0)
		{
			--size;
		}
		do
		{
			if(strcmp(fdata.cFileName,".")&&strcmp(fdata.cFileName,"..")&&check_file_name(fdata.cFileName,strlen(fdata.cFileName),0))
			{
				node=malloc(sizeof(*node));
				if(node==NULL)
				{
					release_file_entries(head);
					FindClose(fh);
					free(real_path);
					free(encoded_rpath);
					send_page_500(sock);
					return;
				}
				strcpy(node->name,fdata.cFileName);
				url_encode(node->encoded_name,fdata.cFileName);
				node->isdir=fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY;
				p=head;
				pp=NULL;
				while(p)
				{
					if(strcmp(p->name,node->name)>0)
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
				size+=31+strlen(fdata.cFileName)+strlen_encoded(fdata.cFileName)+strlen_encoded(rpath);
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
Upload To (Directory) <input type=\"text\" name=\"FP\"/><br/>\
<input type=\"file\" name=\"F\" multiple/><br/>\
<input type=\"submit\" name=\"SM\" value=\"Upload\"/><br/></form>\n\
<p><a href=\"");
		sock_write(sock,buf,strlen(buf));
		sock_write(sock,encoded_rpath,strlen(encoded_rpath));
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
			sock_write(sock,p->encoded_name,strlen(p->encoded_name));
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
			_fseeki64(fp,0,SEEK_END);
			size=_ftelli64(fp);
			_fseeki64(fp,0,SEEK_SET);
			strcpy(buf,"HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nConnection: close\r\nContent-Length: ");
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
	free(encoded_rpath);
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
int boundary_end(void *data,int datalen,void *target,int targetlen)
{
	int i;
	i=0;
	while(i<=datalen-targetlen-2)
	{
		if(((char *)data)[i]==*(char *)target)
		{
			if(!memcmp((char *)data+i,target,targetlen))
			{
				if(!memcmp((char *)data+i+targetlen,"--",2))
				{
					return 1;
				}
			}
		}
		++i;
	}
	return 0;
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

void handle_post(SOCKET sock,char *header)
{
	char val;
	char *bound;
	char *real_path;
	char *tmp_file;
	char buf[8192];
	int i,bound_len,status,stage,size,read_size,ret,path_len,connection_close;
	FILE *fp;
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
		send_page_400(sock);
		return;
	}
	stage=0;
	size=0;
	read_size=0;
	connection_close=0;
	while(1)
	{
		memmove(buf,buf+read_size,size-read_size);
		size-=read_size;
		read_size=0;
		if(!connection_close)
		{
			do
			{
				ret=sock_read(sock,buf+size,8192-size);
				if(ret<0)
				{
					ret=0;
				}
				size+=ret;
				if(mem_match(buf,size,bound,bound_len)>=0)
				{
					break;
				}
			}
			while(size<8192&&ret);
			if(boundary_end(buf,size,bound,bound_len))
			{
				connection_close=1;
			}
		}
		ret=mem_match(buf,size,bound,bound_len);
		if(ret<0)
		{
			if(stage!=2||size<4+bound_len)
			{
				if(stage==1)
				{
					free(real_path);
				}
				if(stage==2)
				{
					fclose(fp);
					free(real_path);
					remove(tmp_file);
				}
				free(tmp_file);
				send_page_400(sock);
				return;
			}
			i=fwrite(buf+read_size,1,size-4-bound_len,fp);
			if(i!=size-4-bound_len)
			{
				fclose(fp);
				remove(tmp_file);
				free(real_path);
				free(tmp_file);
				send_page_500(sock);
				return;
			}
			read_size=size-4-bound_len;
		}
		else
		{
			if(stage==0)
			{
				if(ret!=2)
				{
					free(tmp_file);
					send_page_400(sock);
					return;
				}
				ret+=bound_len+2;
				if(memcmp(buf+ret,"Content-Disposition: form-data; ",32))
				{
					free(tmp_file);
					send_page_400(sock);
					return;
				}
				ret+=32;
				while(1)
				{
					if(ret+9>=size||buf[ret]=='\n')
					{
						free(tmp_file);
						send_page_400(sock);
						return;
					}
					if(buf[ret]=='n')
					{
						if(!memcmp(buf+ret,"name=\"",6))
						{
							if(!memcmp(buf+ret+6,"FP\"",3))
							{
								break;
							}
							else
							{
								free(tmp_file);
								send_page_400(sock);
								return;
							}
						}
					}
					++ret;
				}
				while(1)
				{
					if(ret+3>=size)
					{
						free(tmp_file);
						send_page_400(sock);
						return;
					}
					if(buf[ret]=='\n')
					{
						if(!memcmp(buf+ret,"\n\r\n",3))
						{
							break;
						}
					}
					++ret;
				}
				ret+=3;
				i=ret;
				while(ret<size&&buf[ret]!='\r')
				{
					++ret;
					if(i+1024<ret)
					{
						free(tmp_file);
						send_page_400(sock);
						return;
					}
				}
				if(ret+2>=size)
				{
					free(tmp_file);
					send_page_400(sock);
					return;
				}
				real_path=get_real_path(buf+i,ret-i,&status,0);
				if(real_path==NULL)
				{
					free(tmp_file);
					if(status==2)
					{
						send_page_500(sock);
					}
					else
					{
						send_page_400(sock);
					}
					return;
				}
				CreateDirectoryA(real_path,NULL);
				path_len=strlen(real_path);
				ret+=2;
				read_size=ret;
				stage=1;
			}
			else if(stage==1)
			{
				if(ret!=2)
				{
					free(real_path);
					free(tmp_file);
					send_page_400(sock);
					return;
				}
				ret+=bound_len+2;
				if(memcmp(buf+ret,"Content-Disposition: form-data; ",32))
				{
					free(real_path);
					free(tmp_file);
					send_page_400(sock);
					return;
				}
				ret+=32;
				i=ret;
				while(1)
				{
					if(ret+8>=size||buf[ret]=='\n')
					{
						free(real_path);
						free(tmp_file);
						send_page_400(sock);
						return;
					}
					if(buf[ret]=='n')
					{
						if(!memcmp(buf+ret,"name=\"",6))
						{
							if(!memcmp(buf+ret+6,"F\"",2))
							{
								break;
							}
							else
							{
								free(real_path);
								free(tmp_file);
								send_page_302(sock);
								return;
							}
						}
					}
					++ret;
				}
				ret=i;
				while(1)
				{
					if(ret+10>=size||buf[ret]=='\n')
					{
						free(real_path);
						free(tmp_file);
						send_page_400(sock);
						return;
					}
					if(buf[ret]=='f')
					{
						if(!memcmp(buf+ret,"filename=\"",10))
						{
							ret+=10;
							i=ret;
							while(ret<size&&buf[ret]!='\"')
							{
								if(buf[ret]=='/'||buf[ret]=='\\'||buf[ret]=='<'||buf[ret]=='>'||buf[ret]=='|'||buf[ret]=='?'||buf[ret]=='*'||buf[ret]=='\"'||buf[ret]==':'||buf[ret]=='#')
								{
									free(real_path);
									free(tmp_file);
									send_page_400(sock);
									return;
								}
								++ret;
							}
							if(ret>=size||ret-i>=256)
							{
								free(real_path);
								free(tmp_file);
								send_page_400(sock);
								return;	
							}
							buf[ret]=0;
							++ret;
							real_path[path_len]='/';
							real_path[path_len+1]=0;
							strcat(real_path,buf+i);
							break;
						}
					}
					++ret;
				}
				while(1)
				{
					if(ret+3>=size)
					{
						free(tmp_file);
						send_page_400(sock);
						return;
					}
					if(buf[ret]=='\n')
					{
						if(!memcmp(buf+ret,"\n\r\n",3))
						{
							break;
						}
					}
					++ret;
				}
				ret+=3;
				read_size=ret;
				fp=fopen(tmp_file,"wb");
				if(fp==NULL)
				{
					free(real_path);
					free(tmp_file);
					send_page_500(sock);
					return;
				}
				stage=2;
			}
			else if(stage==2)
			{
				i=fwrite(buf,1,ret-4,fp);
				fclose(fp);
				if(i!=ret-4)
				{
					remove(tmp_file);
					free(real_path);
					free(tmp_file);
					send_page_500(sock);
					return;
				}
				remove(real_path);
				rename(tmp_file,real_path);
				stage=1;
				read_size=ret-2;
			}
		}
	}
}
DWORD WINAPI T_service(LPVOID arg)
{
	int status;
	char *header;
	SOCKET sock;
	sock=(SOCKET)arg;
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
