#ifdef __pdp10__
typedef unsigned char6 __attribute__ ((size (6)));
typedef unsigned char7 __attribute__ ((size (7)));
typedef unsigned char8 __attribute__ ((size (8)));
#define DO(F) F (char6, 12) F (char7, 10) F (char8, 8) F (char, 8) F (short, 4)
#else
#define DO(F) F (char, 8) F (short, 4)
#endif

#define DEF(T, N)				\
  static T a##T[N];				\
  static d##T (T *x, T *y) { return x - y; }

#define TEST(T, N)				\
  for (i = 0; i < N; i++)			\
    for (j = 0; j < N; j++)			\
      if (d##T (&a##T[i], &a##T[j]) != i - j)	\
        exit (1);

DO (DEF)

int
main ()
{
  int i, j;
  DO (TEST)
  exit (0);
}
