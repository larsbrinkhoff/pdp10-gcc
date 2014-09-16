/* Definitions for PDP-10 target machine using MACRO assembler.
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

#ifndef __PDP10_MACRO_H__
#define __PDP10_MACRO_H__

#define TARGET_ASM_NAMED_SECTION pdp10_asm_named_section

#define ASM_FILE_START(STREAM) macro_file_start (STREAM)

#define ASM_FILE_END(STREAM) macro_file_end (STREAM)

#define ASM_COMMENT_START ";"

#define ASM_APP_ON ""

#define ASM_APP_OFF ""

#define IS_ASM_LOGICAL_LINE_SEPARATOR(C) 0

#define TEXT_SECTION_ASM_OP pdp10_text_section_asm_op
extern char *pdp10_text_section_asm_op;

#define DATA_SECTION_ASM_OP pdp10_data_section_asm_op
extern char *pdp10_data_section_asm_op;

#define RODATA_SECTION_ASM_OP pdp10_rodata_section_asm_op
extern char *pdp10_rodata_section_asm_op;

#define BSS_SECTION_ASM_OP pdp10_bss_section_asm_op
extern char *pdp10_bss_section_asm_op;

#define ASM_SECTION_END(STREAM) macro_section_end (STREAM)

#define EXTRA_SECTIONS in_rodata

#define EXTRA_SECTION_FUNCTIONS \
  RODATA_SECTION_FUNCTION

#define RODATA_SECTION_FUNCTION				\
void							\
pdp10_rodata_section ()					\
{							\
  if (in_section != in_rodata)				\
    {							\
      ASM_SECTION_END (asm_out_file);		\
      fputs (RODATA_SECTION_ASM_OP, asm_out_file);	\
      fputc ('\n', asm_out_file);			\
      in_section = in_rodata;				\
    }							\
}

#define READONLY_DATA_SECTION pdp10_rodata_section

#define ASM_OUTPUT_DOUBLE(STREAM, VALUE)		\
do {							\
  unsigned HOST_WIDE_INT high, low;			\
  long l[3];						\
  REAL_VALUE_TO_TARGET_DOUBLE ((VALUE), l);		\
  high = (((((HOST_WIDE_INT)l[0]) & 0xffffffffUL) << 4)	\
	  + (HOST_WIDE_INT)((l[1] >> 28) & 0xfUL));	\
  low = ((((HOST_WIDE_INT)l[1] << 8) & 0xfffffffUL)	\
	 + (HOST_WIDE_INT)((l[2] >> 24) & 0xffUL));	\
  fprintf ((STREAM), "\tEXP %llo,%llo\n", high, low);	\
} while (0)

#define ASM_OUTPUT_FLOAT(STREAM, VALUE)		\
do {						\
  char str[30];					\
  REAL_VALUE_TO_DECIMAL ((VALUE), "%.8g", str);	\
  fprintf ((STREAM), "\t%s\n", str);		\
} while (0);

/*  #define ASM_OUTPUT_DOUBLE_INT(STREAM, EXP)	\ */
/*  do {						\ */
/*    pdp10_output_byte ((STREAM), 0, 0);		\ */
/*    fputc ('\t', STREAM);				\ */
/*    pdp10_print_operand (STREAM, EXP, 'D');	\ */
/*    fputc ('\n', STREAM);				\ */
/*  } while (0) */

/*  #define ASM_OUTPUT_INT(STREAM, EXP)		\ */
/*    pdp10_output_byte (STREAM, 0, 0);		\ */
/*    fputc ('\t', STREAM);				\ */
/*    pdp10_print_operand (STREAM, EXP, 0);		\ */
/*    fputc ('\n', STREAM); */

/*  #define ASM_OUTPUT_SHORT(STREAM, EXP) \ */
/*    pdp10_output_byte (STREAM, EXP, 18) */

/*  #define ASM_OUTPUT_CHAR(STREAM, EXP) \ */
/*    pdp10_output_byte (STREAM, EXP, 9) */

/*  #define ASM_OUTPUT_BYTE(STREAM, VALUE) \ */
/*    pdp10_output_byte (STREAM, GEN_INT (VALUE), 9) */

#define ASM_OUTPUT_INTERNAL_LABEL(STREAM, PREFIX, NUM)		\
  pdp10_output_byte ((STREAM), 0, 0);				\
  fprintf ((STREAM), "%%%s%u:!\n", (PREFIX), (unsigned)(NUM))

#define LOCAL_LABEL_PREFIX "%"

#define ASM_OUTPUT_ADDR_VEC_ELT(STREAM, VALUE)				    \
 do {									    \
   fprintf ((STREAM), "\t%s%s%d\n",					    \
	    /* Surprisingly, GIWs are faster than local indirect words.  */ \
	    TARGET_EXTENDED ? "GIW " : "", LOCAL_LABEL_PREFIX, (VALUE));    \
   pdp10_giw++;								    \
 } while (0)

#define ASM_OUTPUT_SKIP(STREAM, NBYTES)					\
do {									\
  if ((NBYTES) < 4)							\
    {									\
      int i = (NBYTES);							\
      while (i--)							\
        pdp10_output_byte ((STREAM), const0_rtx, 9);			\
    }									\
  else									\
    {									\
      int n = (NBYTES) - pdp10_align_with_pad (0);			\
      pdp10_output_byte ((STREAM), 0, 0);				\
      fprintf ((STREAM), "\tBLOCK\t%o\n", n / 4);			\
      pdp10_output_byte ((STREAM), const0_rtx, 9 * (n % 4));		\
    }									\
} while (0)

#define ASM_OUTPUT_ALIGN(STREAM, POWER) \
  do { } while (0)

#define ASM_OUTPUT_ASCII(STREAM, PTR, LEN) \
  macro_output_ascii (STREAM, PTR, LEN)

#define ASM_GLOBALIZE_LABEL(STREAM, NAME) \
  macro_globalize_label (STREAM, NAME)

enum
{
  PDP10_SYMBOL_EXTERN = 0x01,
  PDP10_SYMBOL_USED = 0x02,
  PDP10_SYMBOL_DEFINED = 0x04
};

#define ASM_OUTPUT_EXTERNAL(STREAM, DECL, NAME)		\
do {							\
  if (strncmp (NAME, "__builtin_", 10) == 0)		\
    break;						\
  pdp10_extern_symbol (NAME, PDP10_SYMBOL_EXTERN);	\
} while (0)

#define ASM_OUTPUT_EXTERNAL_LIBCALL(STREAM, SYMREF) \
  pdp10_extern_symbol (XSTR (SYMREF, 0), PDP10_SYMBOL_EXTERN);

#define ASM_OUTPUT_COMMON(STREAM, NAME, SIZE, ROUNDED)	\
do {							\
  bss_section ();					\
  pdp10_output_byte (STREAM, 0, 0);			\
  fputs ("\tENTRY\t", STREAM);				\
  assemble_name (STREAM, (NAME));			\
  fputc ('\n', STREAM);					\
  assemble_name (STREAM, (NAME));			\
  fputs (":\n\tBLOCK\t", STREAM);			\
  fprintf (STREAM, "%o\n", ((ROUNDED) + 3) / 4);	\
  pdp10_extern_symbol ((NAME), PDP10_SYMBOL_DEFINED);	\
} while (0)

#if 0
#define ASM_OUTPUT_ALIGNED_DECL_LOCAL(STREAM, DECL, NAME, SIZE, ALIGN)	\
do {									\
  rtx rtl = DECL_RTL (DECL);						\
  rtx sym; = XEXP (rtl, 0);						\
  if (GET_CODE (sym) != SYMBOL_REF)					\
    sym = XEXP (XEXP (sym, 0), 0);					\
  ASM_OUTPUT_LABEL (STREAM, XSTR (sym, 0));				\
  fprintf (STREAM, "\tBLOCK\t%o\n", (((SIZE) + 3) / 4));		\
} while (0)
#else
#define ASM_OUTPUT_LOCAL(STREAM, NAME, SIZE, ROUNDED)		\
do {								\
  bss_section ();						\
  ASM_OUTPUT_LABEL (STREAM, (NAME));				\
  fprintf (STREAM, "\tBLOCK\t%o\n", (((ROUNDED) + 3) / 4));	\
} while (0)
#endif

#define ASM_OUTPUT_LABEL(STREAM, NAME)		\
do {						\
  const char *c = (NAME);			\
  assemble_name ((STREAM), (NAME));		\
  fputc (':', (STREAM));			\
  if (*c == '*')				\
    c++;					\
  if (*c == '%')				\
    fputc ('!', (STREAM));			\
  fputc ('\n', (STREAM));			\
} while (0)

#define ASM_DECLARE_FUNCTION_NAME(STREAM, NAME, DECL)		\
do {								\
  if (! DECL_WEAK (DECL))					\
    SYMBOL_REF_INTERNAL (XEXP (DECL_RTL (DECL), 0)) = 1;	\
  ASM_OUTPUT_LABEL ((STREAM), (NAME));				\
  pdp10_extern_symbol ((NAME), PDP10_SYMBOL_DEFINED);		\
} while (0)

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

#define ASM_OUTPUT_DEF(FILE, LABEL, VALUE) \
  macro_output_def ((FILE), (LABEL), (VALUE))

#define ASM_OUTPUT_WEAK_ALIAS(STREAM, NAME, VALUE) \
  ASM_OUTPUT_DEF (STREAM, NAME, VALUE)

#if 0 /* Remove me.  */
/* MACRO doesn't support DBX debug info.  */
#define ASM_STABS_OP "\t;stabs\t"
#define ASM_STABD_OP "\t;stabd\t"
#define ASM_STABN_OP "\t;stabn\t"

/* Don't break long debug info lines.  */
#define DBX_CONTIN_LENGTH 0
#endif

#define DEBUG_INFO_SECTION	".dbinf"
#define DEBUG_ABBREV_SECTION	".dbabb"
#define DEBUG_ARANGES_SECTION	".dbarn"
#define DEBUG_MACINFO_SECTION	".dbmac"
#define DEBUG_LINE_SECTION	".dblin"
#define DEBUG_LOC_SECTION	".dbloc"
#define DEBUG_PUBNAMES_SECTION	".dbpub"
#define DEBUG_STR_SECTION	".dbstr"
#define DEBUG_RANGES_SECTION	".dbrng"
#define DEBUG_FRAME_SECTION	".dbfrm"

#define TEXT_SECTION_LABEL		"Ltxt"
#define DEBUG_LINE_SECTION_LABEL	"Ldbl"
#define DEBUG_INFO_SECTION_LABEL	"Ldbi"
#define DEBUG_ABBREV_SECTION_LABEL	"Ldba"
#define DEBUG_LOC_SECTION_LABEL		"Ldbl"
#define DEBUG_RANGES_SECTION_LABEL	"Ldbr"
#define DEBUG_MACINFO_SECTION_LABEL     "Ldbm"
#define TEXT_END_LABEL			"Letx"
#define DATA_END_LABEL			"Ledt"
#define BSS_END_LABEL			"Lebs"
#define BLOCK_BEGIN_LABEL		"BB"
#define BLOCK_END_LABEL			"BE"

#define UMODDI3_LIBCALL "_umoddi3"
#define UDIVDI3_LIBCALL "_udivdi3"

#endif /* __PDP10_MACRO_H__ */
