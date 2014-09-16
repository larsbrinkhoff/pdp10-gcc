/* Configuration for GCC for PDP-10 hosts.
   Copyright (C) 2001 Free Software Foundation, Inc.
   Contributed by Lars Brinkhoff (lars@nocrew.org), funded by XKL, LLC.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

/* A C expression for the status code to be returned when the compiler
   exits after a serious error.  */
#define FATAL_EXIT_CODE 33

/* A C expression for the status code to be returned when the compiler
   exits without serious error.  */
#define SUCCESS_EXIT_CODE 0

/* The PDP-10 stores words of multi-word values in big-endian order.  */
#define HOST_WORDS_BIG_ENDIAN 1

/* A code distinguishing the floating point format of the host
   machine.  */
#define HOST_FLOAT_FORMAT PDP10_FLOAT_FORMAT

/* A C expression for the number of bits in char on the host machine.  */
#define HOST_BITS_PER_CHAR	 9

/* A C expression for the number of bits in short on the host machine.  */
#define HOST_BITS_PER_SHORT	18

/* A C expression for the number of bits in int on the host machine.  */
#define HOST_BITS_PER_INT	36

/* A C expression for the number of bits in long on the host machine.  */
#define HOST_BITS_PER_LONG	36

/* A C expression for the number of bits in long long on the host
   machine.  */
#define HOST_BITS_PER_LONGLONG	72

/* If not compiled with GNU C, use C alloca.  */
#ifndef __GNUC__
#define USE_C_ALLOCA
#endif

/* Define this if your system does not provide the variable
   sys_siglist.  */
#define NO_SYS_SIGLIST

/* Define this to be 1 if you know the host compiler supports
   prototypes, even if it doesn't define __STDC__, or define it to be
   0 if you do not want any prototypes when compiling GCC.  */
#define USE_PROTOTYPES 1

/* target machine dependencies.
   tm.h is a symbolic link to the actual target specific file.  */
#include "tm.h"

/* EOF xm-pdp10.h */
