#include <stdio.h>

static struct sss{
  long f;
  long double snd;
} sss;

#define _offsetof(st,f) ((char *)&((st *) 16)->f - (char *) 16)

int main (void) {
  printf ("+++Struct long-longdouble:\n");
  printf ("size=%d,align=%d,offset-long=%d,offset-longdouble=%d,\nalign-long=%d,align-longdouble=%d\n",
          sizeof (sss), __alignof__ (sss),
          _offsetof (struct sss, f), _offsetof (struct sss, snd),
          __alignof__ (sss.f), __alignof__ (sss.snd));
  return 0;
}
