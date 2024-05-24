void sprinti(char *str,unsigned long long int a,int digits)
{
	unsigned long long int n;
	int d,l,sl;
	char buf[20];
	n=10000000000000000000ULL;
	d=20;
	while(n>a&&d>digits)
	{
		n/=10;
		--d;
	}
	l=0;
	while(n)
	{
		buf[l]=a/n+'0';
		a%=n;
		n/=10;
		++l;
	}
	sl=strlen(str);
	memcpy(str+sl,buf,l);
	str[sl+l]=0;
}
char *sinputi(char *str,unsigned long long int *result)
{
	unsigned long long int ret;
	char c;
	ret=0;
	while((c=*str)>='0'&&c<='9')
	{
		ret=ret*10+(c-'0');
		++str;
	}
	*result=ret;
	return str;
}
