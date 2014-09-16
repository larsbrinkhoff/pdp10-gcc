typedef struct trio { int a, b, c; } trio;

#if 1
int
bar (int i, int j, int k, trio t)
{
  if (t.a != 1 || t.b != 2 || t.c != 3 ||
      i != 4 || j != 5 || k != 6)
    abort ();
}
#endif

#if 1
int
foo (trio t, int i, int j, int k)
{
  return bar (i, j, k, t);
}
#endif

#if 1
main ()
{
  trio t = { 1, 2, 3 };

  foo (t, 4, 5, 6);
  exit (0);
}
#endif
