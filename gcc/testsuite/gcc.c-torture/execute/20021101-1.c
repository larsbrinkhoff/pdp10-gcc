struct foo { int *a[2]; };
struct bar { int *b; };

struct foo x =
{
  { (int *)((char *)&x.a[2] - 8), 0 }
};

struct bar y =
{
  (int *)((char *)&y.b - 8)
};

int * foo (void)
{
  int **a = &x.a[2];
  char *b = (char *)a;
  char *c = b - 8;
  int *d = (int *)c;
  return d;
}

int * bar (void)
{
  int **a = &y.b;
  char *b = (char *)a;
  char *c = b - 8;
  int *d = (int *)c;
  return d;
}

int main ()
{
  if (foo () != x.a[0] || bar () != y.b)
    abort ();
  exit (0);
}
