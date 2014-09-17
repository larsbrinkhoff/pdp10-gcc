/* Definitions for specs for the GNU compiler for the Java(TM) language.
   Copyright (C) 1996, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.

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
Boston, MA 02111-1307, USA.

Java and all Java-based marks are trademarks or registered trademarks
of Sun Microsystems, Inc. in the United States and other countries.
The Free Software Foundation is independent of Sun Microsystems, Inc.  */

/* This is the contribution to the `default_compilers' array in gcc.c for
   Java.  */

  {".java",   "@java" , 0},
  {".class",  "@java" , 0},
  {".zip",    "@java" , 0},
  {".jar",    "@java" , 0},
  {"@java",
   "%{fjni:%{femit-class-files:%e-fjni and -femit-class-files are incompatible}}\
    %{fjni:%{femit-class-file:%e-fjni and -femit-class-file are incompatible}}\
    %{femit-class-file:%{!fsyntax-only:%e-femit-class-file should used along with -fsyntax-only}}\
    %{femit-class-files:%{!fsyntax-only:%e-femit-class-file should used along with -fsyntax-only}}\
    %{!E:jc1 %i %(jc1) %(cc1_options) %{+e*} %{I*}\
             %{MD} %{MMD} %{M} %{MM} %{MA} %{MT*} %{MF*}\
             %{!fsyntax-only:%(invoke_as)}}", 0},

