/* Test for builtin noreturn attributes.  */
/* Origin: Joseph Myers <jsm28@cam.ac.uk> */

extern void abort (void);
extern void exit (int);
#if 0 /* Doesn't work with prototype (bug?).  */
extern void _exit (int);
extern void _Exit (int);
#endif

extern void tabort (void);
extern void texit (void);
extern void t_exit (void);
#ifndef __pdp10__
extern void t_Exit (void);
#endif

extern void link_failure (void);

int
main (void)
{
  volatile int i = 0;
  /* The real test here is that the program links.  */
  if (i)
    tabort ();
  if (i)
    texit ();
  if (i)
    t_exit ();
#ifndef __pdp10__
  if (i)
    t_Exit ();
#endif
  exit (0);
}

void
tabort (void)
{
  abort ();
  link_failure ();
}

void
texit (void)
{
  exit (1);
  link_failure ();
}

void
t_exit (void)
{
  _exit (1);
  link_failure ();
}

/* Some non-Unix libcs might not have _exit.  This version should never
   get called.  */
static void
_exit (int i)
{
  abort ();
}

#ifndef __pdp10__
void
t_Exit (void)
{
  _eexit (1);
  link_failure ();
}

/* Some libcs might not have _Exit.  This version should never get called.  */
static void
_Exit (int i)
{
  abort ();
}
#endif

/* When optimizing, no calls to link_failure should remain.  In any case,
   link_failure should not be called.  */

#ifndef __OPTIMIZE__
void
link_failure (void)
{
  abort ();
}
#endif
