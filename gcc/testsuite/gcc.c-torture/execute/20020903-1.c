static short x[2];

static void
f (void)
{
  x[0] = 0;
}

static void
g (void)
{
  x[1] = 0;
}

int
main (void)
{
  x[0] = x[1] = -1;
  f ();
  if (x[0] != 0)
    abort ();

  x[0] = x[1] = -1;
  g ();
  if (x[1] != 0)
    abort ();

  exit (0);
}
