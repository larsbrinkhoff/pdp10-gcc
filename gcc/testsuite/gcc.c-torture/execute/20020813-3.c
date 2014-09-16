char *ff;

void bar (int x)
{
  if (x != 42)
    abort ();
}

void foo(int c)
{
  bar(c);
  ff -= c;
  bar(c);
}

int main ()
{
  foo (42);
  exit (0);
}
