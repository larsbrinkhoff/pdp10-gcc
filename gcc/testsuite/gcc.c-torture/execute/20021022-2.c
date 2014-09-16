struct s
{
  int a[8];
  int *b;
};

void *
foo (struct s *x, int y)
{
  void *z = &x->b;
  return z + y;
}

void *
bar (struct s *x, int y)
{
  void *z = (void *)&x->b + y;
  return z;
}

struct s S;

int main (void)
{
  if (foo (&S, 3) != bar (&S, 3))
    abort ();
  exit (0);
}
