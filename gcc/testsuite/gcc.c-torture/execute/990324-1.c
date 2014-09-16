void f(long i)
{
  if ((signed char)i < 0 || (signed char)i == 0) 
    abort ();
  else
    exit (0);
}

main()
{
#ifdef __pdp10__
  f(0777777777001);
#else
  f(0xffffff01);
#endif
}

