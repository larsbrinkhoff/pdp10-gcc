static int x[2];

static void foo (int y)
{
  if (x + y != x + 1)
    abort ();
}

int main ()
{
  foo (1);
  exit (0);
}
