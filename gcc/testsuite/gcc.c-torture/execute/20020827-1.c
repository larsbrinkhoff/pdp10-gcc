/* The PDP-10 port would fail when compiling 20020813-1.c with -O3.
   This test distills the problematic code to demonstrate the bug.

   Loading and storing a byte at an offset (equal to an integral
   multiple of words) from a base address would incorrectly use
   indexed addressing.  Instead the address must be incremented or
   decremented as appropriate to point to the correct byte before
   accessing memory.  */

void f (char *x)
{
  x[4] = 42;
}

void g2 (char *x)
{
  if (x[0] != 42)
    abort ();
}

void g (void)
{
  char x[5];
  x[0] = 42;
  g2 (x);
}

char h (char *x)
{
  return x[4];
}

char x[5];

int main ()
{
  x[4] = 0;
  f (x);
  if (x[4] != 42)
    abort ();

  g ();

  x[4] = 42;
  if (h (x) != 42)
    abort ();

  exit (0);
}
