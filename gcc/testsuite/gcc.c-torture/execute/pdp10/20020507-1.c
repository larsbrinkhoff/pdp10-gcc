static int x;

static void foo (int *y)
{
  if (y != &x)
    abort ();
}

int main ()
{
  foo (&x);
  exit (0);
}
