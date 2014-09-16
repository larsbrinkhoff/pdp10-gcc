/* Test loading a byte with different levels of indirections (with
   optional offset).  All but the last one should expand to one
   instruction on the PDP-10.  */

struct s2 { int a; char *b; };

struct s3 { int a; char **b; };

char f1 (char *x)
{
  return *x;		/* (mem:QI x) */
}

char f2a (char **x)
{
  return **x;		/* (mem:QI (mem:P x)) */
}

char f2b (struct s2 *x)
{
  return *x->b;		/* (mem:QI (mem:P (plus:P x (const_int n)))) */
}

char f3a (char ***x)
{
  return ***x;		/* (mem:QI (mem:P (mem:P x))) */
}

char f3b (struct s3 *x)
{
  return **x->b;	/* (mem:QI (mem:P (mem:P (plus:P x (const_int n))))) */
}

char f3c (struct s2 **x)
{
  return *(*x)->b;	/* (mem:QI (mem:P (plus:P (mem:P x) (const_int n)))) */
}

char c[1] = { 42 };
char *p = c;
char **pp = &p;
struct s2 x2 = { 1, c };
struct s3 x3 = { 1, &p };
struct s2 *px2 = &x2;

int main ()
{
  if (f1 (c) != 42)
    abort ();
  if (f2a (&p) != 42)
    abort ();
  if (f2b (&x2) != 42)
    abort ();
  if (f3a (&pp) != 42)
    abort ();
  if (f3b (&x3) != 42)
    abort ();
  if (f3c (&px2) != 42)
    abort ();
  exit (0);
}
