int foo (char *x)
{
  return ((int)x) + 1;
}

int bar (char *x)
{
  int y = (int)x;
  return y + 1;
}

char a[1];

int main ()
{
  if (foo (a) != bar (a))
    abort ();
  exit (0);
}
