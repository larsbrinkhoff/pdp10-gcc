/* Copyright (C) 2000, 2001  Free Software Foundation.

   Ensure all expected transformations of builtin fputs occur and that
   we honor side effects in the stream argument.

   Written by Kaveh R. Ghazi, 10/30/2000.  */

#include <stdio.h>
extern void abort(void);
/* Declare this without args because that's what gcc does internally.
   We want to make sure it works without a helpful prototype from us.
   If stdio.h provides one, that is okay.  */
extern int fputs();

int main()
{
  FILE *s_array[] = {stdout, NULL}, **s_ptr = s_array;
  const char *const s1 = "hello world";
  
  fputs ("", *s_ptr);
  fputs ("\n", *s_ptr);
  fputs ("bye", *s_ptr);
  fputs (s1, *s_ptr);
  fputs (s1+5, *s_ptr);
  fputs (s1+10, *s_ptr);
  fputs (s1+11, *s_ptr);
  
  /* Check side-effects when transforming fputs -> NOP.  */
  fputs ("", *s_ptr++);
  if (s_ptr != s_array+1 || *s_ptr != 0)
    abort();

  /* Check side-effects when transforming fputs -> fputc.  */
  s_ptr = s_array;
  fputs ("\n", *s_ptr++);
  if (s_ptr != s_array+1 || *s_ptr != 0)
    abort();

  /* Check side-effects when transforming fputs -> fwrite.  */
  s_ptr = s_array;
  fputs ("hello\n", *s_ptr++);
  if (s_ptr != s_array+1 || *s_ptr != 0)
    abort();

  /* Test at least one instance of the __builtin_ style.  We do this
     to ensure that it works and that the prototype is correct.  */
  s_ptr = s_array;
  __builtin_fputs ("", *s_ptr);
  /* These builtin stubs are called by __builtin_fputs, ensure their
     prototypes are set correctly too.  */
  __builtin_fputc ('\n', *s_ptr);
  __builtin_fwrite ("hello\n", 1, 6, *s_ptr);

  return 0;
}

#ifdef __OPTIMIZE__
/* When optimizing, all the above cases should be transformed into
   something else.  So any remaining calls to the original function
   should abort.  */
static int
fputs(const char *string, FILE *stream)
{
  abort();
}
#endif
