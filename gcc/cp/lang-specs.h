/* Definitions for specs for C++.
   Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* This is the contribution to the `default_compilers' array in gcc.c for
   g++.  */

#ifndef CPLUSPLUS_CPP_SPEC
#define CPLUSPLUS_CPP_SPEC 0
#endif

  {".cc",  "@c++", 0},
  {".cp",  "@c++", 0},
  {".cxx", "@c++", 0},
  {".cpp", "@c++", 0},
  {".c++", "@c++", 0},
  {".C",   "@c++", 0},
  {"@c++",
   /* cc1plus has an integrated ISO C preprocessor.  We should invoke
      the external preprocessor if -save-temps is given.  */
    "%{E|M|MM:cc1plus -E -lang-c++ %{!no-gcc:-D__GNUG__=%v1}\
       %{!Wno-deprecated:-D__DEPRECATED}\
       %{!fno-exceptions:-D__EXCEPTIONS}\
       -D__GXX_ABI_VERSION=100\
       %{ansi:-D__STRICT_ANSI__ -trigraphs -$} %(cpp_options)}\
     %{!E:%{!M:%{!MM:\
       %{save-temps:cc1plus -E -lang-c++ \
		    %{!no-gcc:-D__GNUG__=%v1}\
       		    %{!Wno-deprecated:-D__DEPRECATED}\
		    %{!fno-exceptions:-D__EXCEPTIONS}\
		    -D__GXX_ABI_VERSION=100\
		    %{ansi:-D__STRICT_ANSI__ -trigraphs -$}\
		    %(cpp_options) %b.ii \n}\
      cc1plus %{save-temps:-fpreprocessed %b.ii}\
              %{!save-temps:%(cpp_unique_options)\
			    %{!no-gcc:-D__GNUG__=%v1} \
       			    %{!Wno-deprecated:-D__DEPRECATED}\
			    %{!fno-exceptions:-D__EXCEPTIONS}\
			    -D__GXX_ABI_VERSION=100\
			    %{ansi:-D__STRICT_ANSI__}}\
       %{ansi:-trigraphs -$}\
       %(cc1_options) %2 %{+e1*}\
       %{!fsyntax-only:%(invoke_as)}}}}",
     CPLUSPLUS_CPP_SPEC},
  {".ii", "@c++-cpp-output", 0},
  {"@c++-cpp-output",
   "%{!M:%{!MM:%{!E:\
    cc1plus -fpreprocessed %i %(cc1_options) %2 %{+e*}\
    %{!fsyntax-only:%(invoke_as)}}}}", 0},
