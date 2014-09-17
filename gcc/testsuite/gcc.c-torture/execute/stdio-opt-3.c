/* Copyright (C) 2001  Free Software Foundation.

   Ensure all expected transformations of builtin fprintf occur and
   that we honor side effects in the arguments.

   Written by Kaveh R. Ghazi, 1/7/2001.  */

#include <stdio.h>
extern int fprintf (FILE *, const char *, ...);
extern void abort(void);

int main()
{
  FILE *s_array[] = {stdout, NULL}, **s_ptr = s_array;
  const char *const s1 = "hello world";
  const char *const s2[] = { s1, 0 }, *const*s3;
  
  fprintf (*s_ptr, "%s", "hello");
  fprintf (*s_ptr, "%s", "\n");
  fprintf (*s_ptr, "%s", *s2);
  s3 = s2;
  fprintf (*s_ptr, "%s", *s3++);
  if (s3 != s2+1 || *s3 != 0)
    abort();
  s3 = s2;
  fprintf (*s_ptr++, "%s", *s3++);
  if (s3 != s2+1 || *s3 != 0 || s_ptr != s_array+1 || *s_ptr != 0)
    abort();
  
  s_ptr = s_array;
  fprintf (*s_ptr, "%c", '\n');
  fprintf (*s_ptr, "%c", **s2);
  s3 = s2;
  fprintf (*s_ptr, "%c", **s3++);
  if (s3 != s2+1 || *s3 != 0)
    abort();
  s3 = s2;
  fprintf (*s_ptr++, "%c", **s3++);
  if (s3 != s2+1 || *s3 != 0 || s_ptr != s_array+1 || *s_ptr != 0)
    abort();
  
  s_ptr = s_array;
  fprintf (*s_ptr++, "hello world");
  if (s_ptr != s_array+1 || *s_ptr != 0)
    abort();
  s_ptr = s_array;
  fprintf (*s_ptr, "\n");
    
  /* Test at least one instance of the __builtin_ style.  We do this
     to ensure that it works and that the prototype is correct.  */
  __builtin_fprintf (*s_ptr, "%s", "hello world\n");

  return 0;
}

#ifdef __OPTIMIZE__
/* When optimizing, all the above cases should be transformed into
   something else.  So any remaining calls to the original function
   should abort.  */
static int
fprintf (FILE *stream, const char *string, ...)
{
  abort();
}
#endif
