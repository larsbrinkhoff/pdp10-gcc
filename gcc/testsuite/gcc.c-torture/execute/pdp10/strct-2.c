struct x
{
  int d, e, f;
};

static void f (int a, int b, struct x c)
{
  if (a != 1 || b != 2 || c.d != 3 || c.e != 4 || c.f != 5)
    abort ();
}

int main ()
{
  f (1, 2, (struct x) { 3, 4, 5 });
  exit (0);
}
