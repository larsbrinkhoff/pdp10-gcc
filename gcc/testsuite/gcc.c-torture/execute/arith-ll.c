typedef long long T;

static T add (T a, T b)
{
  return a + b;
}

static T sub (T a, T b)
{
  return a - b;
}

static T mul (T a, T b)
{
  return a * b;
}

static T div (T a, T b)
{
  return a / b;
}

#define TRY(f, op)							\
do {									\
  if (*op != '/' || x[i].b != 0)					\
    {									\
      T y = f (x[i].a, x[i].b);						\
      if (y != x[i].f)							\
	{								\
	  printf ("%d %s %d = %d != %d\n",				\
		  (int)x[i].a, op, (int)x[i].b, (int)y, (int)x[i].f);	\
	  failures++;							\
	}								\
    }									\
} while (0)

#define TEST(a,b)	TEST3 ((a), (b), (a)/(b))
#define TESTZ(a,b)	TEST3 ((a), (b), 0)
#define TEST3(a,b,q)	{ (a), (b), (a)+(b), (a)-(b), (a)*(b), (q) }

struct foo
{
  T a, b, add, sub, mul, div;
};

int main ()
{
  int i, failures = 0;
  static const struct foo x[] =
  {
    TEST  (	-2,	-1),
    TESTZ (	-2,	 0),
    TEST  (	-2,	 1),
    TEST  (	-1,	-1),
    TESTZ (	-1,	 0),
    TEST  (	-1,	 1),
    TEST  (	 0,	-1),
    TESTZ (	 0,	 0),
    TEST  (	 0,	 1),
    TEST  (	 1,	-1),
    TESTZ (	 1,	 0),
    TEST  (	 1,	 1),
    TEST  (	 2,	-1),
    TESTZ (	 2,	 0),
    TEST  (	 2,	 1)
  };

  for (i = 0; i < sizeof x / sizeof x[0]; i++)
    {
      TRY (add, "+");
      TRY (sub, "-");
      TRY (mul, "*");
      TRY (div, "/");
    }

  if (failures)
    abort ();
  exit (0);
}
