struct foo {
  char *a, *b, *c;
  char d[16];
} x;

void g (char *y)
{
  if (*y != 42)
    abort ();
}

void f (struct foo *f)
{
  g (f->d + 7);
}

int main ()
{
  x.d[7] = 42;
  f (&x);
  exit (0);
}
