#define NULL ((void *)0)
#define INVALID_HANDLE_VALUE ((void *)0xffffffffffffffff)
#define htons(x) (((unsigned short)(x)<<8)|((unsigned short)(x)>>8))
#define htonl(x) (((unsigned int)(x)<<24)|((unsigned int)(x)>>24)|((unsigned int)(x)<<8&0xff0000)|(unsigned int)(x)>>8&0xff00)
#define INVALID_SOCKET ((void *)-1)
#define AF_INET 2
#define SOCK_STREAM 1
#define IPPROTO_TCP 6
#define MSG_PEEK 2
#define MSG_WAITALL 8
#define FILE_ATTRIBUTE_DIRECTORY 16
#define SEEK_SET 0
#define SEEK_END 2
struct win32_find_data_a
{
	unsigned int attr;
	unsigned int ctime[2];
	unsigned int atime[2];
	unsigned int wtime[2];
	unsigned int size_hi;
	unsigned int size_lo;
	unsigned int rsv[2];
	char name[260];
	char rsv2[36];
};
struct sockaddr_in
{
	unsigned short family;
	unsigned short port;
	unsigned int addr;
	unsigned char pad[8];
};
struct fd_set
{
	int nfds;
	int pad;
	void *sock[1];
};