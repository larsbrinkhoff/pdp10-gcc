#include <stdio.h>

class c{
public:
  long double f;
};


static class sss: public c{
public:
  double m;
} sss;

#define _offsetof(st,f) ((char *)&((st *) 16)->f - (char *) 16)

int main (void) {
  printf ("++Class with double inhereting class with longdouble:\n");
  printf ("size=%d,align=%d\n", sizeof (sss), __alignof__ (sss));
  printf ("offset-longdouble=%d,offset-double=%d,\nalign-longdouble=%d,align-double=%d\n",
          _offsetof (class sss, f), _offsetof (class sss, m),
          __alignof__ (sss.f), __alignof__ (sss.m));
  return 0;
}
