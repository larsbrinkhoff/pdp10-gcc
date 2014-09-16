static char x[9];

#define F(n) void f##n (void) { x[n] = 'A'; }

F(0) F(1) F(2) F(3) F(4) F(5) F(6) F(7) F(8)

int main ()
{
  void (*f[]) (void) = { f0, f1, f2, f3, f4, f5, f6, f7, f8 };
  int i;

  for (i = 0; i < sizeof x; i++)
    {
      memset (x, 0, sizeof x);
      f[i] ();
      if (x[i] != 'A')
	abort ();
    }

  exit (0);
}
