typedef struct
{
  struct { short l, r; } a[3];
} halves;

short *
foo (halves *p, int i)
{
  return &p->a[i].r;
}

short
bar (halves *p, int i)
{
  return p->a[i].r;
}

halves x = { { { 1, 2 }, { 3, 4 }, { 5, 6 } } };

int main ()
{
  if (foo (&x, 0) != &x.a[0].r)
    abort ();
  if (foo (&x, 1) != &x.a[1].r)
    abort ();
  if (foo (&x, 2) != &x.a[2].r)
    abort ();

  if (bar (&x, 0) != 2)
    abort ();
  if (bar (&x, 1) != 4)
    abort ();
  if (bar (&x, 2) != 6)
    abort ();

  exit (0);
}
