struct s {
  int a[0100];
  int b[0100];   
} x;

static int
foo (int y)
{
  return x.b[y];
}

int main ()
{
  x.b[42] = 42;
  if (foo (42) != 42)
    abort ();
  exit (0);
}
