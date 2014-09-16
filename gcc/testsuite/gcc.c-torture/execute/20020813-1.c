struct s
{
  int a;
  struct
  {
    int b;
    char c[1];
  } d;
};

static char *
f (struct s *x)
{
  return x->d.c;
}

int main ()
{
  struct s x;
  x.d.c[0] = 42;
  if (*f (&x) != 42)
    abort ();
  exit (0);
}
