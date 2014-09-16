typedef unsigned ll __attribute__ ((mode (DI)));

static void f (int a, int b, int c, ll d)
{
  if (d != 04000000000005ULL)
    abort ();
}

int main ()
{
  f (1, 2, 3, 04000000000005ULL);
  exit (0);
}
