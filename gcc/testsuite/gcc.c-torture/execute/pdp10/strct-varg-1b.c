#include <stdarg.h>

struct s { int x, y; };

f (int x_attr, ...)
{
  struct s va_values;
  va_list va;
  int attr;
  int i;

  va_start (va, x_attr);
  attr = x_attr;

  if (attr != 2)
    abort ();

  va_values = va_arg (va, struct s);
  printf ("x = %d, y = %d\n", va_values.x, va_values.y);
  if (va_values.x != 0xaaaa || va_values.y != 0x5555)
    abort ();

  attr = va_arg (va, int);
  if (attr != 3)
    abort ();

  va_values = va_arg (va, struct s);
  printf ("x = %d, y = %d\n", va_values.x, va_values.y);
  if (va_values.x != 0xffff || va_values.y != 0x1111)
    abort ();

  va_end (va);
}

main ()
{
  struct s a, b;

  a.x = 0xaaaa;
  a.y = 0x5555;
  b.x = 0xffff;
  b.y = 0x1111;

  f (2, a, 3, b);
  exit (0);
}
