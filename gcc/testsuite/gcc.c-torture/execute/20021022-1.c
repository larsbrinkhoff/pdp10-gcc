static void bar (int x)
{
  if (x != 0)
    abort ();
}

static void foo (int x)
{
  bar (x >= 1 && x <= 100 ? 0 : 1);
}

int main (void)
{
  foo (42);
  exit (0);
}
