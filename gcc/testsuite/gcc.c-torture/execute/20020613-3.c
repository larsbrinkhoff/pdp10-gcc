static int intify (char *x)
{
  return (int)x;
}

char x[2];

int main ()
{
  int a, b, c;

  a = (int)(&x[1]) - (int)(&x[0]);
  b = intify (&x[1]);
  c = intify (&x[0]);

  if (a != b - c)
    abort ();

  exit (0);
}
