typedef struct x { int a; int b; } __attribute__((aligned(32))) X;
typedef struct y { X x; X y[31]; int c; } Y;

Y y[2];

int main(void)
{
#ifndef __pdp10__
  if (((char *)&y[1] - (char *)&y[0]) & 31)
    abort ();
#endif
  exit (0);
}                
