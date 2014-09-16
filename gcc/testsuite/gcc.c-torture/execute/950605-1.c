f (c)
    unsigned char c;
{
#ifdef __pdp10__
  if (c != 0777)
#else
  if (c != 0xFF)
#endif
    abort ();
}

main ()
{
  f (-1);
  exit (0);
}
