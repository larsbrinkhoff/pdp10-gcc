typedef unsigned int	__uint32_t __attribute__ ((size (32)));
typedef __uint32_t	u_int32_t;
typedef unsigned char	u_char;

typedef struct
{
  u_int32_t x[5];
} foo;

void f (u_char a[20], foo *b)
{
  int i;
  a[i] = (u_char)((b->x[i>>2] >> ((3 - (i&3)) * 8)) & 255);
}
