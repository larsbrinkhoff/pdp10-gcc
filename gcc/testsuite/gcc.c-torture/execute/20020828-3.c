/* Tests copying word-aligned constant-sized char arrays.  */

char x[12];
char y[12];

#define COPY(n) static void copy##n (int *x, int *y) { memcpy (x, y, n); }

COPY ( 1) COPY ( 2) COPY ( 3) COPY ( 4)
COPY ( 5) COPY ( 6) COPY ( 7) COPY ( 8)
COPY ( 9) COPY (10) COPY (11) COPY (12)

static void
test (void (*copy) (int *, int *), int n)
{
  int i;

  for (i = 0; i < (int)sizeof y; i++)
    {
      x[i] = 0;
      y[i] = i + 1;
    }

  copy ((int *)x, (int *)y);

  for (i = 0; i < n; i++)
    if (x[i] != i + 1)
      abort ();
  for (; i < (int)sizeof x; i++)
    if (x[i] != 0)
      abort ();
}

int main ()
{
  test (copy1, 1);
  test (copy2, 2);
  test (copy3, 3);
  test (copy4, 4);
  test (copy5, 5);
  test (copy6, 6);
  test (copy7, 7);
  test (copy8, 8);
  test (copy9, 9);
  test (copy10, 10);
  test (copy11, 11);
  test (copy12, 12);

  exit (0);
}
