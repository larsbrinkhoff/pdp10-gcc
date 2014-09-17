/* Simplified from PR target/5309.  */
/* { dg-do compile } */
/* { dg-options "-O2" } */
/* { dg-options "-O2 -m64 -mcpu=ultrasparc" { target sparc64-*-* } } */
/* { dg-options "-O2 -m64 -mcpu=ultrasparc" { target sparcv9-*-* } } */
/* { dg-options "-O2" { target sparc-*-solaris2.[0-6] } } */
/* { dg-options "-O2" { target sparc-*-solaris2.[0-6].* } } */

long bar (unsigned int);
long foo (long x, unsigned int y)
{
  return *(((long *) (bar (y) - 1)) + 1 + (x >> 2) % 359);
}
