#ifdef __pdp10__
char a[5] = { 11, 12, 13, 14, 15 };

#define DEF(n)					\
  char * f##n (void)				\
  {						\
    char *p = &a[-1];				\
    return p + n;				\
  }

#define TEST(n)					\
  x = f##n () + (n == 0);			\
  if (*x != n + 10 + (n == 0))			\
     abort ();

/* These functions take the address of a[-1], which is
   implementation-defined.  Only test this on the PDP-10, where the
   definition is known.  */
DEF(0)
DEF(1)
DEF(2)
DEF(3)
DEF(4)
DEF(5)
#endif

int main ()
{
#ifdef __pdp10__
  char *x;

  TEST (0);
  TEST (1);
  TEST (2);
  TEST (3);
  TEST (4);
  TEST (5);
#endif

  exit (0);
}
