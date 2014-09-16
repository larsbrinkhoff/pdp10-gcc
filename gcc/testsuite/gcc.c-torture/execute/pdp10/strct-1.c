struct x
{
  int e, f;
};

static void f (int a, int b, int c, struct x d)
{
  if (a != 1 || b != 2 || c != 3 || d.e != 4 || d.f != 5)
    abort ();
}

int main ()
{
  f (1, 2, 3, (struct x) { 4, 5 });
  exit (0);
}
