struct foo
{
  char	*a;
  int	b, c, d, e;
};

struct foo *
f ()
{
  struct foo *x, *y;
  memcpy (x, y, sizeof (struct foo));
}
