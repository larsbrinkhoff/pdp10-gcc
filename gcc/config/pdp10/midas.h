/* Definitions for PDP-10 target machine using MIDAS assembler.
   Copyright (C) 2001, 2002 Free Software Foundation, Inc.
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

#ifndef __PDP10_MIDAS_H__
#define __PDP10_MIDAS_H__

#define ASM_COMMENT_START ";"

#define ASM_APP_ON ""

#define ASM_APP_OFF ""

#define IS_ASM_LOGICAL_LINE_SEPARATOR(C) ((C) == '?')

#define TEXT_SECTION_ASM_OP ""

#define DATA_SECTION_ASM_OP ""

#define ASM_OUTPUT_ALIGN(STREAM, POWER) \
  do { } while (0)

#define ASM_OUTPUT_ASCII(STREAM, PTR, LEN) \
  asm_output_asciz (STREAM, PTR, LEN)

#define ASM_GLOBALIZE_LABEL(STREAM,NAME)	\
  fputs ("\tENTRY\t", STREAM),			\
  assemble_name (STREAM, NAME);			\
  fputc ('\n', STREAM)

#define ASM_OUTPUT_EXTERNAL(STREAM, DECL, NAME)		\
do {							\
  if (strcmp (NAME, "__builtin_constant_p") == 0)	\
    break;						\
  fputs ("\tEXTERN\t", STREAM),				\
  assemble_name (STREAM, NAME);				\
  fputc ('\n', STREAM);					\
} while (0)

#define ASM_OUTPUT_EXTERNAL_LIBCALL(STREAM, SYMREF)	\
  fputs ("\tEXTERN\t", STREAM),				\
  assemble_name (STREAM, XSTR (SYMREF, 0));		\
  fputc ('\n', STREAM)

#define ASM_OUTPUT_COMMON(STREAM, NAME, SIZE, ROUNDED)	\
  assemble_name ((STREAM), (NAME));		     	\
  fputs (":\n\tBLOCK\t", STREAM); 		     	\
  fprintf (STREAM, "%d\n", (ROUNDED + 3) / 4)

#define ASM_IDENTIFY_GCC(FILE)				\
do {							\
  const char *gnu_comp = "gnu.";			\
  const char *lang = lang_identify ();			\
  int len = strlen (lang) + sizeof (gnu_comp) + 1;	\
  char *s = (char *) alloca (len);			\
  fputs ("gcc2.compiled:\n", FILE);			\
  sprintf (s, "%s%s", gnu_comp, lang);			\
  ASM_OUTPUT_LABEL (asm_out_file, s);			\
} while (0)

#define ASM_OUTPUT_DEF(FILE, LABEL, VALUE)	\
do {						\
  assemble_name (FILE, LABEL);			\
  fputs ("==", FILE);				\
  assemble_name (FILE, VALUE);			\
  fprintf (FILE, "\n");				\
} while (0)

#endif /* __PDP10_MIDAS_H__ */
