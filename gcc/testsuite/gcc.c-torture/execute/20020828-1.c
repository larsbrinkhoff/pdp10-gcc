/* The PDP-10 port fails when compiling 991201-1.c with -O1 or -O2.
   This test distills the problematic code to demonstrate the bug.

   When indexing into the an array inside a struct, the struct address
   must be converted to a byte pointer before being adjusted to point
   to the right element.  That conversion isn't done.  */

struct s
{
  char a[4];
};

struct s x;

static char ld_c (int i)
{
  return x.a[i];
}

static void st_c (int i, int j)
{
  x.a[i] = j;
}

static char ld_v (struct s *x, int i)
{
  return x->a[i];
}

static void st_v (struct s *x, int i, int j)
{
  x->a[i] = j;
}

int main()
{
  memset (&x, 0, sizeof x);
  st_c (3, 42);
  if (x.a[3] != 42)
    abort ();

  if (ld_c (3) != 42)
    abort ();

  memset (&x, 0, sizeof x);
  st_v (&x, 3, 42);
  if (x.a[3] != 42)
    abort ();

  if (ld_v (&x, 3) != 42)
    abort ();

  exit(0);
}
