int read (void)
{
  return 42;
}

struct moo {
  int     (*fo_read)      (void); 
} fo = { read };

struct foo {
  struct moo *f_ops;
  int     (*f_read)      (void); 
} f = { &fo, read };

int     
foo (int (*read) (void))
{
  return (*read)();
}

int     
bar (struct foo *fp)
{
  return (*fp->f_read)();
}

int     
baz (struct foo *fp)
{
  return (*fp->f_ops->fo_read)();
}

int main ()
{
  if (foo (read) != 42)
    abort ();
  if (bar (&f) != 42)
    abort ();
  if (baz (&f) != 42)
    abort ();
  exit (0);
}
