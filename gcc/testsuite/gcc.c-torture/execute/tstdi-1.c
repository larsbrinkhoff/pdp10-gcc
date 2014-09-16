#define FALSE 140
#define TRUE 13

#ifdef __pdp10__
#ifdef __LONG_LONG_71BIT__
#define MSB		0200000000000000000000000LL
#define MSB_PLUS_1	0200000000000000000000001LL
#define	MSB_MINUS_1	0177777777777777777777777LL
#else
#define MSB		0400000000000000000000000LL
#define MSB_PLUS_1	0400000000000000000000001LL
#define	MSB_MINUS_1	0377777777777777777777777LL
#endif
#else
#define MSB		0x8000000000000000LL
#define MSB_PLUS_1	0x8000000000000001LL
#define	MSB_MINUS_1	0x7fffffffffffffffLL
#endif

feq (x)
     long long int x;
{
  if (x == 0)
    return TRUE;
  else
    return FALSE;
}

fne (x)
     long long int x;
{
  if (x != 0)
    return TRUE;
  else
    return FALSE;
}

flt (x)
     long long int x;
{
  if (x < 0)
    return TRUE;
  else
    return FALSE;
}

fge (x)
     long long int x;
{
  if (x >= 0)
    return TRUE;
  else
    return FALSE;
}

fgt (x)
     long long int x;
{
  if (x > 0)
    return TRUE;
  else
    return FALSE;
}

fle (x)
     long long int x;
{
  if (x <= 0)
    return TRUE;
  else
    return FALSE;
}

main ()
{
  if (feq (0LL) != TRUE)
    abort ();
  if (feq (-1LL) != FALSE)
    abort ();
  if (feq (MSB) != FALSE)
    abort ();
  if (feq (MSB_PLUS_1) != FALSE)
    abort ();
  if (feq (1LL) != FALSE)
    abort ();
  if (feq (MSB_MINUS_1) != FALSE)
    abort ();

  if (fne (0LL) != FALSE)
    abort ();
  if (fne (-1LL) != TRUE)
    abort ();
  if (fne (MSB) != TRUE)
    abort ();
  if (fne (MSB_PLUS_1) != TRUE)
    abort ();
  if (fne (1LL) != TRUE)
    abort ();
  if (fne (MSB_MINUS_1) != TRUE)
    abort ();

  if (flt (0LL) != FALSE)
    abort ();
  if (flt (-1LL) != TRUE)
    abort ();
  if (flt (MSB) != TRUE)
    abort ();
  if (flt (MSB_PLUS_1) != TRUE)
    abort ();
  if (flt (1LL) != FALSE)
    abort ();
  if (flt (MSB_MINUS_1) != FALSE)
    abort ();

  if (fge (0LL) != TRUE)
    abort ();
  if (fge (-1LL) != FALSE)
    abort ();
  if (fge (MSB) != FALSE)
    abort ();
  if (fge (MSB_PLUS_1) != FALSE)
    abort ();
  if (fge (1LL) != TRUE)
    abort ();
  if (fge (MSB_MINUS_1) != TRUE)
    abort ();

  if (fgt (0LL) != FALSE)
    abort ();
  if (fgt (-1LL) != FALSE)
    abort ();
  if (fgt (MSB) != FALSE)
    abort ();
  if (fgt (MSB_PLUS_1) != FALSE)
    abort ();
  if (fgt (1LL) != TRUE)
    abort ();
  if (fgt (MSB_MINUS_1) != TRUE)
    abort ();

  if (fle (0LL) != TRUE)
    abort ();
  if (fle (-1LL) != TRUE)
    abort ();
  if (fle (MSB) != TRUE)
    abort ();
  if (fle (MSB_PLUS_1) != TRUE)
    abort ();
  if (fle (1LL) != FALSE)
    abort ();
  if (fle (MSB_MINUS_1) != FALSE)
    abort ();

  exit (0);
}
