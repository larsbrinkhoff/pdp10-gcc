char a[1] = { 42 };

char *f (void)
{
  /* Undefined, or something.  */
  return &a[-1];
}

int main ()
{
  char *p = f ();
  if (p[1] != 42)
    abort ();
  exit (0);
}
