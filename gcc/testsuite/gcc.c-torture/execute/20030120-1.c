int x;

void foo (int p)
{
  static int q = 0;

  if (q == 0)
    q = p;
  else if (q != p)
    abort ();
}

void f1 (void)
{
  foo ((int)(char *)&x);
}

void f2 (void)
{
  char *y = (char *)&x;
  foo ((int)y);
}

int g1 (void)
{
  return (int)(char *)&x;
}

int g2 (void)
{
  char *y = (char *)&x;
  return (int)y;
}

int main ()
{
  f1 ();
  f2 ();

  if (g1 () != g2 ())
    abort ();

  exit (0);
}
