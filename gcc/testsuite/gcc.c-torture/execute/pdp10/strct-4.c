struct x
{
  int d, e, f, g;
};

static void f (int a, int b, struct x c)
{
  if (a != 1 || b != 2 || c.d != 3 || c.e != 4 || c.f != 5 || c.g != 6)
    abort ();
}

int main ()
{
  struct x a = { 3, 4, 5, 6 };
  f (1, 2, a);
  exit (0);
}
