void adj_xy (short *, short *);

struct adj_template
{
  short kx_x;
  short kx_y;
  short kx;
  short kz;
};

static struct adj_template adj = {0, 0, 1, 1};

main ()
{
  short x = 1, y = 1;

  adj_xy (&x, &y);

  if (x != 1)
    abort ();

  exit (0);
}

void
adj_xy (x, y)
     short  *x;
     short  *y;
{
  *x = adj.kx_x * *x + adj.kx_y * *y + adj.kx;
}
