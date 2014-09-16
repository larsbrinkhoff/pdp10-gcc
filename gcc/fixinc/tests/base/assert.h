/*  DO NOT EDIT THIS FILE.

    It has been auto-edited by fixincludes from:

	"fixinc/tests/inc/assert.h"

    This had to be done to correct non-standard usages in the
    original, manufacturer supplied header file.  */

#ifndef FIXINC_BROKEN_ASSERT_STDLIB_CHECK
#define FIXINC_BROKEN_ASSERT_STDLIB_CHECK 1

#ifdef __cplusplus
#include <stdlib.h>
#endif
#ifndef FIXINC_BROKEN_ASSERT_STDIO_CHECK
#define FIXINC_BROKEN_ASSERT_STDIO_CHECK 1

#include <stdio.h>


#if defined( ALPHA___ASSERT_CHECK )
extern void __assert(const char *, const char *, int);
#endif  /* ALPHA___ASSERT_CHECK */


#if defined( BROKEN_ASSERT_STDIO_CHECK )
extern FILE* stderr;
#endif  /* BROKEN_ASSERT_STDIO_CHECK */


#if defined( BROKEN_ASSERT_STDLIB_CHECK )
extern void exit ( int );
#endif  /* BROKEN_ASSERT_STDLIB_CHECK */

#endif  /* FIXINC_BROKEN_ASSERT_STDIO_CHECK */

#endif  /* FIXINC_BROKEN_ASSERT_STDLIB_CHECK */