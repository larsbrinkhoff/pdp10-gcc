#include <stdarg.h>

#define N 4

typedef struct { char x[N]; } T;

void foo (int size, ...)
{
  T x;
  va_list ap;
  int i;

  va_start (ap, size);

  x = va_arg (ap, T);

  //printf ("x.x = { %d, %d, %d, %d }\n", x.x[0], x.x[1], x.x[2], x.x[3]);

  if (x.x[0] != 1 || x.x[1] != 2 || x.x[2] != 3 || x.x[3] != 4)
    abort ();

  va_end (ap);
}

int main (void)
{
  T x;
  int i;

  x.x[0] = 1;
  x.x[1] = 2;
  x.x[2] = 3;
  x.x[3] = 4;

  //printf ("x.x = { %d, %d, %d, %d }\n", x.x[0], x.x[1], x.x[2], x.x[3]);

  foo (0, x);
  exit (0);
}
