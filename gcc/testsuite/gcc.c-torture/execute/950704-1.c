#ifdef __pdp10__
#ifdef __LONG_LONG_71BIT__
#define MSB		0200000000000000000000000LL
#define	MSB_MINUS_1	0177777777777777777777777LL
#else
#define MSB		0400000000000000000000000LL
#define	MSB_MINUS_1	0377777777777777777777777LL
#endif
#else
#define MSB		0x8000000000000000LL
#define	MSB_MINUS_1	0x7fffffffffffffffLL
#endif

int errflag;

long long
f (long long x, long long y)
{
  long long r;

  errflag = 0;
  r = x + y;
  if (x >= 0)
    {
      if ((y < 0) || (r >= 0))
	return r;
    }
  else
    {
      if ((y > 0) || (r < 0))
	return r;
    }
  errflag = 1;
  return 0;
}

main ()
{
  f (0, 0);
  if (errflag)
    abort ();

  f (1, -1);
  if (errflag)
    abort ();

  f (-1, 1);
  if (errflag)
    abort ();

  f (MSB, MSB);
  if (!errflag)
    abort ();

  f (MSB, -1LL);
  if (!errflag)
    abort ();

  f (MSB_MINUS_1, MSB_MINUS_1);
  if (!errflag)
    abort ();

  f (MSB_MINUS_1, 1LL);
  if (!errflag)
    abort ();

  f (MSB_MINUS_1, MSB);
  if (errflag)
    abort ();

  exit (0);
}
