/* Definitions for AMD x86-64 running Linux-based GNU systems with ELF format.
   Copyright (C) 2001, 2002 Free Software Foundation, Inc.
   Contributed by Jan Hubicka <jh@suse.cz>, based on linux.h.

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

#define LINUX_DEFAULT_ELF

#define TARGET_VERSION fprintf (stderr, " (x86-64 Linux/ELF)");

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-D__ELF__ -Dunix -D__gnu_linux__ -Dlinux -Asystem(posix)"

#undef CPP_SPEC
#define CPP_SPEC "%(cpp_cpu) %{fPIC:-D__PIC__ -D__pic__} %{fpic:-D__PIC__ -D__pic__} %{posix:-D_POSIX_SOURCE} %{pthread:-D_REENTRANT} %{!m32:-D__LONG_MAX__=9223372036854775807L}"

/* Provide a LINK_SPEC.  Here we provide support for the special GCC
   options -static and -shared, which allow us to link things in one
   of these three modes by applying the appropriate combinations of
   options at link-time.

   When the -shared link option is used a final link is not being
   done.  */

#undef	LINK_SPEC
#define LINK_SPEC "%{!m32:-m elf_x86_64 -Y P,/usr/lib64} %{m32:-m elf_i386} \
  %{shared:-shared} \
  %{!shared: \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{m32:%{!dynamic-linker:-dynamic-linker /lib/ld-linux.so.2}} \
      %{!m32:%{!dynamic-linker:-dynamic-linker /lib64/ld-linux-x86-64.so.2}}} \
    %{static:-static}}"

#undef  STARTFILE_SPEC
#define STARTFILE_SPEC \
  "%{m32:%{!shared: \
       %{pg:gcrt1.o%s} %{!pg:%{p:gcrt1.o%s} \
       %{!p:%{profile:gcrt1.o%s} %{!profile:crt1.o%s}}}} \
     crti.o%s %{static:crtbeginT.o%s}\
     %{!static:%{!shared:crtbegin.o%s} %{shared:crtbeginS.o%s}}} \
   %{!m32:%{!shared: \
       %{pg:/usr/lib64/gcrt1.o%s} %{!pg:%{p:/usr/lib64/gcrt1.o%s} \
       %{!p:%{profile:/usr/lib64/gcrt1.o%s} %{!profile:/usr/lib64/crt1.o%s}}}}\
     /usr/lib64/crti.o%s %{static:crtbeginT.o%s} \
     %{!static:%{!shared:crtbegin.o%s} %{shared:crtbeginS.o%s}}}"

#undef  ENDFILE_SPEC
#define ENDFILE_SPEC "\
  %{m32:%{!shared:crtend.o%s} %{shared:crtendS.o%s} crtn.o%s} \
  %{!m32:%{!shared:crtend.o%s} %{shared:crtendS.o%s} /usr/lib64/crtn.o%s}"
