typedef void  *(*T)(void);
#if 0
f1 ()
{
  ((T) 0)();
}
#endif
f2 ()
{
  ((T) 1000)();
}
f3 ()
{
  ((T) 10000000)();
}
#if 0
f4 (r)
{
  ((T) r)();
}
f5 ()
{
  int (*r)() = f3;
  ((T) r)();
}
#endif
