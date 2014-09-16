/* Tests clearing word-aligned constant-sized char arrays.  */

char x[12];

#define CLEAR(n) static void clr##n (int *x) { memset (x, 0, n); }

CLEAR ( 1) CLEAR ( 2) CLEAR ( 3) CLEAR ( 4)
CLEAR ( 5) CLEAR ( 6) CLEAR ( 7) CLEAR ( 8)
CLEAR ( 9) CLEAR (10) CLEAR (11) CLEAR (12)

static void
test (void (*clear) (int *), int n)
{
  int i;

  for (i = 0; i < (int)sizeof x; i++)
    x[i] = i + 1;

  clear ((int *)x);

  for (i = 0; i < n; i++)
    if (x[i] != 0)
      abort ();
  for (; i < (int)sizeof x; i++)
    if (x[i] != i + 1)
      abort ();
}

int main ()
{
  test (clr1, 1);
  test (clr2, 2);
  test (clr3, 3);
  test (clr4, 4);
  test (clr5, 5);
  test (clr6, 6);
  test (clr7, 7);
  test (clr8, 8);
  test (clr9, 9);
  test (clr10, 10);
  test (clr11, 11);
  test (clr12, 12);

  exit (0);
}
