/* On the i960 any arg bigger than 16 bytes causes all subsequent args
   to be passed on the stack.  We test this.  */

#include <stdarg.h>

typedef struct {
  char a[32];
} big;

#if 1
void
f (big x, char *s, ...)
{
  va_list ap;

  if (x.a[0] != 'a' || x.a[1] != 'b' || x.a[2] != 'c')
    abort ();
  if (*s != 0)
    abort ();
  va_start (ap, s);
  if (va_arg (ap, int) != 42)
    abort ();
  if (va_arg (ap, int) != 'x')
    abort ();
  if (va_arg (ap, int) != 0)
    abort ();
  va_end (ap);
}
#else
void
f (big x, char *s, ...)
{
  va_list ap;
  int y;

  printf ("x.a = \"%s\"\n", x.a);
  if (x.a[0] != 'a' || x.a[1] != 'b' || x.a[2] != 'c')
    abort ();
  va_start (ap, s);
  y = va_arg (ap, int);
  printf ("y(1) = %d (%d)\n", y, 42);
  if (y != 42)
    abort ();
  y = va_arg (ap, int);
  printf ("y(2) = %d (%d)\n", y, 'x');
  if (y != 'x')
    abort ();
  y = va_arg (ap, int);
  printf ("y(3) = %d (%d)\n", y, 0);
  if (y != 0)
    abort ();
  va_end (ap);
}
#endif

main ()
{
  static big x = { "abc" };

  f (x, "", 42, 'x', 0);
  exit (0);
}
