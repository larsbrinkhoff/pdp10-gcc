char c[4];

static void
f (void)
{
  c[0] = 1;
  c[1] = 2;
  c[2] = 3;
  c[3] = 4;
}

main ()
{
  f ();
  if (c[0] != 1 || c[1] != 2 || c[2] != 3 || c[3] != 4)
    abort ();
  exit (0);
}
