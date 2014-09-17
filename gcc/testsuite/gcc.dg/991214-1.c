/* { dg-do compile { target i?86-*-* } } */
/* { dg-options "-O2" } */

/* Test against a problem with the combiner substituting explicit hard reg
   references when it shouldn't.  */
void foo (int, int) __attribute__ ((regparm (3)));
void foo (int x, int y)
{
  __asm__ __volatile__("" : : "d" (x), "r" (y));
}
