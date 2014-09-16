/* Fails when compiled with -mregparm=0 -g -O1 in XKL_STUFF mode.  */

int foo (void)
{
  return 42;
}

struct { int a, b; } y, *x = &y;

void
bar (void)
{
  if (foo ())
    x->b = foo ();
}

int main ()
{
  y.b = 0;
  bar ();
  if (y.b != 42)
    abort ();
  exit (0);
}
