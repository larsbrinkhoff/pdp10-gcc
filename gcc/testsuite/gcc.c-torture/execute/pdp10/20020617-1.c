typedef unsigned uint32_t __attribute__ ((size (32)));
uint32_t foo = (uint32_t)-1;

int main ()
{
  if (foo != (uint32_t)-1)
    abort ();
  exit (0);
}
