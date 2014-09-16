#define sign_extend8	sext8
#define sign_extend16	sext16
#define zero_extend8	zext8
#define zero_extend16	zext16

void
store16 (p, a)
     short *p;
     short a;
{
  *p = a;
}

signed int
sign_extend16 (p)
     signed short *p;
{
  return *p;
}

unsigned int
zero_extend16 (p)
     unsigned short *p;
{
  return *p;
}

void
store8 (p, a)
     char *p;
     char a;
{
  *p = a;
}

signed int
sign_extend8 (p)
     signed char *p;
{
  return *p;
}

unsigned int
zero_extend8 (p)
     unsigned char *p;
{
  return *p;
}

