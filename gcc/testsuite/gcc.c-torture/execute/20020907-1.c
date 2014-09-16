struct foo
{
  void (*z) (void);
};

struct bar
{
  struct foo *y;
};

void
f (struct bar *x)
{
  (*x->y->z) ();
}

static void
g (void)
{
  exit (0);
}

struct foo a = { g };
struct bar b = { &a };

int
main (void)
{
  f (&b);
  abort ();
}
