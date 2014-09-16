struct x
{
  int e, f, g;
};

static void f (int a, int b, int c, struct x d)
{
  if (a != 1 || b != 2 || c != 3 || d.e != 4 || d.f != 5 || d.g != 6)
    abort ();
}

int main ()
{
  struct x a = { 4, 5, 6 };
  f (1, 2, 3, a);
  exit (0);
}
