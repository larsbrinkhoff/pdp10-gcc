struct foo
{
  int a, b, c;
};

f ()
{
  struct foo *x;
  __builtin_memset (x, 0, sizeof (struct foo));
}
