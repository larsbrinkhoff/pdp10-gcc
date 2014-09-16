static int x = -1;
static char y[4];

static void foo (int *z)
{
  memcpy (y, z, 4);
  if (y[0] == 0)
    abort (0);
}

int main ()
{
  foo (&x);
  exit (0);
}
