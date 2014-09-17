/* PR c/4389
   This testcase failed because host_integerp (x, 0) was returning
   1 even for constants bigger than 2^31.  */
/* { dg-do run } */
/* { dg-options "-O2" } */

extern void abort (void);
extern void exit (int);
struct A {
  int a[10000][10000];
};
int b[2] = { 213151, 0 };

void foo (struct A *x, int y)
{
  if (x->a[9999][9999] != x->a[y][y])
    abort ();
  if (x->a[9999][9999] != 213151)
    abort ();
}

int main (void)
{
  struct A *x;
  asm ("" : "=r" (x) : "0" (&b[1]));
  foo (x - 1, 9999);
  exit (0);
}
