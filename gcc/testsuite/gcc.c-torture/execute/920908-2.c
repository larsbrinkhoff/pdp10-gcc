/*
CONF:m68k-sun-sunos4.1.1
OPTIONS:-O
*/
struct T
{
unsigned i:8;
unsigned c:24;
};
f(struct T t)
{
struct T s[1];
s[0]=t;
return(char)s->c;
}
main()
{
struct T t;
t.i=0xff;
t.c=0xffff11;
#ifdef __pdp10__
if(f(t)!=0x111)abort();
#else
if(f(t)!=0x11)abort();
#endif
exit(0);
}
