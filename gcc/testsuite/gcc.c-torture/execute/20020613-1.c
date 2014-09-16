struct halves
{
  short lh, rh;
};

static int foo (int x)
{
#if 0
  struct halves *y = (struct halves *)x;
  return (int)&y->rh;
#endif
  return (int)&(((struct halves *)x)->rh);
}

struct halves h = { 1, 2 };

int main ()
{
  short *x = (short *) foo ((int)&h);
  if (*x != 2)
    abort ();
  exit (0);
}
