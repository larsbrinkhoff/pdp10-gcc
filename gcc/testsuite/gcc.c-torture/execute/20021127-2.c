struct s
{
  char a, b;
};

struct s array[] = { { 1, 0 }, { 0, 1 } };

static int
foo (int i, int *x)
{
  int y, z;

  y = *x;
  if (y == 0)
    z = array[i].a;
  else
    z = array[i].b;

  if (z == 0)
    return y;

  return 0;
}

int
main ()
{
  int x;

#define TEST(I, X, RESULT) x = X; if (foo (I, &x) != RESULT) abort ()

  TEST (0, 0, 0);
  TEST (0, 1, 1);
  TEST (1, 0, 0);
  TEST (1, 1, 0);

  exit (0);
}
