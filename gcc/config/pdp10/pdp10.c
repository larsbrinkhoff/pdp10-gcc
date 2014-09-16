/* Output routines for the PDP-10.
   Copyright (C) 2001, 2002 Free Software Foundation, Inc.
   Contributed by Lars Brinkhoff (lars@nocrew.org), funded by XKL, LLC.

For information about the PDP-10 archtecture, see the README file in
this directory.

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


/**********************************************************************

	Index

**********************************************************************/

/*	  Front Page
	  Index
	  Includes
	  To-do List
	  Miscellaneous
	  Local variables.
	  GCC target structure.
	pdp10.h support:
	  Run-time Target Specification
	  Storage Layout
	  Layout of Source Language Data Types
	  Register Usage
	  Register Classes
	  Stack Layout and Calling Conventions
	  Addressing Modes
	  Dividing the Output into Sections
	  Describing Relative Costs of Operations
	  Defining the Output Assembler Language
	  Target Attributes
	  Miscellaneous Parameters
	  Special Predicates
	  Built-in Functions
	  Support for MACRO, MIDAS, and FAIL
	pdp10.md support:
	  Data Movement
	  Pointer Arithmetic
	  Unconditional Jumps
	  Conditional Jumps
	  Function Prologue and Epilogue
	junkyard:
	  Unsorted */
    

/**********************************************************************

	Includes

**********************************************************************/

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tree.h"
#include "expr.h"
#include "optabs.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "real.h"
#include "insn-config.h"
#include "conditions.h"
#include "insn-flags.h"
#include "output.h"
#include "insn-attr.h"
#include "flags.h"
#include "reload.h"
#include "toplev.h"
#include "recog.h"
#include "function.h"
#include "ggc.h"
#include "pdp10-protos.h"
#include "c-tree.h"
#include "hash.h"
#include "target.h"
#include "target-def.h"
#include "tm_p.h"


/**********************************************************************

	To-do List

**********************************************************************/

/* Only use single_push/single_pop when optimize_size ||
   !TARGET_EXTENDED?  */

/* Candidates for inlining once the code settles:

   pdp10_align_stack_pointer, const_ok_for_letter_p,
   const_double_ok_for_letter_p, extra_constraint,
   starting_frame_offset, stack_pointer_offset, first_parm_offset,
   stack_dynamic_offset, function_profiler, legitimate_address_p,
   legitimize_address, pdp10_const_int_costs,
   pdp10_const_double_costs, pdp10_operand_cost, encode_section_info,
   asm_declare_object_name, asm_output_labelref,
   asm_output_symbol_ref, print_operand_address,
   pdp10_output_call_as_jrst_p.  */

/* Investigate whether it's safe for expand_blt to use BLT when
   operands[2] is not constant.  */

/* Use global format indirect words?  Recognize global format indirect
   word in LEGITIMATE_ADDRESS_P?  */

/* pdp10_function_profiler.  */

/* pdp10_bytesize.  */


/**********************************************************************

	Miscellaneous

**********************************************************************/

#define SAVE		1
#define RESTORE		0

#define HWINT(X) ((HOST_WIDE_INT)(X))
#define SIGN_BIT (HWINT (1) << 35)
#if HOST_BITS_PER_WIDE_INT > 36
#define WORD_MASK ((HWINT (1) << 36) - 1)
#else
#define WORD_MASK (HWINT (-1))
#endif

#define WORD_MODE_P(MODE) \
  (GET_MODE_SIZE (MODE) >= UNITS_PER_WORD || (MODE) == BLKmode)
#define BYTE_MODE_P(MODE) (!WORD_MODE_P (MODE))

#define WORD_TYPE_P(TYPE) (pdp10_bytesize (TYPE) >= 32)
#define BYTE_TYPE_P(TYPE) (!WORD_TYPE_P (TYPE))

#define VALID_OWGBP_BYTE_SIZE_P(SIZE) \
 ((SIZE) == 6 || (SIZE) == 7 || (SIZE) == 8 || (SIZE) == 9 || (SIZE) == 18)

#define GEN_EQ gen_rtx_EQ (VOIDmode, NULL_RTX, NULL_RTX)
#define GEN_NE gen_rtx_NE (VOIDmode, NULL_RTX, NULL_RTX)

#define GEN_MEM_SP_PLUS(mode, offset) \
  gen_rtx_MEM (mode, plus_constant (stack_pointer_rtx, offset))

#define gen_ADDRESS(X) \
  gen_rtx_UNSPEC (Pmode, gen_rtvec (1, (X)), UNSPEC_ADDRESS)

#if !HAVE_IBP
#define gen_IBP(OP0, OP1) gen_ADJBP (OP0, OP1, GEN_INT (1))
#endif

static void	pdp10_print_number PARAMS ((FILE *stream,
					    HOST_WIDE_INT x,
					    int div,
					    int bits,
					    int zero));

#define ADDRESS_MASK (TARGET_EXTENDED ? HWINT (07777777777) : HWINT (0777777))

#define pdp10_print_address(stream, x) \
  pdp10_print_number (stream, x, 1, TARGET_EXTENDED ? 30 : 18, 0)
#define pdp10_print_offset30(stream, x) \
  pdp10_print_number (stream, x, 1, TARGET_EXTENDED ? -30 : -18, 1)
#define pdp10_print_offset(stream, x) \
  pdp10_print_number (stream, x, 1, -18, 1)
#define pdp10_print_integer(stream, x) \
  pdp10_print_number (stream, x, 1, -36, 0)
#define pdp10_print_unsigned(stream, x) \
  pdp10_print_number (stream, x, 1, 36, 0)

#define PRINT_OP_NO_BRACKETS	0x100
#define PRINT_OP_GIW		0x200
#define PRINT_OP_INDIRECT	0x400

#define SAME_REG(op0, op1) REG_P (op1) && REGNO (op0) == REGNO (op1)


/**********************************************************************

	Local variables.

**********************************************************************/

static int expand_extzv_16_0 = 0;
static int output_extzv_16_0 = 0;
static int expand_extzv_16_1 = 0;
static int output_extzv_16_1 = 0;
static int expand_extzv_32 = 0;
static int output_extzv_32 = 0;
int pdp10_output_ldbe_9 = 0;
static int expand_extv_16_0 = 0;
static int output_extv_16_0 = 0;
static int expand_extv_16_1 = 0;
static int output_extv_16_1 = 0;
static int expand_extv_32 = 0;
static int output_extv_32 = 0;
static int expand_extv = 0;
static int output_extv = 0;
int pdp10_expand_ffs = 0;

/* The number of registers used for argument passing.  */
int pdp10_regparm = PDP10_DEFAULT_REGPARM;

/* The number of registers used to return a value.  */
int pdp10_retregs = PDP10_DEFAULT_REGPARM;

/* ... */
int pdp10_giw = 0;
static int used_byte_subtraction_table[19];

int pdp10_outputting_debug_info = 0;


/**********************************************************************

	GCC target structure.

**********************************************************************/

const struct attribute_spec pdp10_attribute_table[];
static void	pdp10_asm_named_section PARAMS ((const char *, unsigned int));
static bool	pdp10_assemble_integer PARAMS ((rtx x, unsigned intsize,
						int aligned_p));

#undef TARGET_ATTRIBUTE_TABLE
#define TARGET_ATTRIBUTE_TABLE pdp10_attribute_table

#undef TARGET_ASM_OPEN_PAREN
#define TARGET_ASM_OPEN_PAREN "<"

#undef TARGET_ASM_CLOSE_PAREN
#define TARGET_ASM_CLOSE_PAREN ">"

#ifndef __PDP10_GAS_H__
#undef TARGET_ASM_BYTE_OP
#define TARGET_ASM_BYTE_OP 0

#undef TARGET_ASM_INTEGER
#define TARGET_ASM_INTEGER pdp10_assemble_integer
#endif

#undef TARGET_ASM_FUNCTION_PROLOGUE
#define TARGET_ASM_FUNCTION_PROLOGUE pdp10_output_func_prologue

#undef TARGET_ASM_FUNCTION_EPILOGUE
#define TARGET_ASM_FUNCTION_EPILOGUE pdp10_output_func_epilogue

#undef TARGET_INIT_BUILTINS
#define TARGET_INIT_BUILTINS pdp10_init_builtins

#undef TARGET_EXPAND_BUILTIN
#define TARGET_EXPAND_BUILTIN pdp10_expand_builtin

struct gcc_target targetm = TARGET_INITIALIZER;


/**********************************************************************

	Run-time Target Specifiation

**********************************************************************/

static char *normalize_cpu_name PARAMS ((char *name));
static int available_regs PARAMS ((void));

/* Receives a pointer to the value of the -mcpu option.  */
char *pdp10_cpu = 0;

/* Receives a pointer to the value of the -march option.  */
char *pdp10_arch = 0;

/* Receives a pointer to the value of the -mtune option.  */
char *pdp10_tune = 0;

/* Receives a pointer to the value of the -mtext-psect option.  */
char *pdp10_text_psect = 0;

/* Receives a pointer to the value of the -mrodata-psect option.  */
char *pdp10_rodata_psect = 0;

/* Receives a pointer to the value of the -mdata-psect option.  */
char *pdp10_data_psect = 0;

/* Receives a pointer to the value of the -mbss-psect option.  */
char *pdp10_bss_psect = 0;

/* Receives a pointer to the value of the -mmodel option.  */
char *pdp10_model = 0;

/* Receives a pointer to the value of the -mregparm option.  */
char *pdp10_argument_regs = 0;

/* Receives a pointer to the value of the -mreturn-regs option.  */
char *pdp10_return_regs = 0;

/* Receives a pointer to the value of the -mcall-clobbered option.  */
char *pdp10_call_clobbered = 0;

/* Receives a pointer to the value of the -mstring-bytesize option.  */
char *pdp10_string_bytesize = 0;

/* Receives a pointer to the value of the -mchar-bytesize option.  */
char *pdp10_char_bytesize_arg = 0;

int pdp10_char_bytesize = 9;

#ifdef __PDP10_MACRO_H__
/* A C expression whose value is a string containing the assembler
   operation that should precede instructions (and maybe read-only
   data).  */
char *pdp10_text_section_asm_op;

/* A C expression whose value is a string containing the assembler
   operation that should precede read-only data.  */
char *pdp10_rodata_section_asm_op;

/* A C expression whose value is a string containing the assembler
   operation that should precede read-write data.  */
char *pdp10_data_section_asm_op;

/* A C expression whose value is a string containing the assembler
   operation that should precede uninitialized data.  */
char *pdp10_bss_section_asm_op;
#endif

/* Which processor to tune for.  */
int target_tune = 0;

/* Which code model to use.  */
int target_model = MODEL_NONE;

/* What byte size to use for string literals.  */
int pdp10_string_literal_byte_size = 9;

/* A table to match the canonical -mcpu name with the numerical cpu
   type, and also two bit masks to enable or disable features.  */
static struct { const char *name; int cpu; int enable; int disable; }
cpu_table[] =
{
  /* name,	cpu,		enable,		disable */

  { "166",	CPU_166,	0,		CPU_EXTENDED	},
  { "ka10",	CPU_KA10,	0,		CPU_EXTENDED	},
  { "ki10",	CPU_KI10,	0,		CPU_EXTENDED	},
  { "ks10",	CPU_KS10,	0,		CPU_EXTENDED	},
  { "kl10",	CPU_KL10,	CPU_EXTENDED,	0		},
  { "sc20",	CPU_KL10,	CPU_EXTENDED,	0		},
  { "sc25",	CPU_KL10,	CPU_EXTENDED,	0		},
  { "sc30m",	CPU_KL10,	CPU_EXTENDED,	0		},
  { "sc40",	CPU_KL10,	CPU_EXTENDED,	0		},
  { "xkl1",	CPU_XKL1,	CPU_EXTENDED,	0		},
  { "xkl2",	CPU_XKL2,	CPU_EXTENDED,	0		},
};

/* A table to find the canonical cpu name.  */
static struct { const char *alias; const char *name; }
cpu_alias[] =
{
  { "pdp6",	"166"	},
  { "ka",	"ka10"	},
  { "ki",	"ki10"	},
  { "kl",	"kl10"	},
  { "ks",	"ks10"	},
  { "sc30",	"sc30m"	},
};

/* Normalize the cpu name passed in NAME.  All characters are
   converted to lower-case, and '-' characters are removed.  Finally,
   cpu aliases are resolved.  */
static char *
normalize_cpu_name (name)
     char *name;
{
  char *p, *q;
  unsigned int i;

  if (name == NULL)
    return NULL;

  for (p = q = name; *p != 0; p++)
    {
      if (*p != '-')
	*q++ = TOLOWER (*p);
    }
  *q = 0;

  for (i = 0; i < ARRAY_SIZE (cpu_alias); i++)
    {
      if (strcmp (name, cpu_alias[i].alias) == 0)
	{
	  name = (char *)cpu_alias[i].name;
	  break;
	}
    }

  return name;
}

/* Return the number of registers available for -mregparm,
   -mreturn-regs, and -mcall-clobbered.  */
static int
available_regs ()
{
  static int n = -1;
  int i;

  if (n >= 0)
    return n;

  for (i = n = 0; i < FIRST_PSEUDO_REGISTER; i++, n++)
    if ((fixed_regs[i] && n != 0)
	|| i == HARD_FRAME_POINTER_REGNUM
	|| i == PIC_SOMETHING_REGNUM
	|| i == STATIC_CHAIN_REGNUM)
      break;

  return n;
}

/* Called by the OVERRIDE_OPTIONS macro.

   This is run after all command line options has been read, but
   before actual compilation has begun.  */
void
pdp10_override_options ()
{
  unsigned int i;

#if XKL_STUFF
  if (debug_info_level != DINFO_LEVEL_NONE)
    {
      extern int flag_thread_jumps, flag_cprop_registers, flag_loop_optimize;
      extern int flag_crossjumping, flag_gcse, flag_rerun_cse_after_loop;
      extern int flag_delete_null_pointer_checks;

      pdp10_regparm = 0;
      optimize = 0;

      flag_defer_pop = 0;
      flag_thread_jumps = 0;
#ifdef DELAY_SLOTS
      flag_delayed_branch = 0;
#endif
#ifdef CAN_DEBUG_WITHOUT_FP
      flag_omit_frame_pointer = 0;
#endif
      flag_guess_branch_prob = 0;
      flag_cprop_registers = 0;
      flag_loop_optimize = 0;
      flag_crossjumping = 0;
      flag_optimize_sibling_calls = 0;
      flag_cse_follow_jumps = 0;
      flag_cse_skip_blocks = 0;
      flag_gcse = 0;
      flag_expensive_optimizations = 0;
      flag_strength_reduce = 0;
      flag_rerun_cse_after_loop = 0;
      flag_rerun_loop_opt = 0;
      flag_caller_saves = 0;
      flag_force_mem = 0;
      flag_peephole2 = 0;
#ifdef INSN_SCHEDULING
      flag_schedule_insns = 0;
      flag_schedule_insns_after_reload = 0;
#endif
      flag_regmove = 0;
      flag_strict_aliasing = 0;
      flag_delete_null_pointer_checks = 0;
      flag_reorder_blocks = 0;
      flag_inline_functions = 0;
      flag_rename_registers = 0;
    }
#endif

  /* -mcpu is short for both -march and -mtune.  */
  if (pdp10_cpu)
    {
      pdp10_arch = pdp10_cpu;
      pdp10_tune = pdp10_cpu;
    }

  pdp10_arch = normalize_cpu_name (pdp10_arch);
  pdp10_tune = normalize_cpu_name (pdp10_tune);

  if (pdp10_arch || pdp10_tune)
    {
      for (i = 0; i < ARRAY_SIZE (cpu_table); i++)
	{
	  if (pdp10_arch && strcmp (pdp10_arch, cpu_table[i].name) == 0)
	    {
	      target_flags = (target_flags & ~CPU_MASK) | cpu_table[i].cpu;
	      target_flags |= cpu_table[i].enable;
	      target_flags &= ~cpu_table[i].disable;
	    }

	  if (pdp10_tune && strcmp (pdp10_tune, cpu_table[i].name) == 0)
	    target_tune = cpu_table[i].cpu;
	}
    }

  if (pdp10_arch && TARGET_CPU == 0)
    fprintf (stderr, "Unknown CPU: %s\n", pdp10_arch);
  if (pdp10_tune && target_tune == 0)
    fprintf (stderr, "Unknown CPU: %s\n", pdp10_tune);

  /* The default processor is XKL-1.  */
  if (TARGET_CPU == 0)
    target_flags = (target_flags & ~CPU_MASK) | CPU_XKL1;
  if (target_tune == 0)
    target_tune = CPU_XKL1;

  if ((target_flags & OPT_EXTENDED)
      && (target_flags & OPT_UNEXTENDED))
    fprintf (stderr,
	     "Can't use -mextended and -munextended at the same time\n");

  if (target_flags & OPT_EXTENDED)
    target_flags |= CPU_EXTENDED;
  if (target_flags & OPT_UNEXTENDED)
    target_flags &= ~CPU_EXTENDED;

  if (pdp10_model)
    {
      if (strcmp (pdp10_model, "small") == 0)
	target_model = MODEL_SMALL;
      else if (strcmp (pdp10_model, "smallish") == 0)
	target_model = MODEL_SMALLISH;
      else if (strcmp (pdp10_model, "medium") == 0)
	target_model = MODEL_MEDIUM;
      else if (strcmp (pdp10_model, "large") == 0)
	target_model = MODEL_LARGE;
      else
	fprintf (stderr, "Unknown code model: %s\n", pdp10_model);
    }

  /* The default program model is small for unextended processors,
     smallish for XKL processors, and large for other extended
     processors.  */
  if (target_model == MODEL_NONE)
    {
      if (TARGET_EXTENDED)
	{
	  if (TARGET_XKL1 || TARGET_XKL2)
	    target_model = MODEL_SMALLISH;
	  else
	    target_model = MODEL_LARGE;
	}
      else
	target_model = MODEL_SMALL;
    }

#ifdef __PDP10_MACRO_H__
  if (TARGET_EXTENDED)
    {
      if (!pdp10_text_psect)
	pdp10_text_psect = (char *)".text";
      pdp10_text_section_asm_op = xmalloc (15 + strlen (pdp10_text_psect));
      sprintf (pdp10_text_section_asm_op, "\t.PSECT %s/ronly",
	       pdp10_text_psect);

      if (!pdp10_rodata_psect)
	pdp10_rodata_psect = (char *)".rodata";
      pdp10_rodata_section_asm_op = xmalloc (15 + strlen (pdp10_rodata_psect));
      sprintf (pdp10_rodata_section_asm_op, "\t.PSECT %s/ronly",
	       pdp10_rodata_psect);

      if (!pdp10_data_psect)
	pdp10_data_psect = (char *)".data";
      pdp10_data_section_asm_op = xmalloc (15 + strlen (pdp10_data_psect));
      sprintf (pdp10_data_section_asm_op, "\t.PSECT %s",
	       pdp10_data_psect);

      if (!pdp10_bss_psect)
	pdp10_bss_psect = (char *)".bss";
      pdp10_bss_section_asm_op = xmalloc (15 + strlen (pdp10_bss_psect));
      sprintf (pdp10_bss_section_asm_op, "\t.PSECT %s",
	       pdp10_bss_psect);
    }
  else
    {
      pdp10_text_section_asm_op = (char *)"";
      pdp10_rodata_section_asm_op = (char *)"";
      pdp10_data_section_asm_op = (char *)"";
      pdp10_bss_section_asm_op = (char *)"";
    }
#endif

  if (pdp10_argument_regs)
    {
      int i;

      pdp10_regparm = atoi (pdp10_argument_regs);
      if (pdp10_regparm < 0 || pdp10_regparm > available_regs () - 1)
	{
	  fprintf (stderr, "Bad value for -mregparm: %d\n", pdp10_regparm);
	  exit (1);
	}

      for (i = 0; i < pdp10_regparm; i++)
	reg_alloc_order[i] = pdp10_regparm - i;
    }

  if (pdp10_return_regs)
    {
      pdp10_retregs = atoi (pdp10_return_regs);
      if (pdp10_retregs < 1 || pdp10_retregs > available_regs () - 1)
	{
	  fprintf (stderr, "Bad value for -mreturn-regs: %d\n",
		   pdp10_retregs);
	  exit (1);
	}
    }

  if (pdp10_call_clobbered)
    {
      int n = atoi (pdp10_call_clobbered);

      if (n < 0 || n > available_regs ())
	{
	  fprintf (stderr, "Bad value for -mcall-clobbered: %d\n", n);
	  exit (1);
	}
      else if (n <= pdp10_retregs)
	{
	  fprintf (stderr, "-mcall-clobbered=%d clash with -mreturn-regs=%d\n",
		   n, pdp10_retregs);
	  exit (1);
	}

      for (i = 0; i < (unsigned) n; i++)
	call_used_regs[i] = 1;
      for (; i < FIRST_PSEUDO_REGISTER; i++)
	call_used_regs[i] = fixed_regs[i];
    }

  if (pdp10_string_bytesize)
    {
      int n = atoi (pdp10_string_bytesize);

      if (n != 6 && n != 7 && n != 8 && n != 9)
	{
	  fprintf (stderr, "Bad value for -mstring-bytesize: %s\n",
		   pdp10_string_bytesize);
	  exit (1);
	}

      pdp10_string_literal_byte_size = n;
    }

  if (pdp10_char_bytesize_arg)
    {
      int n = atoi (pdp10_char_bytesize_arg);

      if (n != 6 && n != 7 && n != 8 && n != 9)
	{
	  fprintf (stderr, "Bad value for -mchar-bytesize: %s\n",
		   pdp10_char_bytesize_arg);
	  exit (1);
	}

      pdp10_char_bytesize = n;
    }

  ggc_add_rtx_root (&pdp10_compare_op0, 1);
  ggc_add_rtx_root (&pdp10_compare_op1, 1);
}


/**********************************************************************

	Storage Layout

**********************************************************************/

static int ps_base_for_bytesize PARAMS ((int size));

/* Called by the ALIGN_STACK_POINTER macro.  This generates code to
   align the stack pointer.  */
rtx
pdp10_align_stack_pointer (x, align)
     rtx x;
     unsigned align;
{
  if (align % UNITS_PER_WORD != 0)
    abort ();

  if (!TARGET_EXTENDED)
    abort ();

  align /= UNITS_PER_WORD;
  if (align > 1)
    {
      x = expand_binop (Pmode, add_optab, x, GEN_INT (align - 1), NULL_RTX,
			1, OPTAB_LIB_WIDEN);
      x = expand_divmod (0, TRUNC_DIV_EXPR, Pmode, x, GEN_INT (align),
			 NULL_RTX, 1);
      x = expand_mult (Pmode, x, GEN_INT (2), NULL_RTX, 1);
    }

  return x;
}

/* Called by the DATA_ALIGNMENT macro.

   Computes the alignment for a variable in the static store.  TYPE is
   the data type, and ALIGN is the alignment that the object would
   ordinarily have.  The return value of this function is used instead
   of that alignment to align the object.  */
int
pdp10_data_alignment (align, type)
     int align;
     tree type ATTRIBUTE_UNUSED;
{
  /* All static data items are word-aligned.  */
  if (align < BITS_PER_WORD)
    align = BITS_PER_WORD;
  return align;
}

/* Called by the DATA_PADDING macro.  */
rtx
pdp10_data_padding (decl, x)
     tree decl;
     rtx x;
{
  int bytesize;
  HOST_WIDE_INT offset = 0;
  HOST_WIDE_INT ps;
  int o = 0;

  if (TREE_TYPE (decl) == error_mark_node)
    return x;

  bytesize = pdp10_bytesize (TREE_TYPE (decl));
  if (bytesize >= 32)
    return x;

  if (GET_CODE (x) == CONST)
    {
      offset = INTVAL (XEXP (XEXP (x, 0), 1));
      offset &= ADDRESS_MASK;
      x = XEXP (XEXP (x, 0), 0);
    }

  if (TREE_CODE (decl) == VAR_DECL
      && !AGGREGATE_TYPE_P (TREE_TYPE (decl)))
    {
      /* Scalar char variables of all sizes are referred to as the
	 right half of a word.  */
      if (bytesize <= 9)
	bytesize = 18;
      o = BITS_PER_WORD / bytesize - 1;
    }

  if (TARGET_EXTENDED)
    {
      ps = ps_base_for_bytesize (bytesize) + o;
      ps <<= 6;
    }
  else
    ps = ((36 - bytesize * o - bytesize) << 6) + bytesize;

  return plus_constant (x, trunc_int_for_mode (offset + (ps << 24), Pmode));
}

/* Called by the CONSTANT_ALIGNMENT macro.

   Computes the alignment given to a constant that is being placed in
   memory.  EXP is the constant and ALIGN is the alignment that the
   object would ordinarily have.  The return value of this function is
   used instead of that alignment to align the object.  */
int
pdp10_constant_alignment (align, exp)
     int align;
     tree exp;
{
  /* String constants are placed at the start of a word.  */
  if (TREE_CODE (exp) == STRING_CST && align < BITS_PER_WORD)
    align = BITS_PER_WORD;
  return align;
}

/* Called by the LOCAL_ALIGNMENT macro.

   Computes the alignment for a variable in the local store.  TYPE is
   the data type, and ALIGN is the alignment that the object would
   ordinarily have.  The return value of this function is used instead
   of that alignment to align the object.  */
int
pdp10_local_alignment (align, type)
     int align;
     tree type ATTRIBUTE_UNUSED;
{
  if (align < BITS_PER_WORD)
    align = BITS_PER_WORD;
  return align;
}

/* Called by the LOCAL_SIZE macro.

   Computes the size of a variable in the local store.  TYPE is the
   data type, and SIZE is the size that the object would ordinarily
   have.  */
int
pdp10_local_size (size, type)
     int size;
     tree type ATTRIBUTE_UNUSED;
{
  if (size < UNITS_PER_WORD)
    size = UNITS_PER_WORD;
  return size;
}


/**********************************************************************

	Register Usage

**********************************************************************/

void
pdp10_conditional_register_usage ()
{
}


/**********************************************************************

	Register Classes

**********************************************************************/

/* Called by the CONST_OK_FOR_LETTER_P macro.

   Defines the machine-specific operand constraint letters that
   specify particular ranges of integer values.  */
int
pdp10_const_ok_for_letter_p (value, c)
     HOST_WIDE_INT value;
     int c;
{
  switch (c)
    {
    case 'I': return value >= 0 && value <= 0777777;
    case 'J': return value <= 0 && value >= -0777777;
    case 'K': return value >= -0400000 && value <= 0377777;
    case 'L': return 
		(value & 0777777) == 0
		&& pdp10_const_ok_for_letter_p ((value >> 18) & 0777777, 'I');
    case 'M': return value == -1;
    case 'N': return 
		(value & 0777777) == 0777777
		&& pdp10_const_ok_for_letter_p ((value >> 18) & 0777777, 'I');
    case 'O': return value == 0;
    case 'P': return pdp10_const_ok_for_letter_p (~value, 'I');
    }
  return 0;
}

/* Called by the CONST_DOUBLE_OK_FOR_LETTER_P macro.

   Defines the machine-dependent operand constraint letters that
   specify particular ranges of CONST_DOUBLE values.  */
int
pdp10_const_double_ok_for_letter_p (x, c)
     rtx x;
     int c;
{
  REAL_VALUE_TYPE r;
  long y[3];

  if (c != 'G')
    return 0;

  REAL_VALUE_FROM_CONST_DOUBLE (r, x);
  REAL_VALUE_TO_TARGET_DOUBLE (r, y);

  switch (GET_MODE (x))
    {
    case VOIDmode:
      return 0;
    case SFmode:
      return
	(y[0] & 0x00003fffUL) == 0
	&& (y[1] & 0xf0000000UL) == 0;
    case DFmode:
      return
	(y[0] & 0x00003fffUL) == 0
	&& (y[1] & 0xffffffffUL) == 0
	&& (y[2] & 0xff000000UL) == 0;
    default:
      abort ();
    }
}

/* Called by the EXTRA_CONSTRAINT macro.

   Defines the optional machine-dependent constraint letters that can
   be used to segregate specific types of operands for the target
   machine.  */
int
pdp10_extra_constraint (x, c)
     rtx x;
     int c;
{
  switch (c)
    {
    case 'Q':
      return GET_CODE (x) == CONST_INT && INTVAL (x) == 1;
    case 'R':
      return pdp10_const_double_0_operand (x, GET_MODE (x));
    case 'S':
      {
	HOST_WIDE_INT offset = 0;
	enum rtx_code code;
	int ps = 0;

	if ((TARGET_EXTENDED && !TARGET_SMALLISH)
	    || ((code = GET_CODE (x)) != SYMBOL_REF
		&& code != LABEL_REF
		&& code != CONST))
	  return 0;

	if (code == CONST)
	  {
	    x = XEXP (x, 0);
	    offset = INTVAL (XEXP (x, 1));
	    x = XEXP (x, 0);
	    if ((offset & ~ADDRESS_MASK)
		!= (((HOST_WIDE_INT)-1) & ~ADDRESS_MASK))
	      ps = (offset >> 24) & 07700;
	    offset &= ADDRESS_MASK;
	  }
	return ps == 0;
      }
    }
  return 0;
}


/**********************************************************************

	Stack Layout and Calling Conventions

**********************************************************************/

static int	clobbered_regs PARAMS ((int *clobbered));

/* Called by the STARTING_FRAME_OFFSET macro.  */
int
pdp10_starting_frame_offset ()
{
  return 0;
}

/* Called by the STACK_POINTER_OFFSET macro.  */
int
pdp10_stack_pointer_offset ()
{
  return 1;
}

/* Called by the FIRST_PARM_OFFSET macro.  */
int
pdp10_first_parm_offset ()
{
  tree attr = DECL_ATTRIBUTES (current_function_decl);

  if (attr != NULL_TREE && lookup_attribute ("noreturn", attr))
    return 0;

  return -1;
}

/* Called by the STACK_DYNAMIC_OFFSET macro.  */
int
pdp10_stack_dynamic_offset ()
{
  return 1;
}

/* Called by the INITIAL_ELIMINATION_OFFSET macro.  */
int
pdp10_initial_elimination_offset (from, to)
     int from;
     int to;
{
  if (from == ARG_POINTER_REGNUM || from == HARD_FRAME_POINTER_REGNUM)
    {
      int clobbered[FIRST_PSEUDO_REGISTER];
      int clobbered_size = clobbered_regs (clobbered);
      if (to == STACK_POINTER_REGNUM)
	return -((get_frame_size () + current_function_outgoing_args_size
		  + clobbered_size - UNITS_PER_WORD) / UNITS_PER_WORD);
      else if (to == HARD_FRAME_POINTER_REGNUM)
	return 0;
    }
  else if (from == FRAME_POINTER_REGNUM)
    {
      if (to == STACK_POINTER_REGNUM)
	return -((get_frame_size () + current_function_outgoing_args_size
		  - UNITS_PER_WORD) / UNITS_PER_WORD);
      else if (to == HARD_FRAME_POINTER_REGNUM)
	{
	  int clobbered[FIRST_PSEUDO_REGISTER];
	  int clobbered_size = clobbered_regs (clobbered);
	  return clobbered_size / UNITS_PER_WORD;
	}
    }

  abort ();
}

/* Stack layout, before function prologue:

                ...
         [     arg 6    ]
         [     arg 5    ]
   sp -> [return address]

   Stack layout, after fixed-arg function prologue:

                ...
         [     arg 6    ]
         [     arg 5    ]
	 [return address]
   fp -> [ saved reg 16 ]
         [ saved reg  5 ]
         [ saved reg  6 ]
         [ saved reg  7 ]
         [ saved reg 10 ]
         [ saved reg 11 ]   Registers 5 - 16 are saved if clobbered by
         [ saved reg 12 ]   the function.
         [ saved reg 13 ]
         [ saved reg 14 ]
         [ saved reg 15 ]
         [ frame slot 1 ]
         [ frame slot 2 ]   Local variables.
         [ frame slot 3 ]
               ...
         [outgoing arg 3]
         [outgoing arg 2]   Reserved space for outgoing arguments.
   sp -> [outgoing arg 1]

   Stack layout, after vararg function prologue:

                ...
         [     arg 6    ]
         [     arg 5    ]
         [     arg 4    ]   Args 1-4 has been pushed to stack.
         [     arg 3    ]
         [     arg 2    ]
         [     arg 1    ]
	 [return address]   Return address has been moved.
   fp -> [ saved reg 16 ]
         [ saved reg  5 ]
         [ saved reg  6 ]
         [ saved reg  7 ]
         [ saved reg 10 ]
         [ saved reg 11 ]   Registers 5 - 16 are saved if clobbered by
         [ saved reg 12 ]   the function.
         [ saved reg 13 ]
         [ saved reg 14 ]
         [ saved reg 15 ]
         [ frame slot 1 ]
         [ frame slot 2 ]   Local variables.
         [ frame slot 3 ]
               ...
         [outgoing arg 3]
         [outgoing arg 2]   Reserved space for outgoing arguments.
   sp -> [outgoing arg 1]

   Stack layout, after vararg function epilogue, but before return:

                ...
         [     arg 6    ]
         [     arg 5    ]
   sp -> [return address]   Return address has been moved back again.
*/

/* Called by the FUNCTION_PROLOGUE macro.  */
void
pdp10_output_func_prologue (stream, frame_size)
     FILE *stream ATTRIBUTE_UNUSED;
     HOST_WIDE_INT frame_size ATTRIBUTE_UNUSED;
{
}

/* Called by the FUNCTION_EPILOGUE macro.  */
void
pdp10_output_func_epilogue (f, frame_size)
     FILE *f;
     HOST_WIDE_INT frame_size ATTRIBUTE_UNUSED;
{
  fputc ('\n', f);
}

/* Called by the FUNCTION_PROFILER macro.  */
void
pdp10_function_profiler (file, labelno)
     FILE *file ATTRIBUTE_UNUSED;
     int labelno ATTRIBUTE_UNUSED;
{
}

/* Called by the SETUP_INCOMING_VARARGS macro.  */
int
pdp10_setup_incoming_varargs (cum, pretend_size)
     CUMULATIVE_ARGS cum ATTRIBUTE_UNUSED;
     int pretend_size;
{
  return pretend_size;
}


/**********************************************************************

	Addressing Modes

**********************************************************************/

static int	index_register PARAMS ((rtx x, int strict));

/* Return nonzero if X is a valid index register, zero otherwise.  If
   STRICT is nonzero, all pseudo registers qualify.  */
static int
index_register (x, strict)
     rtx x;
     int strict;
{
  if (GET_CODE (x) == SUBREG)
    x = SUBREG_REG (x);
  if (!REG_P (x))
    return 0;
  if (REGNO (x) < FIRST_PSEUDO_REGISTER)
    return REGNO_REG_CLASS (REGNO (x)) == INDEX_REGS;
  else
    return !strict;
}

/* Called by the GO_IF_LEGITIMATE_ADDRESS macro.  Returns nonzero if X
   is a valid address for accessing something of mode MODE.  */
int
pdp10_legitimate_address_p (mode, x, strict)
     enum machine_mode mode;
     rtx x;
     int strict;
{
  /* A constant address is always ok.  */
  if (CONSTANT_ADDRESS_P (x))
    return 1;

  switch (GET_CODE (x))
    {
      /* This seems to be needed sometimes.  */
    case SUBREG:
      x = SUBREG_REG (x);
      if (GET_CODE (x) != REG)
	return 0;
      /* Fall through.  */

      /* A register without an offset is always a valid address.
	 Register 0 can be used with indirect addressing.  */
    case REG:
      return !strict || REGNO (x) < FIRST_PSEUDO_REGISTER;

      /* A sum must be an index register plus a valid constant offset.  */
    case PLUS:
      if (!index_register (XEXP (x, 0), strict))
	return 0;

      /* In general, indexing can't be used with byte pointers.  Only
	 allow it with the stack pointer as the index register.  */
      if (BYTE_MODE_P (mode) && !REGNO_PTR_FRAME_P (REGNO (XEXP (x, 0))))
	return 0;

      x = XEXP (x, 1);
      switch (GET_CODE (x))
	{
	case CONST_INT:
	  /* Checking no_new_pseudos ensures that a global format
	     indirect word is not valid when generating RTL, but it's
	     valid when the instruction combination pass is running.  */
	  if (no_new_pseudos)
	    /* Global format indirect word.  */
	    return 1;
	  else
	    {
	      if (TARGET_EXTENDED)
		return INTVAL (x) >= -0400000 && INTVAL (x) < 0400000;
	      else
		return INTVAL (x) > -01000000 && INTVAL (x) < 01000000;
	    }
	case CONST:
	case LABEL_REF:
	case SYMBOL_REF:
	  return !TARGET_EXTENDED || TARGET_SMALLISH;
	default:
	  return 0;
	}

#if 0
      /* Pre-incrementing works with byte pointers.  */
    case PRE_INC:
      x = XEXP (x, 0);
      return BYTE_MODE_P (mode) && REG_P (x);
#endif

      /* Indirect addressing.  */
    case MEM:
      if (BYTE_MODE_P (mode))
	return pdp10_legitimate_address_p (Pmode, XEXP (x, 0), strict);

      x = XEXP (x, 0);
      if (REG_P (x))
	return index_register (x, strict);
      else if (GET_CODE (x) == PLUS)
	return
	  index_register (XEXP (x, 0), strict)
	  && (((!TARGET_EXTENDED || TARGET_SMALLISH)
	       && CONSTANT_ADDRESS_P (XEXP (x, 1)))
	      || (GET_CODE (XEXP (x, 1)) == CONST_INT
		  && pdp10_const_ok_for_letter_p (INTVAL (XEXP (x, 1)), 'K')));
      /* Fall through.  */

    default:
      return 0;
    }
}

/* Called by the LEGITIMIZE_ADDRESS macro.

   Attempts to replace X with a valid memory address for an operand of
   mode MODE.  Returns NULL_RTX if no valid memory address was found.  */
rtx
pdp10_legitimize_address (x, oldx, mode)
     rtx x;
     rtx oldx ATTRIBUTE_UNUSED;
     enum machine_mode mode;
{
  HOST_WIDE_INT max_offset = TARGET_EXTENDED ? 0377777 : 0777777;
  HOST_WIDE_INT min_offset = TARGET_EXTENDED ? -0400000 : -0777777;
  rtx x0, x1;

  if (mode == BLKmode)
    return NULL_RTX;

  if (GET_CODE (x) != PLUS)
    return NULL_RTX;

  x0 = XEXP (x, 0);
  x1 = XEXP (x, 1);

  if (REG_P (x0)
      && GET_CODE (x1) == CONST_INT
      && (INTVAL (x1) < min_offset || INTVAL (x1) > max_offset))
    {
      HOST_WIDE_INT offset = INTVAL (x1);
      HOST_WIDE_INT mask = 0777777;
      HOST_WIDE_INT sign = 0400000;
      rtx high_reg = gen_reg_rtx (Pmode);
      rtx base_reg = gen_reg_rtx (Pmode);
      rtx high_part;

      if (!TARGET_EXTENDED)
	error ("address too large for unextended mode");
      else if (offset <= -(HWINT (1) << 30)
	       || offset >= (HWINT (1) << 30))
	error ("address too large for extended mode");

      high_part = gen_int_mode ((offset + sign) & ~mask, Pmode);
      /*emit_insn (gen_rtx_SET (VOIDmode, high_reg, gen_ADDRESS (high_part)));*/
      emit_move_insn (high_reg, high_part);
      emit_insn (gen_addsi3 (base_reg, x0, high_reg));
      return plus_constant (base_reg,
			    trunc_int_for_mode (((offset & mask) ^ sign) - sign,
						Pmode));
    }

  return NULL_RTX;
}


/**********************************************************************

	Dividing the Output into Sections

**********************************************************************/

/* Called by the ENCODE_SECTION_INFO macro.

   DECL is a declaration.  If it's a function definition, note that
   the function is defined in the current translation unit.  Also
   record the type of all symbols.  */
void
pdp10_encode_section_info (decl)
     tree decl;
{
  if (TREE_CODE (decl) == FUNCTION_DECL)
    {
      /* Record whether the function is known to be defined in this
         unit or not.  */
      if (!TREE_PUBLIC (decl) && !DECL_WEAK (decl))
	SYMBOL_REF_INTERNAL (XEXP (DECL_RTL (decl), 0)) = 1;
    }
}


/**********************************************************************

	Describing Relative Costs of Operations

**********************************************************************/

/* Called by the CONST_COSTS macro.

   Describes the relative consts of CONST_INT expressions.  */
int
pdp10_const_int_costs (value, outer_code)
     HOST_WIDE_INT value;
     enum rtx_code outer_code;
{
  char c = 'I';

  switch (outer_code)
    {
      /* Shift instructions take a signed 18-bit value.  */
    case ASHIFT:
    case ASHIFTRT:
    case LSHIFTRT:
    case ROTATE:
    case ROTATERT:
      c = 'K';
      break;

      /* TLZ and TRZ take 18-bit one's complemented values.  AND takes
	 an 18-bit value.  */
    case AND:
      if (CONST_OK_FOR_LETTER_P (value, 'N')	/* TLZ */
	  || CONST_OK_FOR_LETTER_P (value, 'P'))	/* TRZ */
	return 0;
      break;

      /* TLO and TLC take an unsigned 18-bit value in the upper 18
	 bits.  IOR and XOR take an unsigned 18-bit value.  */
    case IOR:
    case XOR:
      if (CONST_OK_FOR_LETTER_P (value, 'L'))	/* TLO and TLC */
	return 0;
      break;

    case SET:
      if (CONST_OK_FOR_LETTER_P (value, 'L')	/* MOVS */
	  || CONST_OK_FOR_LETTER_P (value, 'N')	/* HRLO */
	  || CONST_OK_FOR_LETTER_P (value, 'P'))	/* HRRO */
	return 0;
      /* Fall through.  */

    case PLUS:
      if (CONST_OK_FOR_LETTER_P (value, 'J'))	/* SUBI and MOVN */
	return 0;
      /* ADDI and MOVEI take an unsigned 18-bit value.  */
      break;

    case NEG:
      /* Changing this may affect right-shifts.  */
      return 0;

    default:
      break;
    }

  return CONST_OK_FOR_LETTER_P (value, c) ? 0 : COSTS_N_INSNS (1) / 4;
}

/* Called by the CONST_COSTS macro.

   Describes the relative consts of CONST_DOUBLE expressions.  */
int
pdp10_const_double_costs (x, code, outer_code)
     rtx x;
     enum rtx_code code ATTRIBUTE_UNUSED;
     enum rtx_code outer_code ATTRIBUTE_UNUSED;
{
  /* A floating-point immediate costs nothing.  */
  if (pdp10_const_double_ok_for_letter_p (x, 'G'))
    return 0;

  return COSTS_N_INSNS (1);
}

/* Called by the RTX_COSTS macro.

   Describes the relative consts of nonconstant expressions.  */
int
pdp10_operand_cost (x, outer_code)
     rtx x;
     enum rtx_code outer_code;
{
  switch (GET_CODE (x))
    {
    CONST_COSTS (x, GET_CODE (x), outer_code)

    case REG:
    case PLUS:
      return COSTS_N_INSNS (1);

    case MEM:
      return
      (GET_MODE (x) == SImode ?
       COSTS_N_INSNS (1) / 4 : COSTS_N_INSNS (1) / 2)
      + ADDRESS_COST (XEXP (x, 0));

    case NOT:
    case NEG:
    case USE:
    case SUBREG:
    case LSHIFTRT:
    case ZERO_EXTRACT:
    case SIGN_EXTRACT:
      return pdp10_operand_cost (XEXP (x, 0), outer_code);

      /* Not a valid operand.  */
    case MINUS:
      return COSTS_N_INSNS (1000);

    case ADDRESSOF:
      /* Dunno.  Try this.  */
      return COSTS_N_INSNS (1);

    case AND:
    case MULT:
      return (pdp10_operand_cost (XEXP (x, 0), outer_code)
	      + pdp10_operand_cost (XEXP (x, 0), outer_code));

    case UNSPEC:
      switch (XINT (x, 1))
	{
	case UNSPEC_ADJBP:
	  return COSTS_N_INSNS (100)
	    + pdp10_operand_cost (XVECEXP (x, 0, 0), outer_code);
	case UNSPEC_ADDRESS:
	  return COSTS_N_INSNS (1);
	default:
	  return COSTS_N_INSNS (1);
	}

      /* For TSC.  */
    case IOR:
    case XOR:
    case ASHIFT:
    case ASHIFTRT:
      return COSTS_N_INSNS (1);

    default:
      debug_rtx (x);
      abort ();
    }
}

/* Called by the ADDRESS_COST macro.

   Gives the cost of an addressing mode that contains X.  */
int
pdp10_address_cost (x)
     rtx x;
{
  if (GET_CODE (x) == ADDRESSOF)
    return 0;
  else if (CONSTANT_ADDRESS_P (x))
    return
      TARGET_EXTENDED
      && !TARGET_SMALLISH
      && GET_CODE (x) != LABEL_REF
      && !(GET_CODE (x) == SYMBOL_REF && SYMBOL_REF_INTERNAL (x))
      ? COSTS_N_INSNS (1) / 2 : COSTS_N_INSNS (1) / 4;
  else if (REG_P (x))
    return COSTS_N_INSNS (1) / 2;
  else if (GET_CODE (x) == PLUS)
    return COSTS_N_INSNS (1) / 2;
  else if (GET_CODE (x) == PRE_INC)
    return REG_P (XEXP (x, 0)) ?
      COSTS_N_INSNS (1) : COSTS_N_INSNS (2);
  else if (GET_CODE (x) == MEM)
    {
      x = XEXP (x, 0);
      if (REG_P (x))
	return COSTS_N_INSNS (2);
      else if (GET_CODE (x) == PLUS)
	return COSTS_N_INSNS (2);
      return COSTS_N_INSNS (3);
    }
  else if (GET_CODE (x) == SUBREG && REG_P (SUBREG_REG (x)))
    return COSTS_N_INSNS (1) / 2;
  else
    return MAX_COST;

  /* Not a valid address.  */
  debug_rtx (x);
  abort ();
  return -1;
}


/**********************************************************************

	Defining the Output Assembler Language

**********************************************************************/

static const char *
		condition_string PARAMS ((enum rtx_code condition));
static void	print_reg PARAMS ((FILE *stream, int regno));
static void	print_byte_pointer PARAMS ((FILE *stream, rtx x,
					    int size, int pos));
static void	print_word_pointer PARAMS ((FILE *stream, rtx x, int));
static int	decode_ps PARAMS ((int ps, int *p, int *s));
/*static*/ int	adjust_ps PARAMS ((int ps, int *adjust));
static void	maybe_remove_label_and_following_code PARAMS ((rtx label));
static rtx	follow_label PARAMS ((rtx label));

/* Called by the ASM_DECLARE_OBJECT_NAME macro.

   Outputs to the stdio stream STREAM any text necessary for declaring
   the name NAME of an initialized variable which is being defined.  */
void
pdp10_asm_declare_object_name (stream, name, decl)
     FILE *stream;
     const char *name;
     tree decl;
{
  int byte_size;

#ifndef __PDP10_GAS_H__
  pdp10_extern_symbol (name, PDP10_SYMBOL_DEFINED);
#endif

  ASM_OUTPUT_LABEL (stream, name);

  /* If this is a scalar integer variable smaller than a word, justify
     it to the right-most addressable position inside a word.  */
  if (TREE_CODE (decl) == VAR_DECL
      && TREE_CODE (TREE_TYPE (decl)) == INTEGER_TYPE
      && (byte_size = TYPE_PRECISION (TREE_TYPE (decl))) < 36)
    {
      int pad = 36 - byte_size - 36 % byte_size;
      pdp10_output_byte (stream, const0_rtx, pad);
    }
}

/* Called by the ASM_OUTPUT_LABELREF macro.

   Outputs to the stdio stream FILE a reference in assembler syntax to
   a label named NAME.  */
void
pdp10_asm_output_labelref (file, name)
     FILE *file;
     const char *name;
{
  int len;
  char *label;
  int i;

#ifndef __PDP10_GAS_H__
  pdp10_output_byte (file, 0, 0);
#endif

  if (*name == '*')
    {
      fputs (name + 1, file);
      return;
    }

  /* If not using GAS, translate all '_' characters into '.'.  */
#ifdef __PDP10_GAS_H__
  label = name;
#else
  len = strlen (name);
  label = alloca (len + 1);
  strcpy (label, name);
  for (i = 0; i < len; i++)
    {
      if (label[i] == '_')
	label[i] = '.';
    }
#endif

  asm_fprintf (file, "%U%s", label);
}

/* Called by the ASM_OUTPUT_SYMBOL_REF macro.

   Outputs a reference to SYMBOL_REF SYM.  */
void
pdp10_asm_output_symbol_ref (stream, sym)
     FILE *stream;
     rtx sym;
{
  const char *name = XSTR (sym, 0);
  int len;
  char *label;
  int i;

  if (*name == '*')
    {
      fputs (name + 1, stream);
      return;
    }

  {
    tree id = maybe_get_identifier (name);
    if (id)
      TREE_SYMBOL_REFERENCED (id) = 1;
  }

#ifndef __PDP10_GAS_H__
  pdp10_extern_symbol (name, PDP10_SYMBOL_USED);
#endif

#ifdef __PDP10_GAS_H__
  label = name;
#else
  len = strlen (name);
  label = alloca (len + 1);
  strcpy (label, name);
  for (i = 0; i < len; i++)
    {
      if (label[i] == '_')
	label[i] = '.';
    }
#endif

  asm_fprintf (stream, "%U%s", label);
}

/* Indentation level for the instruction being output.  Indentation is
   enabled with the -mindent-skipped option.  */
int pdp10_opcode_indentation = 0;

/* Print instruction opcode name INSN on STREAM.  */
const char *
pdp10_print_insn (stream, insn)
     FILE *stream;
     const char *insn;
{
  static const char spaces[] = "          ";
  const int max_spaces = sizeof spaces - 1;
  static rtx last_output_insn = NULL_RTX;

  if (target_flags & ASM_INDENT_SKIPPED)
    {
      if (pdp10_opcode_indentation > max_spaces)
	pdp10_opcode_indentation = max_spaces;
      fputs (spaces + max_spaces - pdp10_opcode_indentation, stream);
    
      if (get_attr_skip (current_output_insn) == SKIP_YES
	  && current_output_insn != last_output_insn)
	pdp10_opcode_indentation++;
      else
	pdp10_opcode_indentation = 0;
      last_output_insn = current_output_insn;
    }

  while (*insn != ' ' && *insn != 0 && *insn != '%')
    {
      int ch = *insn++;
      if (target_flags & ASM_UPPER)
	ch = TOUPPER (ch);
      fputc (ch, stream);
    }
  return insn;
}

/* Return a string for a condition, as used in many conditional PDP-10
   instructions.  */
static const char *
condition_string (condition)
     enum rtx_code condition;
{
  static const char *upper[] = { "", "L", "E", "LE", "A", "GE", "N", "G" };
  static const char *lower[] = { "", "l", "e", "le", "a", "ge", "n", "g" };
  const char **string = target_flags & ASM_UPPER ? upper : lower;

  switch (condition)
    {
   /* never: return string[0]; */
    case LT: return string[1];
    case EQ: return string[2];
    case LE: return string[3];
  /* always: return string[4]; */
    case GE: return string[5];
    case NE: return string[6];
    case GT: return string[7];
    default: abort ();
    }
}

/* Decode PS, the 12-bit P and S field from a byte pointer, and store
   the resulting byte position in *P and byte size in *S.  Return 0 if
   PS is a legal byte, or else -1.  */
static int
decode_ps (ps, pp, sp)
     int ps;
     int *pp;
     int *sp;
{
  int p, s;

  if (ps >= 04600)
    {
      int base;

      if (ps & 077)
	return -1;

      ps = ps >> 6;
      switch (ps)
	{
	case 046: case 047: case 050: case 051:	case 052: case 053:
	  base = 046;
	  s = 6;
	  break;
	case 062: case 063: case 064: case 065: case 066:
	  base = 062;
	  s = 7;
	  break;
	case 055: case 056: case 057: case 060:
	  base = 055;
	  s = 8;
	  break;
	case 070: case 071: case 072: case 073:
	  base = 070;
	  s = 9;
	  break;
	case 075: case 076:
	  base = 075;
	  s = 18;
	  break;
	default:
	  return -1;
	}

      p = s * (ps - base);
    }
  else
    {
      s = ps & 077;
      p = 36 - s - (ps >> 6);
    }

  *sp = s;
  *pp = p;
  return 0;
}

static int
ps_base_for_bytesize (size)
     int size;
{
  switch (size)
    {
    case 6:
      return 046;
    case 7:
      return 062;
    case 8:
      return 055;
    case 9:
      return 070;
    case 16:
    case 18:
      return 075;
    case 32:
    case 36:
      return 0;
    default:
      abort ();
    }

#if 0
  switch (size)
    {
    case 6:
      return 03606;
    case 7:
      return 03507;
    case 8:
      return 03410;
    case 9:
      return 03311;
    case 16:
    case 18:
      return 02222;
    case 32:
    case 36:
      return 0;
    default:
      abort ();
    }
#endif
}

int
bytesize_for_ps (ps)
     int ps;
{
  switch (ps)
    {
    case 0:
      return 36;
    case 045:
    case 046:
    case 047:
    case 050:
    case 051:
    case 052:
    case 053:
      return 6;
    case 054:
    case 055:
    case 056:
    case 057:
    case 060:
      return 8;
    case 061:
    case 062:
    case 063:
    case 064:
    case 065:
    case 066:
      return 7;
    case 067:
    case 070:
    case 071:
    case 072:
    case 073:
      return 9;
    case 074:
    case 075:
    case 076:
      return 18;
    default:
      abort ();
    }
}

/* Decrease the reference count on LABEL, and if the count becomes
   zero, remove LABEL and the following instructions.

   Note that this is different from pdp10_remove_unnecessary_label,
   which only removes the label.  */
static void
maybe_remove_label_and_following_code (label)
     rtx label;
{
  rtx insn;

  LABEL_NUSES (label)--;
  if (LABEL_NUSES (label) > 0)
    return;

  INSN_DELETED_P (label) = 1;

#if 0
  insn = prev_active_insn (label);
  if (get_attr_skip (insn) == SKIP_YES
      /* TODO: is any_condjump_p enough? */
      || any_condjump_p (insn))
    return;
#else
  for (insn = PREV_INSN (label); insn != NULL_RTX; insn = PREV_INSN (insn))
    {
      if (!INSN_DELETED_P (insn) && active_insn_p (insn))
	return;
      if (GET_CODE (insn) == BARRIER)
	break;
    }
#endif

  for (insn = NEXT_INSN (label); GET_CODE (insn) != BARRIER;
       insn = NEXT_INSN (insn))
    if (GET_CODE (insn) != NOTE)
      INSN_DELETED_P (insn) = 1;
}

static rtx
follow_label (label)
     rtx label;
{
  rtx insn = next_active_insn (label);

  if (optimize >= 2
      && (simplejump_p (insn)
	  || (GET_CODE (insn) == CALL_INSN
	      && pdp10_output_call_as_jrst_p (insn))))
    {
      rtx set = pc_set (insn);

      if (set && (GET_CODE (SET_SRC (set)) != LABEL_REF
		  || XEXP (SET_SRC (set), 0) != label))
	maybe_remove_label_and_following_code (label);

      if (GET_CODE (insn) == JUMP_INSN)
	label = SET_SRC (PATTERN (insn));
      else
	{
	  rtx x = PATTERN (insn);
	  if (GET_CODE (x) == SET)
	    x = SET_SRC (x);
	  if (GET_CODE (x) != CALL)
	    abort ();
	  label = XEXP (x, 0);
	  if (GET_CODE (label) != MEM)
	    abort ();
	  label = XEXP (label, 0);
	}
    }

  return label;
}

/* Called by the PRINT_OPERAND macro.

   Output to the stdio stream STREAM the assembler syntax for an
   instruction operand X.  */
void
pdp10_print_operand (stream, x, code)
     FILE *stream;
     rtx x;
     int code;
{
  int flags = code & ~0xff;
  code &= 0xff;

  switch (code)
    {
      /* Optionally print a space.  */
    case '_':
      if (target_flags & ASM_INDENT_SKIPPED)
	fputc (' ', stream);
      return;

      /* Optionally print AC0.  */
    case '@':
      if (target_flags & ASM_AC0)
	fputs ("0,", stream);
      return;

      /* Start a comment.  */
    case ';':
      fputc ('\t', stream);
      fputs (ASM_COMMENT_START, stream);
      return;

      /* Print first, second, or both words of a double-word constant.  */
    case 'A':
    case 'B':
    case 'D':
      if (GET_CODE (x) != CONST_INT)
	break;
      {
	HOST_WIDE_INT i = INTVAL (x);

#ifdef __PDP10_GAS_H__
	fputs (".long ", stream);
#else
	if (code == 'D')
	  fputs ("EXP ", stream);
#endif
	if (code != 'B')
	  pdp10_print_unsigned (stream, (i >> 36) & WORD_MASK);
	if (code == 'D')
	  fputc (',', stream);
	if (code != 'A')
	  pdp10_print_unsigned (stream, i & WORD_MASK);
      }
      return;

      /* Compute one's complement of an integer constant.  */
    case 'C':
      if (GET_CODE (x) != CONST_INT)
	abort ();
      pdp10_print_integer (stream, ~INTVAL (x));
      return;

      /* Follow jump labels.  If the instruction after the label is an
	 unconditional jump without side effects, substitute the label
	 for the target of that jump.  */
    case 'F':
      if (GET_CODE (x) == LABEL_REF)
	x = XEXP (x, 0);
      if (GET_CODE (x) != CODE_LABEL)
	abort ();
      x = follow_label (x);
      if (GET_CODE (x) == CODE_LABEL || GET_CODE (x) == LABEL_REF)
	output_asm_label (x);
      else
	{
	  if (GET_CODE (x) == MEM)
	    fputc ('@', stream);
	  pdp10_print_operand (stream, x, 'L');
	}
      return;

      /* Print floating-point immediate (constraint 'G').  */
    case 'G':
      if (GET_MODE (x) != SFmode && GET_MODE (x) != DFmode)
	abort ();
      {
	REAL_VALUE_TYPE r;
	char str[30];
	REAL_VALUE_FROM_CONST_DOUBLE (r, x);
	REAL_VALUE_TO_DECIMAL (r, "%.3g", str);
	fputc ('(', stream);
	fputs (str, stream);
	fputc (')', stream);
      }
      return;

      /* Print local symbolic address constant, if possible and appropriate. */
    case 'L':
      if (GET_CODE (x) != SYMBOL_REF && GET_CODE (x) != LABEL_REF)
	break;
      /* ADDRESS: use indirect addressing to access extended symbols
	 in the large code model.  */
      if (TARGET_EXTENDED
	  && !TARGET_SMALLISH
	  && GET_CODE (x) == SYMBOL_REF
	  && !SYMBOL_REF_INTERNAL (x))
	{
#if XKL_STUFF
	  if (pdp10_outputting_debug_info)
	    fputs ("@[DBL ", stream);
	  else
#endif
	  fputs ("@[GIW ", stream);
	  pdp10_giw++;
	  output_addr_const (stream, x);
	  fputc (']', stream);
	}
      else
	output_addr_const (stream, x); 	/*assemble_name (stream,  */
      return;

      /* Print negated integer constant.  */
    case 'N':
      pdp10_print_integer (stream, -INTVAL (x));
      return;

      /* Print stack register in a pdp10_push_operand or
         pdp10_pop_operand.  */
    case 'P':
      if (GET_CODE (x) != MEM
	  || (GET_MODE (x) != SImode && GET_MODE (x) != Pmode))
	abort ();
      x = XEXP (x, 0);
      if (GET_CODE (x) != PRE_INC && GET_CODE (x) != POST_DEC)
	abort ();
      x = XEXP (x, 0);
      if (GET_CODE (x) != REG)
	abort ();
      print_reg (stream, REGNO (x));
      return;

      /* Print lower half.  */
    case 'Q':
      if (GET_CODE (x) != CONST_INT)
	abort ();
      pdp10_print_integer (stream, INTVAL (x) & 0777777);
      return;

      /* Print reversed condition.  */
    case 'R':
      fputs (condition_string (reverse_condition (GET_CODE (x))), stream);
      return;

      /* Swap half-words of an integer constant.  */
    case 'S':
      if (GET_CODE (x) != CONST_INT)
	abort ();
      {
	HOST_WIDE_INT i =  INTVAL (x)        & 0777777;
	HOST_WIDE_INT j = (INTVAL (x) >> 18) & 0777777;
	pdp10_print_integer (stream, (i << 18) | j);
      }
      return;

      /* Compute one's complement and swap half-words of an integer
         constant.  */
    case 'T':
      if (GET_CODE (x) != CONST_INT)
	abort ();
      {
	HOST_WIDE_INT i =  (~INTVAL (x))        & 0777777;
	HOST_WIDE_INT j = ((~INTVAL (x)) >> 18) & 0777777;
	pdp10_print_integer (stream, (i << 18) | j);
      }
      return;

      /* Print upper half.  */
    case 'U':
      if (GET_CODE (x) != CONST_INT)
	abort ();
      pdp10_print_integer (stream, (INTVAL (x) >> 18) & 0777777);
      return;

      /* Print canonical rotate count.  */
    case 'V':
    case 'Y':
      if (GET_CODE (x) != CONST_INT)
	abort ();
      {
	HOST_WIDE_INT i = INTVAL (x);
	HOST_WIDE_INT n = code == 'Y' ? 36 : 72;
	i %= n;
	if (i < 0)
	  i += n;
	if (i > n / 2)
	  i = i - n;
	if (i != 0)
	  pdp10_print_integer (stream, i);
      }
      return;

      /* Force word address.  */
    case 'W':
      /* Handled below.  */
      break;

      /* Force a global indirect word.  */
    case 'X':
      if (TARGET_EXTENDED)
	flags |= PRINT_OP_GIW;
      break;

      /* Print next location.  */
    case 'Z':
      switch (GET_CODE (x))
	{
	case REG:
	  print_reg (stream, (REGNO (x) + 1) % 020);
	  break;
	case MEM:
	  if (GET_CODE (XEXP (x, 0)) == MEM)
	    abort ();
	  x = plus_constant (XEXP (x, 0), 1);
	  pdp10_print_operand_address (stream, x, word_mode, 0);
	  break;
	default:
	  debug_rtx (x);
	  abort ();
	}
      return;
    }

  switch (GET_CODE (x))
    {
    case REG:
      print_reg (stream, REGNO (x));
      break;

    case MEM:
      pdp10_print_operand_address (stream, XEXP (x, 0),
				   code == 'W' ? word_mode : GET_MODE (x),
				   code | flags);
      break;

    case CONST_DOUBLE:
      if (GET_MODE (x) == VOIDmode)
	{
	  unsigned HOST_WIDE_INT high = CONST_DOUBLE_HIGH (x);
	  unsigned HOST_WIDE_INT low = CONST_DOUBLE_LOW (x);

#ifdef __PDP10_GAS_H__
	  fputs (".long ", stream);
#else
	  if (code != 'A' && code != 'B')
	    fputs ("EXP ", stream);
#endif
	  if (code != 'B')
	    pdp10_print_unsigned (stream,
				  ((high << (HOST_BITS_PER_WIDE_INT - 36))
				   + (low >> 36)) & WORD_MASK);
	  if (code != 'A' && code != 'B')
	    fputc (',', stream);
	  if (code != 'A')
	    pdp10_print_unsigned (stream, low & WORD_MASK);
	}
      else if (GET_MODE (x) == SFmode)
	{
	  REAL_VALUE_TYPE r;

#if 0
	  char str[30];

	  REAL_VALUE_FROM_CONST_DOUBLE (r, x);
	  REAL_VALUE_TO_DECIMAL (r,
				 GET_MODE (x) == SFmode ? "%.9g" : "%.19g",
				 str);
	  fputs (str, stream);
#else
	  long y[3];
	  HOST_WIDE_INT z;

	  REAL_VALUE_FROM_CONST_DOUBLE (r, x);
	  REAL_VALUE_TO_TARGET_DOUBLE (r, y);

#ifdef __PDP10_GAS_H__
	  fputs (".long ", stream);
#endif

	  z = (HWINT (y[0]) << 4)
	    + HWINT ((y[1] >> 28) & 0xfUL);
	  pdp10_print_unsigned (stream, z);
#endif
	}
      else if (GET_MODE (x) == DFmode)
	{
	  REAL_VALUE_TYPE r;
	  HOST_WIDE_INT z;
	  long y[3];

	  REAL_VALUE_FROM_CONST_DOUBLE (r, x);
	  REAL_VALUE_TO_TARGET_DOUBLE (r, y);

#ifdef __PDP10_GAS_H__
	  fputs (".long ", stream);
#else
	  if (code != 'A' && code != 'B')
	    fputs ("EXP ", stream);
#endif

	  if (code != 'B')
	    {
	      z = (HWINT (y[0]) << 4)
		+ HWINT ((y[1] >> 28) & 0xfUL);
	      pdp10_print_unsigned (stream, z);
	    }

	  if (code != 'A' && code != 'B')
	    fputc (',', stream);

	  if (code != 'A')
	    {
	      z = ((HWINT (y[1]) << 8) & 0xfffffffUL)
		+ HWINT (((y[2] >> 24) & 0xffUL));
	      pdp10_print_unsigned (stream, z);
	    }
	}
      else
	abort ();
      break;

    case CONST_INT:
      pdp10_print_integer (stream, INTVAL (x));
      break;

    case CONST:
    case LABEL_REF:
    case SYMBOL_REF:
      {
	HOST_WIDE_INT offset = 0;
	int ps, bytesize, position;

	if (GET_CODE (x) == CONST)
	  {
	    x = XEXP (x, 0);
	    if (GET_CODE (x) != PLUS
		|| (GET_CODE (XEXP (x, 0)) != SYMBOL_REF
		    && GET_CODE (XEXP (x, 0)) != LABEL_REF)
		|| GET_CODE (XEXP (x, 1)) != CONST_INT)
	      abort ();
	    offset = INTVAL (XEXP (x, 1));
	    x = XEXP (x, 0);
	  }

	if ((offset & ~ADDRESS_MASK) == (((HOST_WIDE_INT)-1) & ~ADDRESS_MASK))
	  {
	    ps = 0;
	    offset = -(-offset & ADDRESS_MASK);
	  }
	else
	  {
	    ps = (offset >> 24) & (TARGET_EXTENDED ? 07700 : 07777);
	    offset &= ADDRESS_MASK;
	    if (offset & ((HOST_WIDE_INT)1 << 29))
	      {
		if (ps != 0)
		  ps += 0100;
		offset ^= (HOST_WIDE_INT)1 << 29;
		offset -= (HOST_WIDE_INT)1 << 29;
	      }
	  }

	if (ps != 0 && code != 'W')
	  {
	    if (decode_ps (ps, &position, &bytesize) >= 0)
	      {
		x = plus_constant (x, offset);
		print_byte_pointer (stream, x, bytesize, position);
	      }
	    else
	      {
		offset += (HOST_WIDE_INT)ps << 24;
		print_word_pointer (stream, x, flags | PRINT_OP_GIW);
		if (offset != 0)
		  {
		    fputc ('+', stream);
		    pdp10_print_unsigned (stream, offset);
		  }
	      }
	  }
	else
	  {
	    print_word_pointer (stream, x, flags);

	    if (offset > 0)
	      {
		HOST_WIDE_INT sign;
		sign = (HOST_WIDE_INT)1 << (TARGET_EXTENDED ? 29 : 17);
		offset = (offset ^ sign) - sign;
	      }

	    if (offset > 0)
	      fputc ('+', stream);
	    pdp10_print_offset30 (stream, offset);
	  }
      }
      break;

    case LT: case EQ: case LE: case GE: case NE: case GT:
      fputs (condition_string (GET_CODE (x)), stream);
      break;

    case MINUS:
      if (GET_CODE (XEXP (x, 0)) != SYMBOL_REF
	  || GET_CODE (XEXP (x, 1)) != SYMBOL_REF)
	abort ();
#if XKL_STUFF
      if (pdp10_outputting_debug_info)
	fputs ("DBE <", stream);
#endif
      print_word_pointer (stream, XEXP (x, 0), 0);
      fputc ('-', stream);
      print_word_pointer (stream, XEXP (x, 1), 0);
#if XKL_STUFF
      if (pdp10_outputting_debug_info)
	fputc ('>', stream);
#endif
      break;

    default:
      debug_rtx (x);
      abort ();
    }
}

/* Print a register.  */
static void
print_reg (stream, regno)
     FILE *stream;
     int regno;
{
  fputs (REGISTER_PREFIX, stream);
  fputs (reg_names[regno], stream);
}

/* Output a byte pointer to STREAM.  SIZE and POS specifies the byte
   within the word X.  */
static void
print_byte_pointer (stream, x, size, pos)
     FILE *stream;
     rtx x;
     int size;
     int pos;
{
  /* ADDRESS: select between global and local byte pointers.  */
  if (TARGET_EXTENDED
      && (CONSTANT_ADDRESS_P (x)
	  || (GET_CODE (x) == MEM && CONSTANT_ADDRESS_P (XEXP (x, 0)))))
    {
      /* Need a global byte pointer.  */

      if (VALID_OWGBP_BYTE_SIZE_P (size) && (pos % size == 0))
	{
	  /* Output a one-word global byte pointer.  */

	  static const char  *byte6_selector[] = { "46","47","50",
						   "51","52","53" };
	  static const char  *byte7_selector[] = { "62","63","64","65","66" };
	  static const char  *byte8_selector[] = { "55","56","57","60" };
	  static const char  *byte9_selector[] = { "70","71","72","73" };
	  static const char *byte18_selector[] = { "75","76" };
	  const char **byte_selector;

	  switch (size)
	    {
	    case  6: byte_selector =  byte6_selector; break;
	    case  7: byte_selector =  byte7_selector; break;
	    case  8: byte_selector =  byte8_selector; break;
	    case  9: byte_selector =  byte9_selector; break;
	    case 18: byte_selector = byte18_selector; break;
	    }

#ifdef __PDP10_GAS_H__
	  fputs (".long ", stream);
	  fputs (byte_selector[pos / size], stream);
	  fputs ("0000000000+", stream);
#else
	  fputs ("OWGBP ", stream);
	  fputs (byte_selector[pos / size], stream);
	  fputc (',', stream);
#endif
	  pdp10_print_operand (stream, x, 'W' | PRINT_OP_NO_BRACKETS);
	  pdp10_giw++;
	}
      else
	{
	  /* Output a two-word global byte pointer.  */

#ifdef __PDP10_GAS_H__
	  fprintf (stream, ".long %02o%02o40000000", 36 - pos - size, size);
	  pdp10_print_operand (stream, x,
			       'W' | PRINT_OP_NO_BRACKETS | PRINT_OP_GIW);
#else
	  fprintf (stream, "EXP <%02o%02o40,,0>,<", 36 - pos - size, size);
	  pdp10_print_operand (stream, x,
			       'W' | PRINT_OP_NO_BRACKETS | PRINT_OP_GIW);
	  fputc ('>', stream);
#endif
	}
    }
  else
    {
      /* Output a local byte pointer.  */

#ifdef __PDP10_GAS_H__
      fprintf (stream, ".long %02o%02o00000000+", 36 - pos - size, size);
      pdp10_print_operand (stream, x, 'W');
#else
      fprintf (stream, "POINT %d,", size);
      pdp10_print_operand (stream, x, 'W');
      fprintf (stream, ",%d", pos + size - 1);
#endif
    }
}

/* Print X, a word pointer, to STREAM.  Any bits other than the word
   address bits are ignored.  If FLAGS & PRINT_OP_INDIRECT is nonzero,
   make a global pointer indirect.  If FLAGS & PRINT_OP_GIW is
   nonzero, print a global pointer even though TARGET_SMALLISH is in
   effect.  */
static void
print_word_pointer (stream, x, flags)
     FILE *stream;
     rtx x;
     int flags;
{
  HOST_WIDE_INT offset = 0;

  if (TARGET_EXTENDED && (!TARGET_SMALLISH || (flags & PRINT_OP_GIW) != 0))
    {
      /* ADDRESS: use a global indirect word.  */
      if (flags & PRINT_OP_INDIRECT)
	fputs ("@[", stream);

#if XKL_STUFF
      if (pdp10_outputting_debug_info)
	fputs ("DBL ", stream);
      else
#endif
      fputs ("GIW ", stream);

      pdp10_giw++;
    }

  switch (GET_CODE (x))
    {
    case CONST:
      x = XEXP (x, 0);
      if (GET_CODE (x) != PLUS
	  || GET_CODE (XEXP (x, 0)) != SYMBOL_REF
	  || GET_CODE (XEXP (x, 1)) != CONST_INT)
	abort ();
      offset = INTVAL (XEXP (x, 1)) & ADDRESS_MASK;
      x = XEXP (x, 0);
      /* Fall through.  */

    case LABEL_REF:
    case SYMBOL_REF:
      output_addr_const (stream, x);
      if (offset > 0)
	fputc ('+', stream);
      pdp10_print_offset30 (stream, offset);
      break;

    case CONST_INT:
      pdp10_print_address (stream, INTVAL (x));
      break;

    default:
      abort ();
    }

  if (TARGET_EXTENDED && (flags & PRINT_OP_INDIRECT) != 0
      && (!TARGET_SMALLISH || (flags & PRINT_OP_GIW) != 0))
    fputc (']', stream);
}

/* Called by the PRINT_OPERAND_ADDRESS macro.

   Output to the stdio stream STREAM the assembler syntax for an
   instruction operand that is a memory reference whose address is X.  */
void
pdp10_print_operand_address (stream, x, mode, flags)
     FILE *stream;
     rtx x;
     enum machine_mode mode;
     int flags;
{
  int byte_pointer = BYTE_MODE_P (mode);

  if (!pdp10_legitimate_address_p (mode, x, 1))
    {
      debug_rtx (x);
      abort ();
    }

  switch (GET_CODE (x))
    {
    case CONST_INT:
      flags = PRINT_OP_INDIRECT;
      if (TARGET_EXTENDED && INTVAL (x) > 017)
	flags |= PRINT_OP_GIW;
      print_word_pointer (stream, x, flags);
      break;

    case CONST:
    case SYMBOL_REF:
      {
	int indirect = !(flags & PRINT_OP_NO_BRACKETS);
	HOST_WIDE_INT offset = 0;
	rtx sym = x;
	int ps = 0;

	if (GET_CODE (x) == CONST)
	  {
	    sym = XEXP (XEXP (x, 0), 0);
	    offset = INTVAL (XEXP (XEXP (x, 0), 1));
	    ps = (offset >> 24) & 07700;
	    offset &= ADDRESS_MASK;
	  }

	if (ps != 0 && byte_pointer)
	  {
	    int bitpos, bitsize;

	    if (decode_ps (ps, &bitpos, &bitsize) == -1)
	      abort ();

	    if (indirect)
	      fputc ('[', stream);
	    print_byte_pointer (stream, x, bitsize, bitpos);
	    if (indirect)
	      fputc (']', stream);
	  }
	else
	  {
	    if (indirect)
	      flags |= PRINT_OP_INDIRECT;
	    print_word_pointer (stream, x, flags);
	  }

	break;
      }

    case REG:
      if (!byte_pointer)
	{
	  if (REGNO (x) == 0)
	    fputc ('@', stream);
	  else
	    fputc ('(', stream);
	}
      print_reg (stream, REGNO (x));
      if (!byte_pointer && REGNO (x) != 0)
	fputc (')', stream);
      break;

    case PLUS:
      if (GET_CODE (XEXP (x, 0)) != REG
	  || (GET_CODE (XEXP (x, 1)) != CONST_INT
	      && GET_CODE (XEXP (x, 1)) != CONST
	      && GET_CODE (XEXP (x, 1)) != LABEL_REF
	      && GET_CODE (XEXP (x, 1)) != SYMBOL_REF)
	  || REGNO (XEXP (x, 0)) == 0)
	abort ();

      if (GET_CODE (XEXP (x, 1)) == CONST_INT)
	{
	  HOST_WIDE_INT offset = INTVAL (XEXP (x, 1));
	  if (offset >= -0400000 && offset <= 0377777)
	    pdp10_print_offset (stream, INTVAL (XEXP (x, 1)));
	  else
	    {
	      fputs ("@[%EXIND(0,", stream);
	      pdp10_giw++;
	      print_reg (stream, REGNO (XEXP (x, 0)));
	      fputc (',', stream);
	      offset = offset & ADDRESS_MASK;
	      pdp10_print_offset30 (stream, offset);
	      fputs (")]", stream);
	      break;
	    }
	}
      else if (GET_CODE (XEXP (x, 1)) == CONST)
	{
	  rtx y = XEXP (XEXP (x, 1), 0);
	  if (GET_CODE (y) != PLUS
	      || GET_CODE (XEXP (y, 0)) != SYMBOL_REF
	      || GET_CODE (XEXP (y, 1)) != CONST_INT)
	    abort ();
	  output_addr_const (stream, XEXP (y, 0));
	  if (INTVAL (XEXP (y, 1)) >= 0)
	    fputc ('+', stream);
	  pdp10_print_offset (stream, INTVAL (XEXP (y, 1)));
	}
      else
	output_addr_const (stream, XEXP (x, 1));

      fputc ('(', stream);
      print_reg (stream, REGNO (XEXP (x, 0)));
      fputc (')', stream);
      break;

    case MEM:
      if (!byte_pointer)
	fputc ('@', stream);
      pdp10_print_operand_address (stream, XEXP (x, 0), Pmode, 0);
      break;

    case PRE_INC:
      /* PRE_INC is used with the ILDB and IDPB instructions.  */
      if (!byte_pointer)
	abort ();
      pdp10_print_operand_address (stream, XEXP (x, 0), mode, 0);
      break;

    case UNSPEC:
      if (XINT (x, 1) != UNSPEC_ADDRESS)
	abort ();
      pdp10_print_operand_address (stream, XVECEXP (x, 0, 0), mode, 0);
      break;

    default:
      abort ();
    }
}


/**********************************************************************

	Target Attributes

**********************************************************************/

static void	make_byte_type PARAMS ((tree type, int precision,
					int unsignedp));
static void	layout_byte_decl PARAMS ((tree decl));
static tree	pdp10_handle_size_attribute PARAMS ((tree *node, tree name,
						     tree args, int flags,
						     bool *no_add_attrs));
static tree	pdp10_handle_fastcall_attribute PARAMS ((tree *node, tree name,
							 tree args, int flags,
							 bool *no_add_attrs));

/* Table of machine attributes.  */
const struct attribute_spec pdp10_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler } */

  /* Specifies the size, in bits, of a type or variable.  */
  { "size",	1, 1, true, false, false, pdp10_handle_size_attribute },

  /* Specifies the size, in bits, of a type or variable.  */
  { "fastcall",	0, 0, false, true, true, pdp10_handle_fastcall_attribute },

  { NULL,           0, 0, false, false, false, NULL }
};

tree char6_signed_type_node;
tree char6_unsigned_type_node;
tree char7_signed_type_node;
tree char7_unsigned_type_node;
tree char8_signed_type_node;
tree char8_unsigned_type_node;
tree char9_signed_type_node;
tree char9_unsigned_type_node;
tree short16_integer_type_node;
tree short16_unsigned_type_node;
tree int32_integer_type_node;
tree int32_unsigned_type_node;

/* Build all PDP-10 byte types.  */
void
pdp10_build_machine_types ()
{
  static int done = 0;

  if (done)
    return;

  char6_signed_type_node = make_node (INTEGER_TYPE);
  make_byte_type (char6_signed_type_node, 6, 0);
  ggc_add_tree_root (&char6_signed_type_node, 1);
  char6_unsigned_type_node = make_node (INTEGER_TYPE);
  make_byte_type (char6_unsigned_type_node, 6, 1);
  ggc_add_tree_root (&char6_unsigned_type_node, 1);

  char7_signed_type_node = make_node (INTEGER_TYPE);
  make_byte_type (char7_signed_type_node, 7, 0);
  ggc_add_tree_root (&char7_signed_type_node, 1);
  char7_unsigned_type_node = make_node (INTEGER_TYPE);
  make_byte_type (char7_unsigned_type_node, 7, 1);
  ggc_add_tree_root (&char7_unsigned_type_node, 1);

  char8_signed_type_node = make_node (INTEGER_TYPE);
  make_byte_type (char8_signed_type_node, 8, 0);
  ggc_add_tree_root (&char8_signed_type_node, 1);
  char8_unsigned_type_node = make_node (INTEGER_TYPE);
  make_byte_type (char8_unsigned_type_node, 8, 1);
  ggc_add_tree_root (&char8_unsigned_type_node, 1);

  char9_signed_type_node
    = flag_signed_char ? char_type_node : signed_char_type_node;
  ggc_add_tree_root (&char9_signed_type_node, 1);
  char9_unsigned_type_node
    = flag_signed_char ? unsigned_char_type_node : char_type_node;
  ggc_add_tree_root (&char9_unsigned_type_node, 1);

  if (pdp10_char_bytesize != 9)
    {
      switch (pdp10_char_bytesize)
	{
	case 6: case 7: case 8: case 9:
	  break;
	default:
	  fprintf (stderr, "Bad value for -mchar-bytesize: %s\n",
		   pdp10_string_bytesize);
	  exit (1);
	}

      if (pdp10_char_bytesize != 9)
	{
	  char_type_node = make_node (INTEGER_TYPE);
	  make_byte_type (char_type_node, pdp10_char_bytesize,
			  !flag_signed_char);
	  signed_char_type_node = make_node (INTEGER_TYPE);
	  make_byte_type (signed_char_type_node, pdp10_char_bytesize, 0);
	  unsigned_char_type_node = make_node (INTEGER_TYPE);
	  make_byte_type (unsigned_char_type_node, pdp10_char_bytesize, 1);
	}
    }

  short16_integer_type_node = make_node (INTEGER_TYPE);
  make_byte_type (short16_integer_type_node, 16, 0);
  ggc_add_tree_root (&short16_integer_type_node, 1);
  short16_unsigned_type_node = make_node (INTEGER_TYPE);
  make_byte_type (short16_unsigned_type_node, 16, 1);
  ggc_add_tree_root (&short16_unsigned_type_node, 1);

  int32_integer_type_node = make_node (INTEGER_TYPE);
  make_byte_type (int32_integer_type_node, 32, 0);
  ggc_add_tree_root (&int32_integer_type_node, 1);
  int32_unsigned_type_node = make_node (INTEGER_TYPE);
  make_byte_type (int32_unsigned_type_node, 32, 1);
  ggc_add_tree_root (&int32_unsigned_type_node, 1);

  done = 1;
}

/* Initialize the type in TYPE.  The new type will be suitable as a
   byte of PRECISION bits pointed to by a PDP-10 global byte pointer,
   and will be unsigned if UNSIGNEDP is nonzero.  */
static void
make_byte_type (type, precision, unsignedp)
     tree type;
     int precision;
     int unsignedp;
{
  TYPE_PRECISION (type) = precision;

  if (unsignedp)
    {
      TYPE_MIN_VALUE (type) = build_int_2 (0, 0);
      TYPE_MAX_VALUE (type)
	= build_int_2 (precision - HOST_BITS_PER_WIDE_INT >= 0
		       ? -1 : (HWINT (1) << precision) - 1,
		       precision - HOST_BITS_PER_WIDE_INT > 0
		       ? ((unsigned HOST_WIDE_INT) ~0
			  >> (HOST_BITS_PER_WIDE_INT
			      - (precision - HOST_BITS_PER_WIDE_INT)))
		       : 0);
    }
  else
    {
      TYPE_MIN_VALUE (type)
	= build_int_2 ((precision - HOST_BITS_PER_WIDE_INT > 0
			? 0 : HWINT (-1) << (precision - 1)),
		       ((HWINT (-1)
			 << (precision - HOST_BITS_PER_WIDE_INT - 1 > 0
			     ? precision - HOST_BITS_PER_WIDE_INT - 1
			     : 0))));
      TYPE_MAX_VALUE (type)
	= build_int_2 ((precision - HOST_BITS_PER_WIDE_INT > 0
			? -1 : (HWINT (1) << (precision - 1)) - 1),
		       (precision - HOST_BITS_PER_WIDE_INT - 1 > 0
			? ((HWINT (1)
			    << (precision - HOST_BITS_PER_WIDE_INT - 1))) - 1
			: 0));
    }

  TREE_TYPE (TYPE_MIN_VALUE (type)) = type;
  TREE_TYPE (TYPE_MAX_VALUE (type)) = type;

  TREE_UNSIGNED (type) = unsignedp;

  TYPE_MODE (type) = smallest_mode_for_size (TYPE_PRECISION (type),
					     MODE_INT);
  TYPE_SIZE (type) = bitsize_int (TYPE_PRECISION (type));
  TYPE_SIZE_UNIT (type)
    = size_int ((TYPE_PRECISION (type) + BITS_PER_UNIT - 1) / BITS_PER_UNIT);

  TYPE_ALIGN (type) = precision == 32 ? 36 : precision;
}

/* Lay out a PDP-10 byte variable in memory.  */
static void
layout_byte_decl (decl)
     tree decl;
{
  register tree type = TREE_TYPE (decl);
  register enum tree_code code = TREE_CODE (decl);

  TREE_UNSIGNED (decl) = TREE_UNSIGNED (type);
  DECL_MODE (decl) = TYPE_MODE (type);
  DECL_SIZE (decl) = TYPE_SIZE (type);
  DECL_SIZE_UNIT (decl) = TYPE_SIZE_UNIT (type);

  /* Force alignment required for the data type.
     But if the decl itself wants greater alignment, don't override that.
     Likewise, if the decl is packed, don't override it.  */
  if (!(code == FIELD_DECL && DECL_BIT_FIELD (decl))
      && (DECL_ALIGN (decl) == 0
	  || (!(code == FIELD_DECL && DECL_PACKED (decl))
	      && TYPE_ALIGN (type) > DECL_ALIGN (decl))))
    {	      
      DECL_ALIGN (decl) = TYPE_ALIGN (type);
      DECL_USER_ALIGN (decl) = TYPE_USER_ALIGN (type);
    }

  /* For fields, set the bit field type and update the alignment.  */
  if (code == FIELD_DECL)
    {
      DECL_BIT_FIELD_TYPE (decl) = DECL_BIT_FIELD (decl) ? type : 0;
      if (maximum_field_alignment != 0)
	DECL_ALIGN (decl) = MIN (DECL_ALIGN (decl), maximum_field_alignment);
      else if (DECL_PACKED (decl))
	DECL_USER_ALIGN (decl) = 0;
    }

  /* Evaluate nonconstant size only once, either now or as soon as safe.  */
  if (DECL_SIZE (decl) != 0 && TREE_CODE (DECL_SIZE (decl)) != INTEGER_CST)
    DECL_SIZE (decl) = variable_size (DECL_SIZE (decl));
  if (DECL_SIZE_UNIT (decl) != 0
      && TREE_CODE (DECL_SIZE_UNIT (decl)) != INTEGER_CST)
    DECL_SIZE_UNIT (decl) = variable_size (DECL_SIZE_UNIT (decl));

  /* If requested, warn about definitions of large data objects.  */
  if (warn_larger_than
      && (code == VAR_DECL || code == PARM_DECL)
      && !DECL_EXTERNAL (decl))
    {
      tree size = DECL_SIZE_UNIT (decl);

      if (size != 0 && TREE_CODE (size) == INTEGER_CST
	  && compare_tree_int (size, larger_than_size) > 0)
	{
	  unsigned int size_as_int = TREE_INT_CST_LOW (size);

	  if (compare_tree_int (size, size_as_int) == 0)
	    warning_with_decl (decl, "size of `%s' is %d bytes", size_as_int);
	  else
	    warning_with_decl (decl, "size of `%s' is larger than %d bytes",
			       larger_than_size);
	}
    }
}

#define PDP10_TYPE_FROM_SIZE(SIZE, UNSIGNEDP)				    \
  ((SIZE) == 6 ?							    \
   ((UNSIGNEDP) ? char6_unsigned_type_node : char6_signed_type_node) :	    \
   (SIZE) == 7 ?							    \
   ((UNSIGNEDP) ? char7_unsigned_type_node : char7_signed_type_node) :	    \
   (SIZE) == 8 ?							    \
   ((UNSIGNEDP) ? char8_unsigned_type_node : char8_signed_type_node) :	    \
   (SIZE) == 9 ?							    \
   ((UNSIGNEDP) ? char9_unsigned_type_node : char9_signed_type_node) :	    \
   (SIZE) == 16 ?							    \
   ((UNSIGNEDP) ? short16_unsigned_type_node : short16_integer_type_node) : \
   (SIZE) == 18 ?							    \
   ((UNSIGNEDP) ? short_unsigned_type_node : short_integer_type_node) :	    \
   (SIZE) == 32 ?							    \
   ((UNSIGNEDP) ? int32_unsigned_type_node : int32_integer_type_node) :	    \
   (SIZE) == 36 ?							    \
   ((UNSIGNEDP) ? unsigned_type_node : integer_type_node) :		    \
   NULL_TREE)

/* Function to handle the size attribute.  NODE points to the node to
   which the attribute is to be applied.  If a DECL, it should be
   modified in place; if a TYPE, a copy should be created.  NAME is
   the name of the attribute (possibly with leading or trailing __).
   ARGS is the TREE_LIST of the arguments (which may be NULL).  FLAGS
   gives further information about the context of the attribute.
   Afterwards, the attributes will be added to the DECL_ATTRIBUTES or
   TYPE_ATTRIBUTES, as appropriate, unless *NO_ADD_ATTRS is set to
   true (which should be done on error, as well as in any other cases
   when the attributes should not be added to the DECL or TYPE).
   Depending on FLAGS, any attributes to be applied to another type or
   DECL later may be returned; otherwise the return value should be
   NULL_TREE.  This pointer may be NULL if no special handling is
   required beyond the checks implied by the rest of this structure.  */
static tree
pdp10_handle_size_attribute (node, name, args, flags, no_add_attrs)
     tree *node;
     tree name ATTRIBUTE_UNUSED;
     tree args;
     int flags ATTRIBUTE_UNUSED;
     bool *no_add_attrs;
{
  enum tree_code code = TREE_CODE (*node);

  pdp10_build_machine_types ();

  *no_add_attrs = 1;

  if (code == INTEGER_TYPE
      || code == VAR_DECL
      || code == PARM_DECL
      || code == TYPE_DECL
      || (code == FIELD_DECL && !DECL_BIT_FIELD (*node)))
    {
      int size = 0;

      if (args && TREE_VALUE (args)
	  && TREE_CODE (TREE_VALUE (args)) == INTEGER_CST)
	size = TREE_INT_CST_LOW (TREE_VALUE (args));

      if (!VALID_OWGBP_BYTE_SIZE_P (size)
	  && size != 16 && size != 32 && size != 36)
	{
	  error ("invalid value for `size' attribute");
	  return NULL_TREE;
	}

      if ((code != INTEGER_TYPE
	   && TREE_CODE (TREE_TYPE (*node)) != INTEGER_TYPE)
	  || TREE_CHAIN (args) != NULL_TREE
	  || size == 0)
	return NULL_TREE;

      if (code == INTEGER_TYPE)
	make_byte_type (*node, size, TREE_UNSIGNED (*node));
      else
	{
	  int unsignedp
	    = TREE_UNSIGNED (code == FIELD_DECL ? *node : TREE_TYPE (*node));
	  TREE_TYPE (*node) = PDP10_TYPE_FROM_SIZE (size, unsignedp);
	  layout_byte_decl (*node);
	}

      *no_add_attrs = 0;
    }

  return NULL_TREE;
}

static tree
pdp10_handle_fastcall_attribute (node, name, args, flags, no_add_attrs)
     tree *node;
     tree name ATTRIBUTE_UNUSED;
     tree args ATTRIBUTE_UNUSED;
     int flags ATTRIBUTE_UNUSED;
     bool *no_add_attrs;
{
  if (TREE_CODE (*node) != FUNCTION_TYPE
      && TREE_CODE (*node) != METHOD_TYPE
      && TREE_CODE (*node) != FIELD_DECL
      && TREE_CODE (*node) != TYPE_DECL)
    {
      warning ("`%s' attribute only applies to functions",
	       IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
      return NULL_TREE;
    }

  return NULL_TREE;
}


/**********************************************************************

	Miscellaneout Parameters

**********************************************************************/

static void	convert_voidp_callback PARAMS ((cpp_reader *pfile));
static void	no_convert_voidp_callback PARAMS ((cpp_reader *pfile));
static void	long_long_71bit_callback PARAMS ((cpp_reader *pfile));
static void	no_long_long_callback PARAMS ((cpp_reader *pfile));
static void	string_bytesize_callback PARAMS ((cpp_reader *pfile));
static int	get_pragma_number PARAMS ((cpp_reader *pfile,
					   const char *pragma));

static void
convert_voidp_callback (pfile)
     cpp_reader *pfile ATTRIBUTE_UNUSED;
{
  target_flags &= ~OPT_VOIDP;
}

static void
no_convert_voidp_callback (pfile)
     cpp_reader *pfile ATTRIBUTE_UNUSED;
{
  target_flags |= OPT_VOIDP;
}

static void
no_long_long_callback (pfile)
     cpp_reader *pfile ATTRIBUTE_UNUSED;
{
  target_flags &= ~CPU_71BIT;
  warning ("#pragma no_long_long doesn't work properly.  "
	   "Use -mno-long-long instead.");
}

static void
long_long_71bit_callback (pfile)
     cpp_reader *pfile ATTRIBUTE_UNUSED;
{
  target_flags |= CPU_71BIT;
  warning ("#pragma long_long_71bit doesn't work properly.  "
	   "Use -mlong-long-71bit instead.");
}

static int
get_pragma_number (pfile, pragma)
     cpp_reader *pfile;
     const char *pragma;
{
  const cpp_token *token = cpp_get_token (pfile);
  char *text = (char *)token->val.str.text;
  char *end;
  int n;

  if (token->type != CPP_NUMBER)
    {
      warning ("#pragma %s takes a numeric argument", pragma);
      return -1;
    }

  end = text + token->val.str.len;
  n = strtol (text, &end, 10);

  if (end == text)
    return -1;
  else
    return n;
}

static void
string_bytesize_callback (pfile)
     cpp_reader *pfile;
{
  int n = get_pragma_number (pfile, "string_bytesize");

  if (n == -1)
    return;

  switch (n)
    {
    case 6: case 7: case 8: case 9:
      break;
    default:
      warning ("#pragma string_bytesize argument must be 6, 7, 8, or 9");
      return;
    }

  pdp10_string_literal_byte_size = n;
}

/* Called by the REGISTER_TARGET_PRAGMAS macro.  */
void
pdp10_register_target_pragmas (ptr)
     void *ptr;
{
  cpp_reader *pfile = ptr;
  cpp_register_pragma (pfile, NULL, "convert_voidp", convert_voidp_callback);
  cpp_register_pragma (pfile, NULL, "no_convert_voidp",
		       no_convert_voidp_callback);
  cpp_register_pragma (pfile, NULL, "long_long_71bit",
		       long_long_71bit_callback);
  cpp_register_pragma (pfile, NULL, "no_long_long", no_long_long_callback);
  cpp_register_pragma (pfile, NULL, "string_bytesize",
		       string_bytesize_callback);
}

/* Called by the MACHINE_DEPENDENT_REORG macro.

   This is ran as the last optimization pass.  (If there was a delayed
   branch scheduling pass in the PDP-10 compiler, it would run after
   this pass.)  */
void
pdp10_machine_dependent_reorg (first)
     rtx first;
{
  rtx insn;

  if (!optimize)
    return;

  for (insn = first; insn; insn = NEXT_INSN (insn))
    {
      /* We only care about INSNs, JUMP_INSNs, and CALL_INSNs.
	 Also ignore any special USE insns.  */
      if ((GET_CODE (insn) != INSN
	   && GET_CODE (insn) != JUMP_INSN
	   && GET_CODE (insn) != CALL_INSN)
	  || GET_CODE (PATTERN (insn)) == USE
	  || GET_CODE (PATTERN (insn)) == CLOBBER
	  || GET_CODE (PATTERN (insn)) == ADDR_VEC
	  || GET_CODE (PATTERN (insn)) == ADDR_DIFF_VEC)
	continue;

      if (get_attr_reorg_type (insn) == REORG_TYPE_LDB
	  || get_attr_reorg_type (insn) == REORG_TYPE_DPB)
	{
	  rtx links;
	  for (links = LOG_LINKS (insn); links; links = XEXP (links, 1))
	    {
	      rtx prev = XEXP (links, 0);
	      rtx insn_set_src, insn_set_dest, prev_set_dest;

	      if (GET_CODE (prev) != INSN
		  || get_attr_reorg_type (prev) != REORG_TYPE_IBP)
		continue;

	      insn_set_src = SET_SRC (PATTERN (insn));
	      insn_set_dest = SET_DEST (PATTERN (insn));
	      prev_set_dest = SET_DEST (PATTERN (prev));

	      /* Check for DPB instruction.  */
	      if (GET_CODE (insn_set_dest) == MEM
		  && GET_CODE (prev_set_dest) == REG
		  && GET_CODE (XEXP (insn_set_dest, 0)) == REG
		  && REGNO (prev_set_dest) == REGNO (XEXP (insn_set_dest, 0)))
		{
		  rtx pre_inc = gen_rtx_PRE_INC (Pmode, prev_set_dest);
		  XEXP (insn_set_dest, 0) = pre_inc;
		  INSN_DELETED_P (prev) = 1;
		}

	      /* Check for LDB instruction.  */
	      if (GET_CODE (insn_set_src) == ZERO_EXTEND
		  || GET_CODE (insn_set_src) == SIGN_EXTEND)
		insn_set_src = XEXP (insn_set_src, 0);
	      if (GET_CODE (insn_set_src) == MEM
		  && GET_CODE (prev_set_dest) == REG
		  && GET_CODE (XEXP (insn_set_src, 0)) == REG
		  && REGNO (prev_set_dest) == REGNO (XEXP (insn_set_src, 0)))
		{
		  rtx pre_inc = gen_rtx_PRE_INC (Pmode, prev_set_dest);
		  XEXP (insn_set_src, 0) = pre_inc;
		  INSN_DELETED_P (prev) = 1;
		}
	    }
	}
    }
}


/**********************************************************************

	Special Predicates

**********************************************************************/

/* Return the known alignment, in storage units, of pointer X.  */
int
pdp10_pointer_alignment (x)
     rtx x;
{
  /* This function must be prepared to handle anything
     GO_IF_LEGITIMATE_ADDRESS says is a valid address.  */

  switch (GET_CODE (x))
    {
    case CONST_INT:
      return 1;
    case CONST:
      if (!TARGET_EXTENDED)
	abort ();
      switch ((int)(INTVAL (XEXP (XEXP (x, 0), 1)) >> 30) & 077)
	{
	case 0:
	case 075:
	case 070:
	  return 4;
	case 076:
	case 072:
	  return 2;
	case 071:
	case 073:
	  return 1;
	default:
	  abort ();
	}
    case LABEL_REF:
    case SYMBOL_REF:
      /* Symbols are always word-aligned.  */
      return UNITS_PER_WORD;
    case PLUS:
      if (!CONSTANT_ADDRESS_P (XEXP (x, 1)))
	{
	  debug_rtx (x);
	  abort ();
	}
      x = XEXP (x, 0);
      /* Fall through.  */
    case REG:
    reg:
      if (REG_POINTER (x))
	return REGNO_POINTER_ALIGN (REGNO (x)) / BITS_PER_UNIT;
      else
	return 1;
    case PRE_INC:
      x = XEXP (x, 0);
      if (REG_P (x))
	goto reg;
      return 1;
    case MEM:
      return 1;
    default:
      abort ();
    }
}

/* Return nonzero if OP is a valid PDP-10 comparison condition.  */
int
pdp10_comparison_operator (op, mode)
     rtx op;
     enum machine_mode mode ATTRIBUTE_UNUSED;
{
  switch (GET_CODE (op))
    {
    case EQ:
    case NE:
    case LT:
    case LE:
    case GT:
    case GE:
      return 1;
    default:
      return 0;
    }
}

/* Return nonzero if OP is a valid PDP-10 equality comparision
   condition.  */
int
pdp10_equality_operator (op, mode)
     rtx op;
     enum machine_mode mode ATTRIBUTE_UNUSED;
{
  enum rtx_code code = GET_CODE (op);
  return code == EQ || code == NE;
}

/* Return nonzero if OP is a rotation operation.  */
int
pdp10_rotate_operator (op, mode)
     rtx op;
     enum machine_mode mode ATTRIBUTE_UNUSED;
{
  enum rtx_code code = GET_CODE (op);
  return code == ROTATE || code == ROTATERT;
}

/* Return nonzero if OP is a valid PDP-10 memory operand.  */
int
reg_or_mem_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return register_operand (op, mode) || memory_operand (op, mode);
}

/* Return nonzero if OP is a valid PDP-10 register-or-memory operand,
   but only allow memory operands after register allocation has
   finished.  */
int
preferably_register_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return
    register_operand (op, mode)
    || (reload_completed && memory_operand (op, mode));
}

/* Return nonzero if OP is a memory operand that may be volatile.  */
int
pdp10_maybe_volatile_memory_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  int saved = volatile_ok;
  int result;

  volatile_ok = 1;
  result = memory_operand (op, mode);
  volatile_ok = saved;
  return result;
}

/* Return nonzero if OP is a memory operand with a constant address.  */
int
pdp10_constaddr_memory_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  return memory_operand (op, mode) && CONSTANT_ADDRESS_P (XEXP (op, 0));
}

/* Return nonzero if OP is a valid PDP-10 push operand.  */
int
pdp10_push_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (GET_CODE (op) != MEM)
    return 0;
  if (mode != VOIDmode && GET_MODE (op) != mode)
    return 0;
  op = XEXP (op, 0);
  if (GET_CODE (op) != PRE_INC)
    return 0;
  op = XEXP (op, 0);
  /* Since PUSH is a slow instruction, only allow registers other than
     then stack pointer when optimizing for small code size.  */
  return REG_P (op) && (REGNO (op) == STACK_POINTER_REGNUM
			|| (optimize_size && TARGET_EXTENDED));
}

/* Return nonzero if OP is a valid PDP-10 pop operand.  */
int
pdp10_pop_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (GET_CODE (op) != MEM)
    return 0;
  if (mode != VOIDmode && GET_MODE (op) != mode)
    return 0;
  op = XEXP (op, 0);
  if (GET_CODE (op) != POST_DEC)
    return 0;
  op = XEXP (op, 0);
  /* Since POP is a slow instruction, only allow registers other than
     then stack pointer when optimizing for small code size.  */
  return REG_P (op) && (REGNO (op) == STACK_POINTER_REGNUM
			|| (optimize_size && TARGET_EXTENDED));
}

/* Return nonzero if OP is a floating-point zero constant.  */
int
pdp10_const_double_0_operand (op, mode)
     rtx op;
     enum machine_mode mode;
{
  REAL_VALUE_TYPE r;
  long y[3];

  if (mode != VOIDmode && GET_MODE (op) != mode)
    return 0;

  if (GET_CODE (op) != CONST_DOUBLE)
    return 0;

  REAL_VALUE_FROM_CONST_DOUBLE (r, op);
  REAL_VALUE_TO_TARGET_DOUBLE (r, y);

  switch (mode)
    {
    case SFmode:
      return
	(y[0] & 0xffffffffUL) == 0
	&& (y[1] & 0xf0000000UL) == 0;
    case DFmode:
      return
	(y[0] & 0xffffffffUL) == 0
	&& (y[1] & 0xffffffffUL) == 0
	&& (y[2] & 0xff000000UL) == 0;
    default:
      abort ();
    }
}

#if 0
int
pdp10_halfword_destination (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (mode != SImode)
    return 0;

  if (GET_CODE (op) == ZERO_EXTRACT)
    return
      GET_CODE (XEXP (op, 1)) == CONST_INT
      && INTVAL (XEXP (op, 1)) == 18
      && GET_CODE (XEXP (op, 2)) == CONST_INT
      && (INTVAL (XEXP (op, 2)) == 0 || INTVAL (XEXP (op, 2)) == 18);

  if (GET_CODE (op) == SUBREG)
    return
      GET_CODE (XEXP (op, 0)) == MEM
      && GET_MODE (XEXP (op, 0)) == HImode
      && SUBREG_BYTE (op) == 0;

  return 0;
}

int
pdp10_halfword_source (op, mode)
     rtx op;
     enum machine_mode mode;
{
  if (mode != SImode)
    return 0;

  if (GET_CODE (op) == ZERO_EXTRACT || GET_CODE (op) == SIGN_EXTRACT)
    return
      GET_CODE (XEXP (op, 1)) == CONST_INT
      && INTVAL (XEXP (op, 1)) == 18
      && GET_CODE (XEXP (op, 2)) == CONST_INT
      && (INTVAL (XEXP (op, 2)) == 0 || INTVAL (XEXP (op, 2)) == 18);

  if (GET_CODE (op) == SUBREG)
    return
      GET_CODE (XEXP (op, 0)) == MEM
      && GET_MODE (XEXP (op, 0)) == HImode
      && SUBREG_BYTE (op) == 0;

  if (GET_CODE (op) == AND)
    return
      GET_CODE (XEXP (op, 1)) == CONST_INT
      && INTVAL (XEXP (op, 1)) == 0777777;

  if (GET_CODE (op) == ASHIFTRT || GET_CODE (op) == LSHIFTRT)
    return
      GET_CODE (XEXP (op, 1)) == CONST_INT
      && INTVAL (XEXP (op, 1)) == 18;

  return 0;
}
#endif


/**********************************************************************

	Built-in Functions

**********************************************************************/

static rtx	expand_builtin_jffo PARAMS ((tree arglist, rtx target));
static rtx	expand_builtin_fsc PARAMS ((tree arglist, rtx target,
					    enum rtx_code code));

/* Called by the MD_INIT_BUILTINS macro.

   Performs the necessary setup for machine-specific builtin
   functions.  */
void
pdp10_init_builtins ()
{
#define PTRTYPE(a)	built_pointer_type (a)
#define NIL		void_list_node
#define CONS(a,b)	tree_cons (NULL_TREE, a, b)
#define FUNTYPE(a,b)	build_function_type (a, b)
#define FUNTYPE0(a)	FUNTYPE (a, NIL)
#define FUNTYPE1(a,b)	FUNTYPE (a, CONS (b, NIL))
#define FUNTYPE2(a,b,c)	FUNTYPE (a, CONS (b, CONS (c, NIL)))
#define VOID		void_type_node
#define VOIDP		ptr_type_node
#define INT		integer_type_node
#define CHAR		char_type_node
#define LONGLONG	long_long_integer_type_node
#define FLOAT		float_type_node
#define DOUBLE		double_type_node

  builtin_function ("__builtin_fsc",
		    FUNTYPE2 (FLOAT, FLOAT, INT),
		    PDP10_BUILTIN_FSC,
		    BUILT_IN_MD,
		    0);

  builtin_function ("__builtin_dfsc",
		    FUNTYPE2 (DOUBLE, DOUBLE, INT),
		    PDP10_BUILTIN_DFSC,
		    BUILT_IN_MD,
		    0);

  if (HAVE_JFFO)
    {
      builtin_function ("__builtin_jffo",
			FUNTYPE2 (VOID, LONGLONG, VOIDP),
			PDP10_BUILTIN_JFFO,
			BUILT_IN_MD,
			0);
    }

#undef PTRTYPE
#undef NIL
#undef CONS
#undef FUNTYPE0
#undef FUNTYPE1
#undef FUNTYPE2
#undef VOID
#undef VOIDP
#undef INT
#undef CHAR
#undef LONGLONG
#undef FLOAT
#undef DOUBLE
}

/* Expand a call to __builtin_jffo.  */
static rtx
expand_builtin_jffo (arglist, target)
     tree arglist;
     rtx target;
{
  tree arg0, arg1;
  rtx op0, op1, pat;

  arg0 = TREE_VALUE (arglist);
  arg1 = TREE_VALUE (TREE_CHAIN (arglist));
  op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
  op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);

  op0 = force_reg (DImode, op0);
  if (!address_operand (op1, Pmode))
    op1 = force_reg (Pmode, op1);
  pat = gen_JFFO (op0, op1);
  if (!pat)
    return 0;
  emit_jump_insn (pat);

  return target;
}

/* Expand a call to __builtin_(d)fsc.  */
static rtx
expand_builtin_fsc (arglist, target, code)
     tree arglist;
     rtx target ATTRIBUTE_UNUSED;
     enum pdp10_builtins code;
{
  tree arg0, arg1;
  rtx op0, op1, pat;

  arg0 = TREE_VALUE (arglist);
  arg1 = TREE_VALUE (TREE_CHAIN (arglist));
  op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
  op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
  op0 = force_reg (DFmode, op0);
  op1 = force_not_mem (op1);

  if (!target)
    target = gen_reg_rtx (code == PDP10_BUILTIN_FSC ? SFmode : DFmode);

  if (code == PDP10_BUILTIN_FSC)
    pat = gen_FSC (target, op0, op1);
  else if (TARGET_GFLOAT)
    pat = gen_GFSC (target, op0, op1);
  else
    pat = gen_DFSC (target, op0, op1);
  if (!pat)
    return 0;
  emit_insn (pat);

  return target;
}

/* Called by the MD_EXPAND_BUILTIN macro.

   Expand a call to a machine specific builtin that was set up by
   MD_INIT_BUILTINS.  EXP is the expression for the function call; the
   result should go to TARGET if that is convenient, and have mode
   MODE if that is convenient.  SUBTARGET may be used as the target
   for computing one of EXP's operands. IGNORE is nonzero if the value
   is to be ignored.  */
rtx
pdp10_expand_builtin (exp, target, subtarget, mode, ignore)
     tree exp;
     rtx target;
     rtx subtarget ATTRIBUTE_UNUSED;
     enum machine_mode mode ATTRIBUTE_UNUSED;
     int ignore ATTRIBUTE_UNUSED;
{
  tree arglist = TREE_OPERAND (exp, 1);
  enum pdp10_builtins code =
    DECL_FUNCTION_CODE (TREE_OPERAND (TREE_OPERAND (exp, 0), 0));

  switch (code)
    {
    case PDP10_BUILTIN_JFFO:
      return expand_builtin_jffo (arglist, target);
    case PDP10_BUILTIN_FSC:
    case PDP10_BUILTIN_DFSC:
      return expand_builtin_fsc (arglist, target, code);
    default:
      abort ();
    }
}

tree
pdp10_build_va_list_type ()
{
  return build_pointer_type (integer_type_node);
}

void
pdp10_expand_builtin_va_start (stdarg_p, valist, nextarg)
     int stdarg_p;
     tree valist;
     rtx nextarg;
{
  int offset = 0;
  tree t;

  if (current_function_varargs)
    /* When current_function_varargs goes away, remember that stdarg_p
       is always true.  */
    ;

  if (!stdarg_p)
    offset -= 1;

  nextarg = plus_constant (nextarg, offset);
  t = build (MODIFY_EXPR, TREE_TYPE (valist), valist,
	     make_tree (integer_type_node, nextarg));
  TREE_SIDE_EFFECTS (t) = 1;

  expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);
}

rtx
pdp10_expand_builtin_va_arg (valist, type)
     tree valist, type;
{
  tree addr_tree, t;
  HOST_WIDE_INT align;
  HOST_WIDE_INT rounded_size;
  rtx addr;

  /* Compute the rounded size of the type.  */
  align = PARM_BOUNDARY / BITS_PER_UNIT;
  rounded_size = (((int_size_in_bytes (type) + align - 1) / align) * align);

  /* Get AP.  */
  addr_tree = valist;

  /* Compute new value for AP.  */
  t = build (MODIFY_EXPR, TREE_TYPE (valist), valist,
	     build (PLUS_EXPR, TREE_TYPE (valist), valist,
		    build_int_2 (-rounded_size / 4, 0)));
  TREE_SIDE_EFFECTS (t) = 1;
  expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);

  if (1) /* FIXME: was PAD_VARARGS_DOWN) */
    {
      /* Small args are padded downward.  */

      HOST_WIDE_INT adj
	= rounded_size > align ? rounded_size : int_size_in_bytes (type);

      addr_tree = build (PLUS_EXPR, TREE_TYPE (addr_tree), addr_tree,
			 build_int_2 (rounded_size - adj, 0));
    }

  addr = expand_expr (addr_tree, NULL_RTX, Pmode, EXPAND_NORMAL);
  addr = copy_to_reg (addr);

  return addr;
}


/**********************************************************************

	Support for MACRO, MIDAS, and FAIL

**********************************************************************/

#ifndef __PDP10_GAS_H__

static int	asciz_ok PARAMS ((const char *ptr, int len));
static void	macro_output_ascii_bytes PARAMS ((FILE *stream,
						  unsigned const char *ptr,
						  int len,
						  int byte_size));
static bool	maybe_output_extern PARAMS ((struct hash_entry *he,
					     hash_table_key ptr));
static struct hash_entry *
		extern_symbol_newfunc PARAMS ((struct hash_entry *he,
					       struct hash_table *table,
					       hash_table_key string));
static void	init_extern_symbol_table PARAMS ((void));
static const char *
		section_name;

struct extern_symbol_entry
{
  struct hash_entry he;
  const char *name;
  int flags;
};

static struct hash_table extern_symbol_table;

/* Called by the ASM_FILE_START macro.

   Outputs to the stdio stream STREAM some appropriate text to go at
   the start of an assembler file.  */
void
macro_file_start (file)
     FILE *file;
{
  pdp10_build_machine_types ();

  if (main_input_filename)
    {
      const char *title = main_input_filename + strlen (main_input_filename);
      while (title != main_input_filename && !IS_DIR_SEPARATOR (title[-1]))
	title--;
      fputs ("\tTITLE\t", file);
      while (*title && memcmp (title, ".c", 3) != 0)
	{
	  char c = *title++;
	  if (ISDIGIT (c) || ISALPHA (c))
	    fputc (c, file);
	}
      fputc ('\n', file);
    }

  if (TARGET_KA10up)
    {
      fputs ("\t.DIRECTIVE\t", file);
      fputs (TARGET_KL10up ? (TARGET_EXTENDED ? "KL10" : "KS10,KL10") :
	     TARGET_KI10up ? "KI10,KS10,KL10" :
			     "KA10,KI10,KS10,KL10",
	     file);
      fputs ("\n", file);
    }

  fputc ('\n', file);

  if (TARGET_EXTENDED)
    {
      fputs ("DEFINE OWGBP (PS,Y)<<PS>B5+<Y>*%ONE>\t", file);
      fputs (ASM_COMMENT_START, file);
      fputs (" one-word global byte pointer\n", file);
      fputs ("DEFINE GIW (Y)<<Y>*%ONE>\t", file);
      fputs (ASM_COMMENT_START, file);
      fputs (" global indirect word\n", file);
#if XKL_STUFF
      if (debug_info_level != DINFO_LEVEL_NONE)
	{
	  /* These used to say ...<Y>*%ONE...  */
	  fputs ("DEFINE DBE (Y)<<Y>!77B5>\n", file);
	  fputs ("DEFINE DBL (Y)<<Y>!44B5>\n", file);
	}
#endif
      fputs ("DEFINE %EXIND (I,X,Y)<<I>B1+<X>B5+<<Y>*%ONE>>\t", file);
      fputs (ASM_COMMENT_START, file);
      fputs (" extended format indirect word\n", file);
      fputs (TEXT_SECTION_ASM_OP, file);
      fputc ('\n', file);
    }
  else
    fputs ("\tEXTERN %BADL6,%BADL7,%BADL8,%BADL9,%BADLH\n\n", file);
}

static bool
maybe_output_extern (he, ptr)
     struct hash_entry *he;
     hash_table_key ptr ATTRIBUTE_UNUSED;
{
  struct extern_symbol_entry *entry = (struct extern_symbol_entry *) he;
  if (entry->flags == (PDP10_SYMBOL_USED | PDP10_SYMBOL_EXTERN))
    {
      fputs ("\tEXTERN\t", asm_out_file);
      assemble_name (asm_out_file, entry->name);
      fputc ('\n', asm_out_file);
    }
  return true;
}

/* Called by the ASM_FILE_END macro.

   Outputs to the stdio stream STREAM some appropriate text to go at
   the end of an assembler file.  */
void
macro_file_end (file)
     FILE *file;
{
  int i;

  ASM_SECTION_END (file);
  fputc ('\n', file);

  if (pdp10_giw)
    fputs ("\tEXTERN\t%ONE\n", file);

  for (i = 6; i <= 18; i++)
    if (used_byte_subtraction_table[i])
      fprintf (file, "\tEXTERN\t%%BAD%c%c\n",
	       TARGET_EXTENDED ? 'X' : 'L', i == 18 ? 'H' : '0' + i);

  hash_traverse (&extern_symbol_table, maybe_output_extern, NULL);
  fputc ('\n', file);

  fprintf (file, "%s expanded load unsigned 16  left: %4d\n",
	   ASM_COMMENT_START, expand_extzv_16_0);
  fprintf (file, "%s output   load unsigned 16  left: %4d\n",
	   ASM_COMMENT_START, output_extzv_16_0);
  fprintf (file, "%s expanded load unsigned 16 right: %4d\n",
	   ASM_COMMENT_START, expand_extzv_16_1);
  fprintf (file, "%s output   load unsigned 16 right: %4d\n",
	   ASM_COMMENT_START, output_extzv_16_1);
  fprintf (file, "%s expanded load unsigned 32:       %4d\n",
	   ASM_COMMENT_START, expand_extzv_32);
  fprintf (file, "%s output   load unsigned 32:       %4d\n",
	   ASM_COMMENT_START, output_extzv_32);
  fprintf (file, "%s output   load   signed  9:       %4d\n",
	   ASM_COMMENT_START, pdp10_output_ldbe_9);
  fprintf (file, "%s expanded load   signed 16  left: %4d\n",
	   ASM_COMMENT_START, expand_extv_16_0);
  fprintf (file, "%s output   load   signed 16  left: %4d\n",
	   ASM_COMMENT_START, output_extv_16_0);
  fprintf (file, "%s expanded load   signed 16 right: %4d\n",
	   ASM_COMMENT_START, expand_extv_16_1);
  fprintf (file, "%s output   load   signed 16 right: %4d\n",
	   ASM_COMMENT_START, output_extv_16_1);
  fprintf (file, "%s expanded load   signed 32:       %4d\n",
	   ASM_COMMENT_START, expand_extv_32);
  fprintf (file, "%s output   load   signed 32:       %4d\n",
	   ASM_COMMENT_START, output_extv_32);
  fprintf (file, "%s expanded load   signed:          %4d\n",
	   ASM_COMMENT_START, expand_extv);
  fprintf (file, "%s output   load   signed:          %4d\n",
	   ASM_COMMENT_START, output_extv);
  fprintf (file, "%s expanded ffs:                    %4d\n",
	   ASM_COMMENT_START, pdp10_expand_ffs);
  fputs ("\n\tEND\n", file);
}

void
macro_section_end (stream)
     FILE *stream;
{
  if (TARGET_EXTENDED)
    {
      if (section_name && strncmp (".db", section_name, 3) == 0)
	pdp10_align_with_pad (0400);
      pdp10_output_byte (stream, 0, 0);
      fputs ("\t.ENDPS\n", stream);
    }
  section_name = NULL;
}

/* Print an ASCII (or SIXBIT) string with characters of BYTE_SIZE
   bits, without using the ASCIZ pseudo.  SIXBIT is output if
   BYTE_SIZE is 6.  */
static void
macro_output_ascii_bytes (stream, ptr, len, byte_size)
     FILE *stream;
     const unsigned char *ptr;
     int len;
     int byte_size;
{
  if (len == 0)
    return;

  while (len--)
    {
      int c = *ptr++;
      if (byte_size == 6)
	{
	  c = TOUPPER (c) - 040;
	  if (c < 0 || c > 077)
	    c = 0;
	}
      pdp10_output_byte (stream, GEN_INT (c), byte_size);
      pdp10_byte_count++;
    }
}

/* Return separator character if the string can be output using the
   ASCIZ pseudo, or 0 otherwise.  */
static int
asciz_ok (ptr, len)
     const char *ptr;
     int len;
{
  char *separators = xstrdup ("/$|%!");
  int c, i;
  char *p;

  while (len--)
    {
      c = *ptr++;
      if (c && (p = strchr (separators, c)))
	*p = 'x';
      if (c >= 32 && c <= 126)
	continue;

      if (c == '\r' && len > 0 && *ptr == '\n')
	{
	  ptr++;
	  len--;
	  continue;
	}

      if (c == 0 && len == 0)
	continue;

      free (separators);
      return 0;
    }

  c = 0;
  for (i = 0; i < (int) strlen (separators); i++)
    if (separators[i] != 'x')
      {
	c = separators[i];
	break;
      }

  free (separators);
  return c;
}

/* Called by the ASM_OUTPUT_ASCII macro.

   Outputs to the stdio stream STREAM an assembler instruction to
   assemble a string constant.  */
void
macro_output_ascii (stream, ptr, len)
     FILE *stream;
     const char *ptr;
     int len;
{
  int byte_size = pdp10_string_literal_byte_size;
  int separator;
  int c;

  /* If the ASCIZ pseudo can't be used, output the string byte by byte.  */
  if (byte_size != 7 || (separator = asciz_ok (ptr, len)) == 0)
    {
      /* Note that 6-bit characters are converted to SIXBIT.  */
      macro_output_ascii_bytes (stream, (unsigned char *)ptr, len, byte_size);
      return;
    }

  fputs ("\tASCIZ\t", stream);
  fputc (separator, stream);

  while (len--)
    {
      c = *ptr++;
      if (c == '\r')
	{
	  len--;
	  ptr++;
	  fputc ('\n', stream);
	}
      else if (c == 0 && len == 0)
	;
      else
	fputc (c, stream);
    }

  fputc (separator, stream);
  fputc ('\n', stream);
}

/* Static variables for pdp10_output_byte.  */
static int bits_in_word;
static rtx last_output_byte = NULL_RTX;
static int last_byte_size;

/* Called primarily by the ASM_OUTPUT_CHAR and ASM_OUTPUT_SHORT
   macros, but also from many other places.

   If EXP is NULL, then output a newline; the next byte will be put in
   a new word.  Otherwise, output the value of EXP as a byte of
   BYTE_SIZE bits.  If the byte doesn't fit in the same word as the
   previous byte, output a newline and put the byte in a new word.  */
void
pdp10_output_byte (stream, exp, byte_size)
     FILE *stream;
     rtx exp;
     int byte_size;
{
  int mask;

  if (last_output_byte == NULL_RTX)
    {
      if (exp == NULL_RTX || byte_size == 0)
	return;
      fprintf (stream, "\tBYTE (%d)", byte_size);
      mask = (1 << byte_size) - 1;
      /*pdp10_print_integer (stream, INTVAL (exp) & mask);*/
      pdp10_print_number (stream, INTVAL (exp) & mask, 1, byte_size, 0);
      bits_in_word = byte_size;
    }
  else if (exp)
    {
      if (bits_in_word + byte_size > 36)
	{
	  pdp10_output_byte (stream, 0, 0);
	  pdp10_output_byte (stream, exp, byte_size);
	  return;
	}
      if (byte_size == last_byte_size)
	fputc (',', stream);
      else
	fprintf (stream, "(%d)", byte_size);
      mask = (1 << byte_size) - 1;
      /*pdp10_print_integer (stream, INTVAL (exp) & mask);*/
      pdp10_print_number (stream, INTVAL (exp) & mask, 1, byte_size, 0);
      bits_in_word += byte_size;
    }
  else
    {
      extern char *dw2_assemble_string[5];
      extern int dw2_assemble_integer_size[5];
      extern rtx dw2_assemble_integer_x[5];
      extern int dw2_assemble_integer_n;
      extern HOST_WIDE_INT dw2_assemble_uleb128[5];
      int i;

      bits_in_word = 0;
      fputc ('\n', stream);

      for (i = 0; i < dw2_assemble_integer_n; i++)
	{
	  fprintf (stream, "\t%s ", ASM_COMMENT_START);
	  if (dw2_assemble_integer_x[i])
	    {
	      if (dw2_assemble_integer_size[i] > 1)
		fprintf (stream, ".%dbyte ", dw2_assemble_integer_size[i]);
	      else
		fputs (".byte ", stream);
	      if (GET_CODE (dw2_assemble_integer_x[i]) == CONST_INT)
		fprintf (stream, "0x%llx", INTVAL (dw2_assemble_integer_x[i]));
	      else
		pdp10_print_operand (asm_out_file, dw2_assemble_integer_x[i], 0);
	    }
	  else if (dw2_assemble_string[i])
	    fprintf (stream, ".string \"%s\"", dw2_assemble_string[i]);
	  else
	    {
	      fputs (".uleb128 ", stream);
	      fprintf (stream, HOST_WIDE_INT_PRINT_HEX,
		       dw2_assemble_uleb128[i]);
	    }

	  fputc ('\n', stream);
	}

      dw2_assemble_integer_n = 0;
      memset (dw2_assemble_integer_x, 0, sizeof dw2_assemble_integer_x);
      memset (dw2_assemble_string, 0, sizeof dw2_assemble_string);
    }

  last_byte_size = byte_size;
  last_output_byte = exp;
}

int
pdp10_align_with_pad (pad)
     int pad;
{
  rtx x = GEN_INT (pad);
  int n = pdp10_byte_count;

  if (bits_in_word > 0)
    while (bits_in_word < 36)
      {
	pdp10_output_byte (asm_out_file, x, 9);
	/* Do not count pad bytes by increasing pdp10_byte_count.  */
      }

  return pdp10_byte_count - n;
}

/* Called by the ASM_GLOBALIZE_LABEL macro.

   Outputs to the stdio stream STREAM some commands that will make the
   label NAME global; that is, available for reference from other
   files.  */
void
macro_globalize_label (stream, name)
     FILE *stream;
     const char *name;
{
  pdp10_output_byte (stream, 0, 0);
  fputs ("\tENTRY\t", stream);
  assemble_name (stream, name);
  fputc ('\n', stream);
}

void
macro_output_def (stream, label, value)
     FILE *stream;
     const char *label;
     const char *value;
{
  assemble_name (stream, label);
  fputs ("=", stream);
  assemble_name (stream, value);
  fprintf (stream, "\n");

  pdp10_extern_symbol (label, PDP10_SYMBOL_DEFINED);
}

/* Switch to an arbitrary section NAME with attributes as specified by
   FLAGS.  */
static void
pdp10_asm_named_section	(name, flags)
     const char *name;
     unsigned int flags;
{
  section_name = name;

  fputs ("\t.PSECT\t", asm_out_file);
  fputs (name, asm_out_file);
  if (flags & SECTION_WRITE)
    fputs ("/rwrite", asm_out_file);
  else
    fputs ("/ronly", asm_out_file);

  if (flags & (SECTION_CODE | SECTION_WRITE | SECTION_DEBUG | SECTION_BSS
	       | SECTION_SMALL | SECTION_LINKONCE | SECTION_FORGET))
    {
      fputs ("\t;", asm_out_file);
      if (flags & SECTION_CODE)
	fputs (" code", asm_out_file);
      if (flags & SECTION_DEBUG)
	fputs (" debug", asm_out_file);
      if (flags & SECTION_LINKONCE)
	fputs (" linkonce", asm_out_file);
      if (flags & SECTION_SMALL)
	fputs (" small", asm_out_file);
      if (flags & SECTION_BSS)
	fputs (" bss", asm_out_file);
      if (flags & SECTION_FORGET)
	fputs (" forget", asm_out_file);
    }

  fputc ('\n', asm_out_file);
}

/* Create a new entry for the extern_symbol_table hash table.  */
static struct hash_entry *
extern_symbol_newfunc (he, table, string)
     struct hash_entry *he;
     struct hash_table *table;
     hash_table_key string ATTRIBUTE_UNUSED;
{
  struct extern_symbol_entry *entry = (struct extern_symbol_entry *) he;

  if (entry == NULL)
    {
      entry = ((struct extern_symbol_entry *)
	       hash_allocate (table, sizeof (struct extern_symbol_entry)));
      if (entry == NULL)
	return NULL;
    }

  entry->name = NULL;
  entry->flags = 0;
  return (struct hash_entry *) entry;
}

static void
init_extern_symbol_table ()
{
  static int inited = 0;

  if (!inited)
    {
      hash_table_init (&extern_symbol_table,
		       extern_symbol_newfunc,
		       string_hash,
		       string_compare);
      inited = 1;
    }
}

void
pdp10_extern_symbol (name, flags)
     const char *name;
     int flags;
{
  struct extern_symbol_entry *entry;

  init_extern_symbol_table ();

  entry = ((struct extern_symbol_entry *)
	   hash_lookup (&extern_symbol_table, (const hash_table_key) name, 
			true, string_copy));
  if (!entry->name)
    entry->name = xstrdup (name);
  entry->flags |= flags;
}

int pdp10_byte_count = 0;
int pdp10_bits_per_unit = BITS_PER_UNIT;

static bool
pdp10_assemble_integer (x, size, aligned_p)
     rtx x;
     unsigned int size;
     int aligned_p;
{
  if (!aligned_p && size > 1)
    return false;

  switch (size)
    {
    case 1:
    case 2:
      pdp10_output_byte (asm_out_file, x, pdp10_bits_per_unit * size);
      break;
    case 4:
    case 8:
      if (aligned_p)
	{
	  pdp10_output_byte (asm_out_file, 0, 0);
	  fputc ('\t', asm_out_file);
	  pdp10_print_operand (asm_out_file, x,
			       (size == 8 ? 'D' : 0) | PRINT_OP_GIW);
	  fputc ('\n', asm_out_file);
	  break;
	}
    default:
      return false;
    }

  pdp10_byte_count += size;

  return true;
}

#endif /* !__PDP10_GAS_H__ */


/**********************************************************************

	Optimizations

**********************************************************************/

static int	single_insn PARAMS ((rtx insn, rtx label1, rtx label2));

/* Helper function for pdp10_output_range_compare.  Examines whether a
   single instruction is suitable to replace the JRST in the range
   comparision.  */
static int
single_insn (insn, label1, label2)
     rtx insn;
     rtx label1;
     rtx label2;
{
  rtx jump;

  if (get_attr_length (insn) != 1
      || get_attr_skip (insn) == SKIP_YES)
    return 0;

  jump = next_nonnote_insn (insn);
  return (jump == NULL_RTX
	  || (GET_CODE (jump) == BARRIER && !simplejump_p (insn))
	  || (GET_CODE (jump) == CODE_LABEL && jump == label2)
	  || (simplejump_p (jump)
	      && label1 == XEXP (SET_SRC (PATTERN (jump)), 0)));
}

/* Output instructions to conditionally jump to operands[3] depending
   on the result of a comparison of the register in operands[0]
   against a range represented by operands[1] and operands[2].  Return
   an empty string.

   The RTL insn being output looks like this:
   
	(set (pc)
	     (if_then_else
	      (ge (xor:SI (plus:SI
			   (match_operand:SI 1 "register_operand" "r")
			   (match_operand:SI 2 "const_int_operand" "i"))
			  (const_int SIGNBIT))
		  (match_operand:SI 3 "const_int_operand" "i"))
	      (label_ref (match_operand 4 "" ""))
	      (pc)))

   And is actually a combination of three insns:

	;; Subtract the low end of the range from operands[0].
	(set (reg:SI X)
	     (plus:SI (match_operand:SI 1 "register_operand" "r")
		      (match_operand:SI 2 "const_int_operand" "i")))
	
	;; Complement the sign bit in preparation for an
	;; unsigned comparison.
	(set (reg:SI Y)
	     (xor:SI (reg:SI X) (const_int SIGNBIT)))
	
	;; Compare the result against the high end of the range,
	;; and jump if it's larger or equal.
	(set (pc)
	     (if_then_else
	      (ge (reg:SI Y)
		  (match_operand:SI 3 "const_int_operand" "i"))
	      (label_ref (match_operand 4 "" ""))
	      (pc)))

   If the comparison if reversed (jump if operands[1] is inside the
   range), the comparison is "less than" instead, and TRNA is nonzero.

   The output looks something like this:

	CAIL op1,-op2			; skip if op1 less than low end
	 CAIL op1,op3^SIGNBIT - op2	; skip if op1 less than high end
	  JRST op4			; jump

   CAML may be used instead of CAIL if necessary.  A TNRA is output
   last if the comparison should be inverted.  If the JRST jump target
   is just one instruction, the JRST may be replaced by that
   instruction.  */
const char *
pdp10_output_range_compare (insn, ops, trna)
     rtx insn;
     rtx *ops;
     int trna;
{
  rtx jump_target, label;
  HOST_WIDE_INT op2;
  rtx operands[5];

  memcpy (operands, ops, sizeof operands);

  /* Compare against the low end of the range.  */
  op2 = INTVAL (operands[2]);
  if (GET_CODE (operands[1]) == MEM)
    {
      if (op2 != 0)
	abort ();
      output_asm_insn ("skipl %0,%1", operands);
    }
  else
    {
      operands[2] = gen_int_mode (-op2, SImode);
      if (INTVAL (operands[2]) & ~HWINT (0777777))
	output_asm_insn ("caml %0,[%2]", operands);
      else
	output_asm_insn ("cail %0,%2", operands);
    }

  /* Compare against the high end of the range.  */
  operands[3] = gen_int_mode ((INTVAL (operands[3]) ^ SIGN_BIT) - op2, SImode);
  if (INTVAL (operands[3]) & ~HWINT (0777777))
    output_asm_insn ("%_caml %0,[%3]", operands);
  else
    output_asm_insn ("%_cail %0,%3", operands);

  if (trna)
    {
      /* Output a TRNA instrcution unless the next instruction is a
	 single instruction, in which case the comparison is reversed
	 (by not emitting the TRNA) and no jump instruction is output.  */

      rtx next_insn = next_real_insn (insn);

      if (!single_insn (next_insn, NULL_RTX, operands[4]))
	output_asm_insn ("%_%_trna", operands);
      else
	{
	  if (LABEL_NUSES (operands[4]) == 1)
	    INSN_DELETED_P (operands[4]) = 1;
	  pdp10_opcode_indentation = 2;
	  return "";
	}
    }

  label = next_nonnote_insn (insn);
  jump_target = next_nonnote_insn (operands[4]);

  /* Ouput the JRST instruction unless the jump target is a single
     instruction (and perhaps a jump back).  If so, replace the JRST
     by that instruction.  */
  if (LABEL_NUSES (operands[4]) == 1
      && single_insn (jump_target, label, NULL_RTX))
    {
      rtx jump_back = next_nonnote_insn (jump_target);

      if (LABEL_NUSES (operands[4]) == 1)
	INSN_DELETED_P (operands[4]) = 1;
      if (GET_CODE (jump_back) != BARRIER)
	{
	  INSN_DELETED_P (jump_back) = 1;
	  if (LABEL_NUSES (label) == 1)
	    INSN_DELETED_P (label) = 1;
	}
      pdp10_opcode_indentation = 2 + trna;
      final_scan_insn (jump_target, asm_out_file, optimize, 0, 0);
      INSN_DELETED_P (jump_target) = 1;
    }
  else
    /* TODO: pdp10_output_jrst (operands[4]); */
    output_asm_insn ("%_%_%_jrst %F4" + (trna ? 0 : 2), operands);

  return "";
}


/**********************************************************************

	Data Movement

**********************************************************************/

static void	normalize_bitfield_mem PARAMS ((rtx *x, int *pos));
static rtx	pdp10_find_last_value PARAMS ((rtx x, rtx *pinsn));
static const char *
		output_halfword_move PARAMS ((rtx reg, rtx x, int reg_right,
					      int mem_right, int store,
					      int extend, int sign));
static const char *
		output_bitfield_move PARAMS ((FILE *stream, rtx insn,
					      int store, int sign, rtx reg,
					      rtx x, int len, int bit));
static int	use_xblt PARAMS ((rtx destination, rtx source, rtx length));
static int	expand_blt PARAMS ((rtx destination, rtx source, rtx length));
static int	expand_xblt PARAMS ((rtx destination, rtx source, rtx length));
static int	expand_blt_clear PARAMS ((rtx destination, rtx length));
static int	expand_xblt_clear PARAMS ((rtx destination, rtx length));
#if TARGET_STRING
static int	expand_movslj PARAMS ((rtx dest, rtx source, rtx length));
static int	expand_movst_clear PARAMS ((rtx destination, rtx length));
#endif

/* Return the offset from the base pointer.  */
int
pdp10_pointer_offset (x)
     rtx x;
{
  /* This function must be prepared to handle anything
     GO_IF_LEGITIMATE_ADDRESS says is a valid address.  */

  switch (GET_CODE (x))
    {
    case SYMBOL_REF:
    case LABEL_REF:
    case REG:
    case PRE_INC:
    case MEM:
      return 0;
    case CONST_INT:
      return INTVAL (x) & ~((HOST_WIDE_INT)(UNITS_PER_WORD - 1));
    case CONST:
      x = XEXP (x, 0);
      if (GET_CODE (x) != PLUS)
	abort ();
      /* Fall through.  */
    case PLUS:
      if (GET_CODE (XEXP (x, 1)) != CONST_INT)
	abort ();
      return INTVAL (XEXP (x, 1));
    default:
      abort ();
    }
}

/* Return or output a single instruction to load an unsigned byte into
   a register.  Called to output a (I)LDB(I) insn.  */
const char *
pdp10_output_load_unsigned_byte (insn, operands)
     rtx insn;
     rtx *operands;
{
  int pos = -1;

  if (GET_CODE (operands[1]) == MEM
      && MEM_ALIGN (operands[1]) >= BITS_PER_WORD
      && !MEM_SCALAR_P (operands[1]))
    pos = 0;

  return output_bitfield_move (asm_out_file, insn, 0, 0, operands[0],
			       operands[1], 9, pos);
}

/* Return or output a single instruction to load a signed byte into a
   register.  Called to output a (I)LDBE(I) insn.  */
const char *
pdp10_output_load_signed_byte (insn, operands)
     rtx insn;
     rtx *operands;
{
  return output_bitfield_move (asm_out_file, insn, 0, 1, operands[0],
			       operands[1], 9, -1);
}

/* Return or output a single instruction to store a byte from a
   register.  Called to output an (I)DPB(I) insn.  */
const char *
pdp10_output_store_byte (insn, operands)
     rtx insn;
     rtx *operands;
{
  int pos = -1;
  int size = 9;

  if (GET_CODE (operands[0]) == MEM)
    {
      rtx pointer, offset;
      int ps, p, s;

      if (GET_CODE (XEXP (operands[0], 0)) == CONST
	  && GET_CODE (pointer = XEXP (XEXP (operands[0], 0), 0)) == PLUS
	  && GET_CODE (offset = XEXP (pointer, 1)) == CONST_INT
	  && (ps = ((INTVAL (offset) >> 24) & 07777)) != 0
	  && decode_ps (ps, &p, &s) != -1)
	{
	  pos = p;
	  size = s;
	}
      else if (MEM_ALIGN (operands[0]) == BITS_PER_WORD)
	pos = 0;
    }

  return output_bitfield_move (asm_out_file, insn, 1, 0, operands[1],
			       operands[0], size, pos);
}

#if 0
const char *
pdp10_output_halfword_move (insn)
     rtx insn;
{
  rtx pattern = INSN_PATTERN (insn);
  rtx dest = SET_DEST (pattern);
  rtx src = SET_SRC (pattern);
  int dest_right, src_right;
  char *s, format[100];

  switch (GET_CODE (dest))
    {
    case ZERO_EXTRACT:
      dest_right = INTVAL (XEXP (dest, 2)) == 18;
      dest = XEXP (dest, 0);
      break;
    case SUBREG:
      dest_right = true;
      dest = XEXP (dest, 0);
      break;
    default:
      abort ();
    }

  switch (GET_CODE (dest))
    {
    case ZERO_EXTRACT:
    case SIGN_EXTRACT:
      src_right = INTVAL (XEXP (src, 2)) == 18;
      src = XEXP (src, 0);
      break;
    case SUBREG:
    case LSHIFT:
    case ASHIFT:
      src_right = false;
      src = XEXP (src, 0);
      break;
    case AND:
      src_right = true;
      src = XEXP (src, 0);
      break;
    default:
      abort ();
    }

  s = format;
  *s = 'h';
  *++s = src_right ? 'r' : 'l';
  *++s = dest_right ? 'r' : 'l';
  if (GET_CODE (dest) == MEM)
    *++s = 'm';
  *++s = ' ';
  s++;
  if (GET_CODE (dest) == MEM)
    strcpy (s, "%W1,%0");
  else /*if (GET_CODE (src) == MEM)*/
    strcpy (s, "%0,%W1");
  /*else
    strcpy (s, "%0,%1");*/

  return format;
}
#endif

/* Return or output a single half-word instruction.  REG is the
   accumulator operand and X is the memory operand.  REG_RIGHT should
   be nonzero if the right half of the accumulator is to be used.
   MEM_RIGHT should should be nonzero if X is an accumulator and the
   right half should be used.  STORE should be 1 for store
   instructions and 0 for load instrucitons.  EXTEND should be 1 for
   sign- or zero-extending loads, and 0 for non-extending loads.  SIGN
   should be 1 for sign- extending loads, and 0 for zero-extending
   loads.  */
static const char *
output_halfword_move (reg, x, reg_right, mem_right, store, extend, sign)
     rtx reg;
     rtx x;
     int reg_right;
     int mem_right;
     int store;
     int extend;
     int sign;
{
  rtx operands[2];
  char format[20];
  char *p = format;

  *p++ = 'h';
  if (store)
    {
      *p++ = reg_right ? 'r' : 'l';
      *p++ = mem_right ? 'r' : 'l';
    }
  else
    {
      *p++ = mem_right ? 'r' : 'l';
      *p++ = reg_right ? 'r' : 'l';
    }
  if (extend)
    *p++ = sign ? 'e' : 'z';
  if (store)
    *p++ = 'm';
  *p++ = ' ';
  *p = 0;

  if (REG_P (x))
    strcat (format, "%0,%1");
  else
    strcat (format, "%0,%W1");

  operands[0] = reg;
  operands[1] = x;
  output_asm_insn (format, operands);

  return "";
}

/* Return or output a single instruction to move a half-word.  Called
   to output a movhi pattern.  SIGN_EXTEND should be 1 if this is a
   sign-extending load.  */
const char *
pdp10_output_movhi (operands, sign_extend)
     rtx *operands;
     int sign_extend;
{
  int store = GET_CODE (operands[0]) == MEM;
  rtx x = store ? operands[0] : operands[1];
  rtx address = NULL_RTX;

  if (GET_CODE (x) == MEM)
    address = XEXP (x, 0);

  if (address && GET_CODE (address) == PRE_INC)
    {
      if (store)
	return "idpb %1,%0";
      else
	return sign_extend ? "extend %0,[ildbe %1]" : "ildb %0,%1";
    }
  else if (address && GET_CODE (address) == POST_INC)
    {
      if (store)
	return "extend %1,[dpbi %0]%; movhi";
      else
	return sign_extend ? "extend %0,[ldbei %1]" : "extend %0,[ldbi %1]";
    }
  else if (!address
	   || MEM_ALIGN (x) >= BITS_PER_WORD
	   || GET_CODE (address) == CONST)
    {
      rtx reg = store ? operands[1] : operands[0];
      int mem_right = 1;
      mem_right = 0;
      if (GET_CODE (x) == MEM
	  && GET_CODE (XEXP (x, 0)) == CONST
	  && GET_CODE (XEXP (XEXP (x, 0), 0)) == PLUS
	  && GET_CODE (XEXP (XEXP (XEXP (x, 0), 0), 1)) == CONST_INT)
	{
	  int ps = (INTVAL (XEXP (XEXP (XEXP (x, 0), 0), 1)) >> 30) & 077;
	  mem_right = ps == 076;
	}
      return output_halfword_move (reg, x, 1, mem_right,
				   store, !store, sign_extend);
    }
  else
    {
      if (store)
	return "dpb %1,%0%; movhi";
      else
	return sign_extend ? "extend %0,[ldbe %1]" : "ldb %0,%1";
    }
}

/* Expand an unsigned bit-field load.  Called to expand the "extzv"
   pattern in some special cases.  Return zero if nothing special has
   to be done, nonzero otherwise.  */
int
pdp10_expand_extzv (operands)
     rtx *operands;
{
  rtx op1 = copy_rtx (operands[1]);
  int len = INTVAL (operands[2]);
  int pos = INTVAL (operands[3]);

  PUT_MODE (op1, SImode);

  if (pos == 0) /* && len == 32) */
    {
      rtx temp = gen_reg_rtx (SImode);
      if (len == 32)
	expand_extzv_32++;
      emit_move_insn (temp, op1);
      emit_insn (gen_lshrsi3 (temp, temp, GEN_INT (BITS_PER_WORD - len)));
      emit_move_insn (operands[0], temp);
      return 1;
    }
#if 0
  /* TODO: is this beneficial?  */
  else if (pos + len == BITS_PER_WORD)
    {
      HOST_WIDE_INT mask = (1 << len) - 1;
      emit_move_insn (operands[0], op1);
      emit_insn (gen_andsi3 (operands[0], operands[0],
			     gen_int_mode (mask, SImode)));
      return 1;
    }
#endif

  return 0;
}

/* Expand a signed bit-field load.  Called to expand an "extv" pattern
   if the LDBE instruction is not available.  */
int
pdp10_expand_extv (operands)
     rtx *operands;
{
  int len = INTVAL (operands[2]);
  int pos = INTVAL (operands[3]);
  rtx op1 = copy_rtx (operands[1]);

  if (len == 18 && (pos == 0 || pos == 18))
    return 0;

  if (GET_CODE (op1) == MEM)
    normalize_bitfield_mem (&op1, &pos);
  PUT_MODE (op1, SImode);

  if (len == 32 && pos == 0)
    {
      expand_extv_32++;
      emit_move_insn (operands[0], op1);
      emit_insn (gen_ashrsi3 (operands[0], operands[0], GEN_INT (4)));
      return 1;
    }

  return 0;
}

/* *X is a (9-bit) byte in memory and *POS is the first bit of a
   bitfield starting in *X.  Modify *X and *POS such that *X is word
   aligned and *POS specifies the first bit, in the word that starts
   at *X, of the bitfield.  */
static void
normalize_bitfield_mem (rtx *x, int *pos)
{
  rtx y;

  if (GET_CODE (*x) != MEM)
    return;

  y = XEXP (*x, 0);
  if (GET_CODE (y) == CONST)
    y = XEXP (y, 0);

  if (GET_CODE (y) == PLUS)
    {
      HOST_WIDE_INT i = INTVAL (XEXP (y, 1));
      HOST_WIDE_INT j = i % UNITS_PER_WORD;

      if (j != 0)
	{
	  if (j < 0)
	    j += UNITS_PER_WORD;

	  *x = gen_rtx_MEM (GET_MODE (*x), 
			    plus_constant (XEXP (y, 0), i - j));
	  *pos += BITS_PER_UNIT * j;
	}
    }
}

/* This is an adaption of find_last_vaue in rtanal.c.  Return the last
   thing that X was assigned from before *PINSN.  If we found an
   assignment, update *PINSN to point to it.  */
static rtx
pdp10_find_last_value (x, pinsn)
     rtx x;
     rtx *pinsn;
{
  rtx p;

  for (p = PREV_INSN (*pinsn); p && GET_CODE (p) != CODE_LABEL;
       p = PREV_INSN (p))
    if (INSN_P (p))
      {
	rtx set = single_set (p);
	rtx note = find_reg_note (p, REG_EQUAL, NULL_RTX);

	if (set && REG_P (SET_DEST (set))
	    && REGNO (SET_DEST (set)) == REGNO (x))
	  {
	    rtx src = SET_SRC (set);

	    if (note && GET_CODE (XEXP (note, 0)) != EXPR_LIST)
	      src = XEXP (note, 0);

	    *pinsn = p;
	    return src;
	  }
	  
	/* If set in non-simple way, we don't have a value.  */
	if (reg_set_p (x, p))
	  break;
      }

  return NULL_RTX;
}

#define INTVAL_OR_ELSE(X, Y)						\
  (GET_CODE (X) == CONST_INT ? INTVAL (X)				\
   : GET_CODE (X) == NEG						\
     && GET_CODE (XEXP ((X), 0)) == CONST_INT ? -INTVAL (XEXP ((X), 0))	\
   : (Y))

/* Return nonzero iff the last value X was set to before INSN had
   BITS zero-extended bits.  */
int
zero_extended_p (rtx x, int bits, rtx insn)
{
  HOST_WIDE_INT w;
  int i, n;

  do
    {
      while (REG_P (x))
	{
	  x = pdp10_find_last_value (x, &insn);
	  if (x == NULL_RTX)
	    return 0;
	}

      switch (GET_CODE (x))
	{
	case LSHIFTRT:
	  n = INTVAL_OR_ELSE (XEXP (x, 1), 0);
	  x = XEXP (x, 0);
	  break;
	case ZERO_EXTRACT:
	  n = BITS_PER_WORD - INTVAL_OR_ELSE (XEXP (x, 1), BITS_PER_WORD);
	  x = NULL_RTX;
	  break;
	case AND:
	  w = INTVAL_OR_ELSE (XEXP (x, 1), HWINT (-1));
	  for (i = BITS_PER_WORD - 1; i >= 0; i--)
	    {
	      if (w & ((HWINT (1)) << i))
		break;
	    }
	  n = BITS_PER_WORD - i;
	  x = XEXP (x, 0);
	  break;
	case SUBREG:
	  n = BITS_PER_WORD - GET_MODE_BITSIZE (GET_MODE (x));
	  break;
#if 1
	case MEM:
	  /* A QI could be as small as 6 bits.  */
	  n = GET_MODE (x) == QImode ? 30
	    : BITS_PER_WORD - GET_MODE_BITSIZE (GET_MODE (x));
	  return n >= bits;
#endif
	case UNSPEC:
	  switch (XINT (x, 1))
	    {
	    case UNSPEC_ADDRESS:
	      return TARGET_EXTENDED ? 0 : 18;
	    }
	  /* Fall through.  */
	default:
	  return 0;
	}

      if (n >= bits)
	return 1;

      bits -= n;
    }
  while (x);

  return 0;
}

/* Return nonzero iff the last value X was set to before INSN had
   BITS sign-extended bits.  */
int
sign_extended_p (rtx x, int bits, rtx insn)
{
  HOST_WIDE_INT w;
  int i, n;

  do
    {
      while (REG_P (x))
	{
	  x = pdp10_find_last_value (x, &insn);
	  if (x == NULL_RTX)
	    return 0;
	}

      switch (GET_CODE (x))
	{
	case ASHIFTRT:
	  n = INTVAL_OR_ELSE (XEXP (x, 1), 0);
	  x = XEXP (x, 0);
	  break;
	case SIGN_EXTRACT:
	  n = BITS_PER_WORD - INTVAL_OR_ELSE (XEXP (x, 1), BITS_PER_WORD);
	  x = NULL_RTX;
	  break;
	case IOR:
	  w = INTVAL_OR_ELSE (XEXP (x, 1), 0);
	  for (i = BITS_PER_WORD - 1; i >= 0; i--)
	    {
	      if (!(w & ((HWINT (1)) << i)))
		break;
	    }
	  n = BITS_PER_WORD - i;
	  x = XEXP (x, 0);
	  break;
	default:
	  return 0;
	}

      if (n >= bits)
	return 1;

      bits -= n;
    }
  while (x);

  return 0;
}

/* Return or output to STREAM a single instruction, INSN, to move a
   bit-field.  If STORE is nonzero, store REG in X, otherwise load X
   into REG.  If SIGN is nonzero, loads should sign-extend, otherwise
   loads should zero-extend.  LEN and POS specifies the length and the
   bit position of the bitfield.  Note that this function will output
   XKL-2 extended instructions if necessary.  */
static const char *
output_bitfield_move (stream, insn, store, sign, reg, x, len, pos)
     FILE *stream;
     rtx insn;
     int store;
     int sign;
     rtx reg;
     rtx x;
     int len;
     int pos;
{
  int extend = sign || (GET_CODE (x) == MEM
			&& GET_CODE (XEXP (x, 0)) == POST_INC);

  if (extend && !TARGET_XKL2)
    {
      debug_rtx (insn);
      abort ();
    }

  /* First of all, can this be output as a half-word move instruction?  */
  if (len == 18 && (pos == 0 || pos == 18))
    {
      int reg_right = 1;
      int mem_right;

      switch (GET_CODE (x))
	{
	case MEM:
	  mem_right = pos == 18;
	  break;
	case REG:
	  mem_right = pos == 18;
	  if (store || GET_CODE (reg) != REG)
	    {
	      rtx tmp = reg; reg = x; x = tmp;
	      reg_right = mem_right;
#ifdef YESTERDAY
	      mem_right = 0;
	      if (GET_CODE (x) == MEM)
		{
		  rtx y = XEXP (x, 0);
		  if (GET_CODE (y) == CONST)
		    y = XEXP (y, 0);
		  if (GET_CODE (y) == PLUS
		      && GET_CODE (XEXP (y, 0)) == SYMBOL_REF
		      && GET_CODE (XEXP (y, 1)) == CONST_INT)
		    {
		      int ps = (INTVAL (XEXP (y, 1)) >> 24) & 07777;
		      mem_right = (ps == 07600 || ps == 00022);
		    }
		}
	      else if (GET_CODE (x) == SUBREG || GET_CODE (x) == ZERO_EXTEND)
		abort ();
#else
	      if (GET_CODE (x) == MEM)
		{
		  if (MEM_OFFSET (x) != 0)
		    mem_right = INTVAL (MEM_OFFSET (x));
		  else
		    mem_right =
		      pdp10_pointer_offset (XEXP (x, 0)) % UNITS_PER_WORD != 0;
		}
	      else
		mem_right = 1;
#endif
	      store = !store && GET_CODE (x) != REG;
	    }
	  break;
	default:
	  abort ();
	}

      return
	output_halfword_move (reg, x, reg_right, mem_right, store, !store,
			      sign);
    }

  fputc ('\t', stream);

  /* Second, can this be output as a shift instruction?  */
  if (REG_P (x)
      && REGNO (x) == REGNO (reg)
      && len + pos <= BITS_PER_WORD
      && ((sign && sign_extended_p (reg, pos, insn))
	  || (!sign && zero_extended_p (reg, pos, insn))))
    {
      pdp10_print_insn (stream, sign ? "ash" : "lsh");
      pdp10_print_tab (stream);
      print_reg (stream, REGNO (reg));
      fprintf (stream, ",-%o\n", BITS_PER_WORD - len - pos);
      return "";
    }

  if (extend)
    {
      pdp10_print_insn (stream, "extend");
      pdp10_print_tab (stream);
      print_reg (stream, REGNO (reg));
      fputs (",[", stream);
    }

  if (GET_CODE (x) == MEM && GET_CODE (XEXP (x, 0)) == PRE_INC)
    {
      fputc ((target_flags & ASM_UPPER) ? 'I' : 'i', stream);
      x = gen_rtx_MEM (GET_MODE (x), XEXP (XEXP (x, 0), 0));
    }
  pdp10_print_insn (stream, store ? "dpb" : "ldb");
  if (sign)
    fputc ((target_flags & ASM_UPPER) ? 'E' : 'e', stream);
  if (GET_CODE (x) == MEM && GET_CODE (XEXP (x, 0)) == POST_INC)
    {
      fputc ((target_flags & ASM_UPPER) ? 'I' : 'i', stream);
      x = gen_rtx_MEM (GET_MODE (x), XEXP (XEXP (x, 0), 0));
    }
  fputc ((target_flags & ASM_TAB) && !extend ? '\t' : ' ', stream);

  if (!extend)
    {
      print_reg (stream, REGNO (reg));
      fputc (',', stream);
    }

  if (pos == -1)
    pdp10_print_operand (stream, x, 0);
  else
    {
      fputc ('[', stream);
      print_byte_pointer (stream, x, len, pos);
      fputc (']', stream);
    }

  if (extend)
    fputc (']', stream);

  fputc ('\n', stream);

  return "";
}

/* Return or output a single instruction to load a register from a
   signed bit-field.  Called to output an extv pattern.  */
const char *
pdp10_output_extv (insn, operands)
     rtx insn;
     rtx *operands;
{
  return output_bitfield_move (asm_out_file, insn, 0, 1, operands[0],
			       operands[1], INTVAL (operands[2]),
			       INTVAL (operands[3]));
}

/* Return or output a sequence of at most three instructions to load a
   register from a signed bit-field.  Called to output an extv
   pattern.  */
const char *
pdp10_output_extv_sequence (operands)
     rtx *operands;
{
  int len = INTVAL (operands[2]);
  int pos = INTVAL (operands[3]);

  if (len == 9)
    pdp10_output_ldbe_9++;
  if (len == 16 && pos == 0)
    output_extv_16_0++;
  else if (len == 16)
    output_extv_16_1++;
  else if (len == 32)
    output_extv_32++;

  operands[3] = GEN_INT (len - BITS_PER_WORD);
  if (pos == 0)
    {
      if (len == 18)
	return "hlre %0,%W1";
      else if (SAME_REG (operands[0], operands[1]))
	return "ash %0,%3";
      else
	return "move %0,%W1\n\tash %0,%3";
    }
  else if (pos == 18)
    {
      if (len == 18)
	return "hrre %0,%W1";
      else
	{
	  operands[2] = GEN_INT (len - 18);
	  return "hrre %0,%W1\n\tash %0,%2";
	}
    }
  else
    {
      operands[2] = GEN_INT (pos);
      if (SAME_REG (operands[0], operands[1]))
	return "lsh %0,%2\n\tash %0,%3";
      else
	return "move %0,%W1\n\tlsh %0,%2\n\tash %0,%3";
    }
}

/* Return or output a sequence of at most three instructions to load a
   register from an unsigned bit-field.  Called to output an extzv
   pattern.  */
const char *
pdp10_output_extzv_sequence (operands)
     rtx *operands;
{
  int len = INTVAL (operands[2]);
  int pos = INTVAL (operands[3]);

  operands[2] = GEN_INT (len + pos - BITS_PER_WORD);
  if (pos == 0)
    {
      if (len == 16)
	output_extzv_16_0++;

      if (len == 18)
	return "hlrz %0,%W1";
      else if (SAME_REG (operands[0], operands[1]))
	return "lsh %0,%2";
      else
	return "move %0,%W1\n\tlsh %0,%2";
    }
  else if (pos == 18)
    {
      if (len == 18)
	return "hrrz %0,%W1";
      else
	{
	  operands[2] = GEN_INT (len - 18);
	  return "hrrz %0,%W1\n\tlsh %0,%2";
	}
    }
  else
    {
      unsigned HOST_WIDE_INT mask;

      if (len == 16)
	output_extzv_16_1++;

      mask = ((HOST_WIDE_INT)1 << len) - 1;
      if (len <= 18)
	operands[3] = gen_int_mode (mask, SImode);
      else
	operands[3] = gen_int_mode ((~mask & WORD_MASK) >> 18, SImode);

      if (SAME_REG (operands[0], operands[1]))
	{
	  if (len + pos == 36)
	    return len <= 18 ? "andi %0,%3" : "tlz %0,%3";
	  else
	    return len <= 18
	      ? "lsh %0,%2\n\tandi %0,%3"
	      : "lsh %0,%2\n\ttlz %0,%3";
	}
      else if (len + pos == 36)
	return len <= 18
	  ? "move %0,%W1\n\tandi %0,%3"
	  : "move %0,%W1\n\ttlz %0,%3";
      else
	return len <= 18
	  ? "move %0,%W1\n\tlsh %0,%2\n\tandi %0,%3"
	  : "move %0,%W1\n\tlsh %0,%2\n\ttlz %0,%3";
    }
}

/* Return or output a single instruction to load a register from an
   unsigned bit-field.  Called to output an extzv pattern.  */
const char *
pdp10_output_extzv (insn, operands)
     rtx insn;
     rtx *operands;
{
  return output_bitfield_move (asm_out_file, insn, 0, 0, operands[0],
			       operands[1], INTVAL (operands[2]),
			       INTVAL (operands[3]));
}

const char *
pdp10_output_extzv_foo (insn, operands)
     rtx insn;
     rtx *operands;
{
  return output_bitfield_move (asm_out_file, insn, 0, 0, operands[0],
			       operands[1], 6, 3);
}

/* Return or output a single instruction to load a halfword from
   memory into the left or right part of a register.  */
const char *
pdp10_output_halfword_insv (insn, operands, position)
     rtx insn;
     rtx *operands;
     int position;
{
  return output_bitfield_move (asm_out_file, insn, 1, 0, operands[1],
			       operands[0], 18, position);
}

/* Return or output a single instruction to store a halfword into
   memory from the left or right part of a register.  */
const char *
pdp10_output_halfword_extv (insn, operands, position)
     rtx insn;
     rtx *operands;
     int position;
{
  return output_bitfield_move (asm_out_file, insn, 0, 0, operands[1],
			       operands[0], 18, position);
}

/* Return or output a single instruction to store a register into a
   bit-field.  Called to output an insv pattern.  */
const char *
pdp10_output_insv (insn, operands)
     rtx insn;
     rtx *operands;
{
  return output_bitfield_move (asm_out_file, insn, 1, 0, operands[3],
			       operands[0], INTVAL (operands[1]),
			       INTVAL (operands[2]));
}

const char *
pdp10_output_movsi (insn, which_alternative)
     rtx insn;
     int which_alternative;
{
  static const char *insns[] =
  {
    "move %0,%1",
    "movei %0,%1",
    "movei %0,%1",	/* Alternative 2.  */
    "movsi %0,%S1",
    "seto %0,",
    "hrloi %0,%U1",
    "hrroi %0,%Q1",
    "move %0,[%1]",	/* Alternative 7.  */
    "setzm %0",
    "setom %0",
    "movem %1,%0",
    "move %0,[%1]"
  };

  if (which_alternative == 2 && TARGET_EXTENDED)
    /* ADDRESS: Alternative 2 is used to load a the value of a symbol
       stored in the immediate field.  In extended mode, use XMOVEI to
       create an extended address.  */
    return "xmovei %0,%1";
  else if (which_alternative == 7 && get_attr_length (insn) == 2)
    return "skipa %0,.+1\n\t%_%1";
  else
    return insns[which_alternative];
}

/* If necessary, expand a double-word move without using DMOVE.
   Return zero if no expansion was made, or else zero.  */
int
pdp10_expand_dmove (destination, source)
     rtx destination;
     rtx source;
{
  rtx destination0, destination1;
  rtx source0, source1;
  enum machine_mode mode0 = SImode;
  enum machine_mode mode1 = SImode;

  /* Many moves can be done in a single RTX insn that expands to two
     machine instructions.  */
  if ((GET_CODE (destination) != MEM ||
       GET_CODE (XEXP (destination, 0)) != MEM)
      && (GET_CODE (source) != MEM ||
	  GET_CODE (XEXP (source, 0)) != MEM))
    return 0;

  if (GET_CODE (destination) == MEM)
    {
      destination0 = gen_rtx_MEM (mode0, XEXP (destination, 0));
      destination1 = gen_rtx_MEM (mode1,
				  plus_constant (XEXP (destination, 0), 1));
    }
  else
    {
      destination0 = gen_rtx_SUBREG (mode0, destination, 0);
      destination1 = gen_rtx_SUBREG (mode1, destination, UNITS_PER_WORD);
    }

  if (GET_CODE (source) == CONST_INT || GET_CODE (source) == CONST_DOUBLE)
    {
      unsigned HOST_WIDE_INT high, low;

      if (GET_CODE (source) == CONST_INT)
	{
	  high = (INTVAL (source) >> 36) & WORD_MASK;
	  low = INTVAL (source) & WORD_MASK;
	}
      else if (GET_MODE (source) == VOIDmode)
	{
	  high =
	    ((CONST_DOUBLE_HIGH (source) << (HOST_BITS_PER_WIDE_INT - 36))
	     + ((CONST_DOUBLE_LOW (source) >> 36)
		& ((HWINT (1) << (HOST_BITS_PER_WIDE_INT - 36)) - 1)));
	  low = CONST_DOUBLE_LOW (source) & WORD_MASK;
	}
      else
	{
	  REAL_VALUE_TYPE r;
	  long y[3];

	  REAL_VALUE_FROM_CONST_DOUBLE (r, source);
	  REAL_VALUE_TO_TARGET_DOUBLE (r, y);

	  high = (((HOST_WIDE_INT)y[0] << 4)
		  + (HOST_WIDE_INT)((y[1] >> 28) & 0xfUL));
	  low = ((((HOST_WIDE_INT)y[1] << 8) & 0xfffffffUL)
		 + (HOST_WIDE_INT)((y[2] >> 24) & 0xffUL));
	}

      source0 = gen_int_mode (high, SImode);
      source1 = gen_int_mode (low, SImode);
    }
  else if (GET_CODE (source) == MEM)
    {
      source0 = gen_rtx_MEM (mode0, XEXP (source, 0));
      source1 = gen_rtx_MEM (mode1,
			     plus_constant (XEXP (source, 0), 1));
    }
  else
    {
      if (GET_CODE (destination0) == SUBREG)
	abort (); /*return 0;*/
      source0 = gen_rtx_SUBREG (mode0, source, 0);
      source1 = gen_rtx_SUBREG (mode1, source, 4);
    }

  emit_move_insn (destination0, source0);
  emit_move_insn (destination1, source1);

  return 1;
}

/* Emit instructions to move four words.  Return nonzero on success.  */
int
pdp10_expand_move_4 (destination, source)
     rtx destination;
     rtx source;
{
  int units_per_move = MOVE_MAX;
  enum machine_mode mode = mode_for_size (BITS_PER_UNIT * units_per_move,
					  MODE_INT, 0);
  int moves = 4 * UNITS_PER_WORD / units_per_move;
  rtx destinations[4];
  rtx sources[4];
  int i;

  sources[0] = NULL_RTX;

  if (GET_CODE (destination) == MEM)
    {
      for (i = 0; i < moves; i++)
	destinations[i] = gen_rtx_MEM (mode,
				       plus_constant (XEXP (destination, 0),
						      i * units_per_move));
    }
  else
    {
      for (i = 0; i < moves; i++)
	destinations[i] = gen_rtx_SUBREG (mode, destination,
					  i * units_per_move);
    }

  if (GET_CODE (source) == CONST_INT || GET_CODE (source) == CONST_DOUBLE)
    {
      unsigned HOST_WIDE_INT high, low;

      if (GET_CODE (source) == CONST_INT)
	{
	  high = (INTVAL (source) >> 36) & 0777777777777ULL;
	  low = INTVAL (source) & 0777777777777ULL;
	}
      else if (GET_MODE (source) == VOIDmode)
	{
	  high = ((CONST_DOUBLE_HIGH (source) << 28)
		  + ((CONST_DOUBLE_LOW (source) >> 36) & 01777777777ULL));
	  low = CONST_DOUBLE_LOW (source) & 0777777777777ULL;
	}
      else
	{
	  REAL_VALUE_TYPE r;
	  long y[3];

	  REAL_VALUE_FROM_CONST_DOUBLE (r, source);
	  REAL_VALUE_TO_TARGET_DOUBLE (r, y);

	  high = (((HOST_WIDE_INT)y[0] << 4)
		  + (HOST_WIDE_INT)((y[1] >> 28) & 0xfUL));
	  low = ((((HOST_WIDE_INT)y[1] << 8) & 0xfffffffUL)
		 + (HOST_WIDE_INT)((y[2] >> 24) & 0xffUL));
	}

      sources[0] = const0_rtx;
      sources[1] = const0_rtx;
      sources[2] = gen_int_mode (high, SImode);
      sources[3] = gen_int_mode (low, SImode);
    }
  else if (GET_CODE (source) == MEM)
    {
      for (i = 0; i < moves; i++)
	sources[i] = gen_rtx_MEM (mode,
				  plus_constant (XEXP (source, 0),
						 i * units_per_move));
    }
  else
    {
      for (i = 0; i < moves; i++)
	sources[i] = gen_rtx_SUBREG (mode, source, i * units_per_move);
    }

  for (i = 0; i < moves; i++)
    emit_move_insn (destinations[i], sources[i]);

  return 1;
}

/* Return nonzero if XBLT should be used instead of BLT.  */
static int
use_xblt (destination, source, length)
     rtx destination;
     rtx source;
     rtx length;
{
  if (!HAVE_XBLT || !TARGET_EXTENDED)
    return 0;

  if (GET_CODE (destination) != CONST_INT
      || (source && GET_CODE (source) != CONST_INT)
      || GET_CODE (length) != CONST_INT)
    return 1;

  if (INTVAL (length) >> 32 > 0)
    return 1;

  if (source)
    {
      HOST_WIDE_INT destination_section = INTVAL (destination) >> 32;
      HOST_WIDE_INT source_section = INTVAL (source) >> 32;

      if (destination_section != source_section)
	return 1;
    }

  return 0;
}

/* Try to emit instructions to BLT a memory block of LENGTH storage
   units from SOURCE to DESTINATION.  Return nonzero on success.  */
static int
expand_blt (destination, source, length)
     rtx destination;
     rtx source;
     rtx length;
{
  rtx ac, e;

  if (GET_CODE (length) != CONST_INT
      || INTVAL (length) % UNITS_PER_WORD != 0)
    return 0;

  ac = gen_reg_rtx (Pmode);
  if (CONSTANT_ADDRESS_P (destination)
      && CONSTANT_ADDRESS_P (source))
    {
      emit_insn (gen_move_two_halves (ac, source, destination));
    }
  else
    {
      emit_insn (gen_MOVEI (ac, destination));
      emit_insn (gen_HRLI (ac, source));
    }

  {
    HOST_WIDE_INT len = INTVAL (length) - UNITS_PER_WORD;
    if (CONSTANT_ADDRESS_P (destination))
      e = plus_constant (destination, len);
    else
      {
	e = gen_reg_rtx (Pmode);
	emit_insn (gen_addsi3 (e, destination, GEN_INT (len)));
      }
  }

  /* Beginning at the location addressed by the left half-word of AC,
     move words to another area beginning at the location addressed by
     the right half-word of AC.  Continue until a word is moved to
     location E.  */
  emit_insn (gen_BLT (ac, gen_rtx_MEM (BLKmode, e)));

  return 1;
}

/* Try to emit instructions to XBLT a memory block of LENGTH storage
   units from SOURCE to DESTINATION.  Return nonzero on success.  */
static int
expand_xblt (destination, source, length)
     rtx destination;
     rtx source;
     rtx length;
{
  rtx temp, mem, acs;
  int i, n;

  if (GET_CODE (length) != CONST_INT)
    return 0;

  n = INTVAL (length);

  switch (n / UNITS_PER_WORD)
    {
    case 0:
      break;

    case 2:
    case 3:
      temp = gen_reg_rtx (DImode);
      for (i = 0; i < n / 8; i++)
	{
	  emit_move_insn (temp, gen_rtx_MEM (DImode,
					     plus_constant (source, i)));
	  emit_move_insn (gen_rtx_MEM (DImode,
				       plus_constant (destination, i)), temp);
	}
      /* Fall through.  */

    case 1:
      if (n % 8 >= 4)
	{
	  int m = 2 * (n / 8);
	  temp = gen_reg_rtx (SImode);
	  emit_move_insn (temp, gen_rtx_MEM (SImode,
					     plus_constant (source, m)));
	  emit_move_insn (gen_rtx_MEM (SImode,
				       plus_constant (destination, m)),
			  temp);
	}
      break;

    default:
      acs = gen_reg_rtx (TImode);
      emit_move_insn (gen_rtx_SUBREG (SImode, acs, 0),
		      GEN_INT (INTVAL (length) / UNITS_PER_WORD));
      emit_move_insn (gen_rtx_SUBREG (Pmode, acs, 4), source);
      emit_move_insn (gen_rtx_SUBREG (Pmode, acs, 8), destination);
#if 0
      emit_insn (gen_XBLT (gen_rtx_SUBREG (SImode, acs, 0),
			   gen_rtx_SUBREG (Pmode, acs, 4),
			   gen_rtx_SUBREG (Pmode, acs, 8)));
#else
      emit_insn (gen_XBLT (acs));
#endif
      break;
    }

  switch (n % UNITS_PER_WORD)
    {
    case 0:
      break;

    case 1:
      temp = gen_reg_rtx (QImode);
      mem = gen_rtx_MEM (QImode, plus_constant (source, n / 4));
      set_mem_align (mem, BITS_PER_WORD);
      emit_move_insn (temp, mem);
      mem = gen_rtx_MEM (QImode, plus_constant (destination, n / 4));
      set_mem_align (mem, BITS_PER_WORD);
      emit_move_insn (mem, temp);
      break;

    case 2:
      temp = gen_reg_rtx (HImode);
      mem = gen_rtx_MEM (HImode, plus_constant (source, n / 4));
      set_mem_align (mem, BITS_PER_WORD);
      emit_move_insn (temp, mem);
      mem = gen_rtx_MEM (HImode, plus_constant (destination, n / 4));
      set_mem_align (mem, BITS_PER_WORD);
      emit_move_insn (mem, temp);
      break;

    case 3:
      temp = gen_reg_rtx (SImode);
      emit_insn (gen_extzv (temp, gen_rtx_MEM (SImode,
					       plus_constant (source, n / 4)),
			    GEN_INT (27), GEN_INT (0)));
      emit_insn (gen_insv (gen_rtx_MEM (SImode,
					plus_constant (destination, n / 4)),
			   GEN_INT (27), GEN_INT (0), temp));
      break;
    }

  return 1;
}

#if TARGET_STRING
/* Try to emit instructions to MOVSLJ a memory block of LENGTH storage
   units from SOURCE to DESTINATION.  Return nonzero on success.  */
static int
expand_movslj (destination, source, length)
     rtx destination;
     rtx source;
     rtx length;
{
  abort ();
}
#endif

/* Emit instructions to copy a memory block.  Return nonzero on
   success.  */
int
pdp10_expand_movstrsi (operands)
     rtx *operands;
{
  /* operands[0] is the block destination address
     operands[1] is the block source address
     operands[2] is the block length
     operands[3] is the known alignment.  */

#if TARGET_STRING
  if (HAVE_MOVSLJ && TARGET_STRING)
    return expand_movslj (operands[0], operands[1], operands[2]);
#endif

  operands[0] = XEXP (operands[0], 0);
  operands[1] = XEXP (operands[1], 0);

  if (GET_CODE (operands[3]) != CONST_INT
      || INTVAL (operands[3]) != UNITS_PER_WORD)
    return 0;

  if (GET_CODE (operands[1]) == CONST
      && GET_CODE (XEXP (operands[1], 0)) == PLUS
      && GET_CODE (XEXP (XEXP (operands[1], 0), 1)) == CONST_INT)
    {
      rtx x = XEXP (operands[1], 0);
      HOST_WIDE_INT offset = INTVAL (XEXP (x, 1));
      operands[1] = plus_constant (XEXP (x, 0), offset & ADDRESS_MASK);
    }

  if (use_xblt (operands[0], operands[1], operands[2]))
    return expand_xblt (operands[0], operands[1], operands[2]);
  else
    return expand_blt (operands[0], operands[1], operands[2]);
}

/* Emit instructions to use BLT to clear a memory block of LENGTH
   storage units from SOURCE to DESTINATION.  Return nonzero on
   success.  */
static int
expand_blt_clear (destination, length)
     rtx destination;
     rtx length;
{
  HOST_WIDE_INT len;
  rtx ac, e;

  if (GET_CODE (length) != CONST_INT
      || INTVAL (length) % UNITS_PER_WORD != 0)
    return 0;

  len = INTVAL (length);
  ac = gen_reg_rtx (Pmode);

  if (CONSTANT_ADDRESS_P (destination))
    {
      emit_insn (gen_move_two_halves (ac,
				      destination,
				      plus_constant (destination,
						     UNITS_PER_WORD)));
      e = plus_constant (destination, len - UNITS_PER_WORD);
    }
  else
    {
      emit_insn (gen_MOVSI (ac, destination));
      emit_insn (gen_HRRI (ac, plus_constant (destination, UNITS_PER_WORD)));
      e = gen_reg_rtx (Pmode);
      emit_insn (gen_addsi3 (e, destination,
			     GEN_INT (len - UNITS_PER_WORD)));
    }

  emit_move_insn (gen_rtx_MEM (SImode, destination), const0_rtx);

  /* Beginning at the location addressed by the left half-word of AC,
     move words to another area beginning at the location addressed by
     the right half-word of AC.  Continue until a word is moved to
     location E.  */
  emit_insn (gen_BLT (ac, gen_rtx_MEM (BLKmode, e)));

  return 1;
}

/* Emit instructions to use XBLT to clear a memory block of LENGTH
   storage units from SOURCE to DESTINATION.  Return nonzero on
   success.  */
static int
expand_xblt_clear (destination, length)
     rtx destination;
     rtx length;
{
  rtx temp, acs;
  int i, n;

  if (GET_CODE (length) != CONST_INT)
    return 0;

  n = INTVAL (length);

  switch (n / UNITS_PER_WORD)
    {
    case 0:
      break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      for (i = 0; i < n / UNITS_PER_WORD; i++)
	emit_move_insn (gen_rtx_MEM (SImode, plus_constant (destination, i)),
			const0_rtx);
      break;

    default:
      acs = gen_reg_rtx (TImode);
      emit_move_insn (gen_rtx_SUBREG (SImode, acs, 0),
		      GEN_INT (n / UNITS_PER_WORD - 1));
      emit_move_insn (gen_rtx_SUBREG (Pmode, acs, 4), destination);
      destination = force_reg (Pmode, destination);
      emit_insn (gen_MOVEI (gen_rtx_SUBREG (Pmode, acs, 8),
			    plus_constant (destination, 1)));
      emit_move_insn (gen_rtx_MEM (SImode, destination), const0_rtx);
#if 0
      emit_insn (gen_XBLT (gen_rtx_SUBREG (SImode, acs, 0),
			   gen_rtx_SUBREG (Pmode, acs, 4),
			   gen_rtx_SUBREG (Pmode, acs, 8)));
#else
      emit_insn (gen_XBLT (acs));
#endif
    }

  switch (n % UNITS_PER_WORD)
    {
    case 0:
      break;

    case 1:
      temp = gen_rtx_MEM (QImode, plus_constant (destination, n / 4));
      set_mem_align (temp, BITS_PER_WORD);
      emit_move_insn (temp, const0_rtx);
      break;

    case 2:
      temp = gen_rtx_MEM (HImode, plus_constant (destination, n / 4));
      set_mem_align (temp, BITS_PER_WORD);
      emit_move_insn (temp, const0_rtx);
      break;

    case 3:
      temp = gen_reg_rtx (SImode);
      emit_move_insn (temp, GEN_INT (0777));
      emit_insn (gen_andsi3 (gen_rtx_MEM (SImode,
					  plus_constant (destination, n / 4)),
			     gen_rtx_MEM (SImode,
					  plus_constant (destination, n / 4)),
			     temp));
      break;
    }

  return 1;
}

#if TARGET_STRING
/* Emit instructions to use MOVST to clear a memory block of LENGTH
   storage units from SOURCE to DESTINATION.  Return nonzero on
   success.  */
static int
expand_movst_clear (destination, length)
     rtx destination;
     rtx length;
{
  abort ();
}
#endif

/* Emit instructions to clear a memory block.  Return nonzero on
   success.  */
int
pdp10_expand_clrstrsi (operands)
     rtx *operands;
{
  /* operands[0] is the block destination address
     operands[1] is the block length
     operands[2] is the known alignment.  */

#if TARGET_STRING
  if (HAVE_MOVST && TARGET_STRING)
    return expand_movst_clear (operands[0], operands[1]);
#endif

  if (GET_CODE (operands[2]) != CONST_INT
      || INTVAL (operands[2]) != UNITS_PER_WORD)
    return 0;

  operands[0] = XEXP (operands[0], 0);

  if (use_xblt (operands[0], NULL_RTX, operands[1]))
    return expand_xblt_clear (operands[0], operands[1]);
  else
    return expand_blt_clear (operands[0], operands[1]);
}

#if TARGET_STRING
/* Emit instructions to compare two memory blocks.  Return nonzero on
   success.  */
int
pdp10_expand_cmpstrsi (operands)
     rtx *operands ATTRIBUTE_UNUSED;
{
  /* operands[0] is the result
     operands[1] is the address of the first string
     operands[2] is the address of the second string
     operands[3] is the string length
     operands[4] is the known alignmen.  */

  if (!HAVE_CMPS || !TARGET_STRING)
    return 0;

  abort ();
}
#endif


/**********************************************************************

	Pointer Arithmetic

**********************************************************************/

static rtx	expand_adjust_byte_pointer PARAMS ((tree type, rtx target,
						    rtx op1, rtx op2));
static void	avoid_reg_equal PARAMS ((rtx op0, rtx op1));
static int	maybe_expand_ibp PARAMS ((int byte_size, rtx op0, rtx op1,
					  int increment));
static rtx	expand_add_or_sub_pointer PARAMS ((rtx target, rtx op0,
						   tree type0, rtx op1,
						   tree type1, int add));
static rtx	expand_subtract_pointers PARAMS ((enum machine_mode mode,
						  rtx target, rtx op0,
						  tree type0, rtx op1,
						  tree type1));

/* Return the value of the S field in byte pointers used to point to
   variables of type TYPE.  A return value of 36 means that a word
   pointer should be used.  */
int
pdp10_bytesize (type)
     tree type;
{
  int size;

  if (TREE_CODE (type) == RECORD_TYPE)
    return 36;

  if (TREE_CODE (type) == ARRAY_TYPE)
    type = TREE_TYPE (type);
  if (VOID_TYPE_P (type))
    size = 9;
  else
    size = tree_low_cst (TYPE_SIZE (type), 1);
  if (size > 36)
    size = 36;

  return size == 16 ? 18 : size == 32 ? 36 : size;
}

/* Emit a dummy move instruction which will be optimized away later.
   The purpose of this instruction is to fool GCC into not generating
   a REG_EQUAL note for a previously emitted (sequence of)
   instruction(s).

   OP0 and OP1 are two operands that, if convenient, can be used in
   the dummy move.  */
static void
avoid_reg_equal (op0, op1)
     rtx op0;
     rtx op1;
{
  rtx temp;

  if (GET_CODE (op0) == REG)
    temp = op0;
  else if (GET_CODE (op1) == REG)
    temp = op1;
  else
    temp = gen_reg_rtx (SImode);

  emit_move_insn (temp, temp);
}

/* Emit instructions, without ADJBP, to adjust the byte pointer in OP0
   by OP1 units, and store the result in TARGET if that's convenient.
   Return the result.  */
static rtx
expand_adjust_byte_pointer (type, target, op0, op1)
     tree type;
     rtx target;
     rtx op0;
     rtx op1;
{
  int bytes_per_word;
  int byte_size;
  rtx mod;

  byte_size = pdp10_bytesize (type);
  bytes_per_word = BITS_PER_WORD / byte_size;

  {
    /* This is unfinished.  Besides, the KCC source code hints about a
       better way to do this.  See comment after simsubbp in ccout.c.  */
    abort ();

    /* char9:
       MOVE target,op0
       MOVE op3,op1
       ANDI op3,3
       ASH op1,-2
       ADD target,op1
       TRZE op3,1	op3=0 => 2 insns
       IBP target	op3=1 => 3 insns
       JUMPE op3,.+3	op3=2 => 4 insns
       IBP target	op3=3 => 5 insns
       IBP target

       char7:
       MOVE target,op0
       IDIV op1,5	op3=0 => 2 insns
       ADD target,op1	op3=1 => 4 insns
       JRST .+2		op3=2 => 6 insns
       IBP target
       SOJGE op3,.-1 */

    /* Move the source to the destination.  */
    target = force_reg (Pmode, target);
    emit_move_insn (target, op0);

    /* Divide the increment by the number of bytes per word.  */
    mod = gen_reg_rtx (Pmode);
    op1 = force_reg (Pmode, op1);
    switch (byte_size)
      {
      case 6:
	abort ();
      case 7:
	emit_insn (gen_IDIVM (op1, op1, GEN_INT (5)));
	break;
      case 8:
      case 9:
	emit_insn (gen_ashrsi3_pointer (op1, op1, GEN_INT (2)));
	break;
      case 16:
      case 18:
	emit_insn (gen_ashrsi3_pointer (op1, op1, GEN_INT (1)));
	break;
      default:
	abort ();
      }

    emit_insn (gen_addsi3 (target, target, op1));
  }

  return target;
}

/* Try to emit instructions to use IBP to add INCREMENT to OP1 and
   store the result in OP0.  Return nonzero on success.  */
static int
maybe_expand_ibp (byte_size, op0, op1, increment)
     int byte_size;
     rtx op0;
     rtx op1;
     int increment;
{
  if (increment == 0)
    return 1;

  if (HAVE_ADJBP && increment != 1)
    return 0;

  {
    int bytes_per_word;
    int addi, ibp;
    int i;

    bytes_per_word = BITS_PER_WORD / byte_size;

    if (increment > 0)
      addi = increment / bytes_per_word;
    else
      addi = (increment - bytes_per_word + 1) / bytes_per_word;
    ibp = increment - bytes_per_word * addi;
 
   if (addi != 0)
      emit_insn (gen_addsi3 (op0, op1, GEN_INT (addi)));
    else
      {
	if (!reg_or_mem_operand (op1, Pmode))
	  op1 = force_reg (Pmode, op1);
	emit_insn (gen_IBP (op0, op1));
	ibp--;
      }

    for (i = 0; i < ibp; i++)
      emit_insn (gen_IBP (op0, op0));
  }

  return 1;
}

/* Emit instructions to add OP1 to the byte pointer in OP0 that points
   to items of type TYPE.  Store the result in TARGET if that's
   convenient.  Return the result.  */
rtx
pdp10_expand_addbp3 (type, target, op0, op1)
     tree type;
     rtx target;
     rtx op0;
     rtx op1;
{
  int bytes_per_word = 36 / pdp10_bytesize (type);
  HOST_WIDE_INT increment;

  /* First, try to use ADDI or SUBI.  */
  if (GET_CODE (op1) == CONST_INT
      && (increment = INTVAL (op1)) % bytes_per_word == 0)
    {
      int n = increment / bytes_per_word;
      if (CONSTANT_P (op0))
	return plus_constant (op0, n);
      else
	emit_insn (gen_addsi3 (target, op0, GEN_INT (n)));
      return target;
    }

  /* Second, try to use IBP.  */
  if (GET_CODE (op1) == CONST_INT
      && maybe_expand_ibp (pdp10_bytesize (type), target, op0, increment))
    return target;

  /* If we get here, either the increment is not a constant, or a
     sequence of IBP instructions is too costly.  */

  if (HAVE_ADJBP)
    {
      if (target == op0)
	target = gen_reg_rtx (Pmode);
      else
	target = force_reg (Pmode, target);
      if (!general_operand (op1, Pmode))
	op1 = force_reg (Pmode, op1);
      emit_move_insn (target, op1);
      if (!general_operand (op0, Pmode))
	op0 = force_reg (Pmode, op0);
      emit_insn (gen_ADJBP (target, op0, target));
      return target;
    }
  else
    {
      /* Emit sequence to adjust a byte pointer without ADJBP.  */
      return expand_adjust_byte_pointer (type, target, op0, op1);
    }
}

/* Emit instructions to add/substract to/from a pointer.  */
static rtx
expand_add_or_sub_pointer (target, op0, type0, op1, type1, add)
     rtx target;
     rtx op0;
     tree type0;
     rtx op1;
     tree type1;
     int add;
{
  int neg = 0;
  tree type;

  if (!add && GET_CODE (op0) == CONST_INT)
    {
      rtx tmp = GEN_INT (-INTVAL (op0));
      op0 = op1;
      op1 = tmp;
      neg = 1;
      add = 1;
    }

  if (!target)
    target = gen_reg_rtx (Pmode);

  type = TREE_TYPE (POINTER_TYPE_P (type0) ? type0 : type1);

  if (WORD_TYPE_P (type))
    {
      /* Add to a word pointer by first dividing the increment by the
         number of storage units per word, which is always four.
         Then, add the increment to the pointer.  */

      rtx increment;

      if (UNITS_PER_WORD != 4)
	abort ();

      if (GET_CODE (op1) == CONST_INT)
	{
	  HOST_WIDE_INT i = INTVAL (op1);
	  HOST_WIDE_INT sign = TARGET_EXTENDED ?
	    (HWINT (1) << 29) : (HWINT (1) << 17);

	  if (CONSTANT_ADDRESS_P (op0))
	    {
	      emit_move_insn (target, plus_constant (op0, add ? i : -i));
	      if (neg)
		target = gen_rtx_NEG (Pmode, target);
	      return target;
	    }

	  i &= ADDRESS_MASK;
	  i = (i ^ sign) - sign;
	  if (!add)
	    {
	      i = -i;
	      add = 1;
	    }
	  increment = GEN_INT (i);
	}
      else
	increment = op1;

      /* Make sure that the first operand is a register, if possible.  */
      if (add && GET_CODE (op0) != REG && GET_CODE (op1) == REG)
	{
	  rtx tmp = op0;
	  op0 = increment;
	  increment = tmp;
	}

      if (!register_operand (op0, Pmode))
	op0 = force_reg (Pmode, op0);
      if (!reg_or_mem_operand (increment, Pmode))
	increment = force_reg (Pmode, increment);

      if (add)
	emit_insn (gen_addsi3 (target, op0, increment));
      else
	emit_insn (gen_subsi3 (target, op0, increment));
    }
  else
    {
      /* Add to a byte pointer by first dividing the increment by the
         number of bytes per word, which depends on the byte size.
         Then, use an IBP or ADJBP instruction.  */

      rtx increment = op1;

      if (!add)
	{
	  if (GET_CODE (increment) == CONST_INT)
	    increment = GEN_INT (-INTVAL (increment));
	  else
	    {
	      rtx temp = gen_reg_rtx (Pmode);
	      emit_insn (gen_negsi2 (temp, increment));
	      increment = temp;
	    }
	}

      target = pdp10_expand_addbp3 (type, target, op0, increment);
    }

  /* This is to fool GCC into not generating a REG_EQUAL note for this
     sequence.  */
  avoid_reg_equal (op0, op1);

  if (neg)
    return expand_simple_unop (Pmode, NEG, target, NULL_RTX, 0);

  return target;
}

/* Emit instructions to subtract two byte pointers.  */
static rtx
expand_subtract_pointers (mode, target, op0, type0, op1, type1)
     enum machine_mode mode;
     rtx target;
     rtx op0;
     tree type0;
     rtx op1;
     tree type1 ATTRIBUTE_UNUSED;
{
  tree type = TREE_TYPE (type0);

  if (!target)
    target = gen_reg_rtx (Pmode);

  if (WORD_TYPE_P (type))
    {
      /* Calculate the difference between two word pointers by
	 subtracting and multiplying by the number of storage units
	 per word.  */

      rtx temp = gen_reg_rtx (Pmode);
      emit_insn (gen_subsi3 (temp, op0, op1));
      emit_insn (gen_ashlsi3_pointer (target, temp, GEN_INT (2)));
    }
  else
    {
      if (HAVE_SUBBP)
	{
	  if (!register_operand (op0, Pmode))
	    op0 = force_reg (Pmode, op0);
	  if (!reg_or_mem_operand (op1, Pmode))
	    op1 = force_reg (Pmode, op0);
	  emit_insn (gen_SUBBP (target, op0, op1));
	}
      else
	{
	  /* Calculate the difference between two byte pointers.

	     This algorithm is copied from KCC with some small
	     modifications.  Basically, the steps are:

		; Subtract the pointers.
		SUB op0,op1

		; Multiply the difference by the number of bytes per
		; word and a magic shift factor.
		MULI op0,<bytes per word>*<2^shift>

		; Shift back the lower word of the product to
		; compensate for the shift factor.  op0+1 will be an
		; approximation of the final result.
		ASH op0+1,-shift

		; Add a magic value from a table to adjust for the
		; difference in byte position.  The table is indexed
		; by op0 which is the upper bits of the result of
		; the multiplication, and thus depends on the byte
		; positions in the pointers.
		ADD op0+1,table(op0)

	     The result is in op0+1.  */

	  int bytesize = pdp10_bytesize (type);
	  rtx temp = gen_reg_rtx (DImode), diff;
	  rtx a = gen_rtx_SUBREG (SImode, temp, 0);
	  char *str;
	  int shift, mul;
	  rtx sym, mem;

	  if (bytesize == 16)
	    bytesize = 18;

	  /* ADDRESS: the code differs slightly in extended and
	     unextended mode.  */

	  if (TARGET_EXTENDED)
	    {
	      if (bytesize == 18 || bytesize == 7)
		shift = 5;
	      else
		shift = 4;
	    }
	  else
	    shift = 1;
	  mul = (BITS_PER_WORD / bytesize) << shift;

	  str = xstrdup ("%BAD..");
	  str[4] = TARGET_EXTENDED ? 'X' : 'L';
	  str[5] = bytesize == 18 ? 'H' : '0' + bytesize;
	  sym = gen_rtx_SYMBOL_REF (Pmode, str);
	  used_byte_subtraction_table[bytesize] = 1;

	  diff = gen_reg_rtx (SImode);

	  if (GET_MODE (op0) != Pmode)
	    op0 = convert_to_mode (Pmode, op0, 1);
	  if (GET_MODE (op1) != Pmode)
	    op1 = convert_to_mode (Pmode, op0, 1);

	  emit_insn (gen_subsi3 (a, op0, op1));
	  emit_insn (gen_MUL (temp, a, GEN_INT (mul)));
	  emit_move_insn (diff, gen_rtx_SUBREG (SImode, temp, 4));
	  emit_insn (gen_ashrsi3 (diff, diff, GEN_INT (shift)));

	  if (TARGET_EXTENDED && !TARGET_SMALLISH)
	    {
	      emit_insn (gen_addsi3 (a, a, sym));
	      mem = gen_rtx_MEM (SImode, a);
	    }
	  else
	    mem = gen_rtx_MEM (SImode, gen_rtx_PLUS (Pmode, a, sym));

	  emit_insn (gen_addsi3 (diff, diff, mem));

	  if (bytesize == 18)
	    emit_insn (gen_ashlsi3_pointer (diff, diff, GEN_INT (1)));

	  emit_move_insn (target, diff);
	}
    }

  /* This is to fool GCC into not generating a REG_EQUAL note for this
     sequence.  */
  avoid_reg_equal (op0, op1);

  if (GET_MODE (target) != mode)
    target = convert_to_mode (mode, target, 0);

  return target;
}


/**********************************************************************

	Unconditional Jumps

**********************************************************************/

/* Return nonzero if the call instruction can be output as JRST.  */
int
pdp10_output_call_as_jrst_p (insn)
     rtx insn;
{
  return SIBLING_CALL_P (insn) || find_reg_note (insn, REG_NORETURN, 0);
}

int
pdp10_output_call_as_jsp_p (sym)
     rtx sym;
{
  tree id = maybe_get_identifier (XSTR (sym, 0));
  tree fn, type, attr;

  return
    id != NULL_TREE
    && (fn = IDENTIFIER_GLOBAL_VALUE (id)) != NULL_TREE
    && TREE_CODE (TREE_TYPE (fn)) == FUNCTION_DECL
    && (type = TREE_TYPE (fn)) != NULL_TREE
    && (attr = TYPE_ATTRIBUTES (type)) != NULL_TREE
    && lookup_attribute ("fastcall", attr);
}


/**********************************************************************

	Conditional Jumps

**********************************************************************/

static int	jrst_or_popj_p PARAMS ((rtx insn));
static rtx	jump_target PARAMS ((rtx insn));
static int	large_jump_address_p PARAMS ((rtx address));

int
pdp10_generate_cbranchdi (code, op0, op1)
     enum rtx_code code;
     rtx op0, op1;
{
  return 0;
}

/* Will this instruction be output as a JRST or a POPJ?  */
static int
jrst_or_popj_p (insn)
     rtx insn;
{
  /* A call instruction output as JRST is ok.  */
  if (GET_CODE (insn) == CALL_INSN && pdp10_output_call_as_jrst_p (insn))
    return 1;

  /* An unconditional jump without side effects is ok.  */
  if (GET_CODE (insn) == JUMP_INSN && onlyjump_p (insn)
      /* Note that any_uncondjump_p doesn't work here, since it only
	 recognizes jumps to labels.  */
      && !any_condjump_p (insn))
    return 1;

  /* A return instruction is ok.  */
  if (returnjump_p (insn))
    return 1;

  return 0;
}

static rtx
jump_target (insn)
     rtx insn;
{
  rtx set = pc_set (insn);
  rtx call;

  if (set)
    return SET_SRC (set);

  if (GET_CODE (insn) != CALL_INSN)
    return NULL_RTX;

  call = PATTERN (insn);
  if (GET_CODE (call) == SET)
    call = SET_SRC (call);

  if (GET_CODE (call) != CALL)
    abort ();
  if (GET_CODE (XEXP (call, 0)) != MEM)
    abort ();

  return XEXP (XEXP (call, 0), 0);
}

/* Return nonzero if the address in the jump instruction requires an
   additional (assembler literal) word.  */
static int
large_jump_address_p (address)
     rtx address;
{
  if (!TARGET_EXTENDED)
    return 0;

  /* Bail out and return 0 for now.  */
  return 0;
}

/* Output a JRST instruction, unless the jump target is another
   unconditional jump instruction (i.e. JRST or POPJ).  In that case
   output the target instruction directly (unless optimize_size is on
   and the result would be larger).  Return an empty string for the
   convenience of the caller.  */
const char *
pdp10_output_jrst (label)
     rtx label;
{
  rtx target = next_active_insn (label);
  rtx address;

  if (optimize >= 2
      && jrst_or_popj_p (target)
      && ((address = jump_target (target)) == NULL_RTX
	  || GET_CODE (address) != LABEL_REF
	  || XEXP (address, 0) != label)
      && !(optimize_size && large_jump_address_p (address)))
    {
      final_scan_insn (target, asm_out_file, optimize, 0, 0);
      maybe_remove_label_and_following_code (label);
    }
  else
    output_asm_insn ("jrst %F0", &label);

  return "";
}

/* Generate code to invert the sign bit of X and return the result.
   Called to prepare operands for an unsigned comparison.  */
rtx
pdp10_flip_sign_bit (x)
     rtx x;
{
  if (GET_CODE (x) == CONST_INT)
    return gen_int_mode (INTVAL (x) ^ SIGN_BIT, SImode);
  else
    {
      rtx y = gen_reg_rtx (SImode);
      if (GET_MODE (x) == DImode)
	x = gen_rtx_SUBREG (SImode, x, 0);
      else if (!register_operand (x, SImode))
	x = force_reg (SImode, x);
      emit_insn (gen_xorsi3 (y, x, gen_int_mode (SIGN_BIT, SImode)));
      return y;
    }
}

/* Return a suitable T[RLD]N[EN] instruction.  OPERANDS are the
   operands and LENGTH is the RTL insn length.  */
const char *
pdp10_output_test_and_skip (insn, operands)
     rtx insn;
     rtx *operands;
{
  static const char *asms[] =
  { "trn%0 %1,%2",
    "tln%0 %1,%S2",
    "tdn%0 %1,[%2]",
    "tdn%0 %1,%2" };
  int which_alternative;

  if (GET_CODE (operands[2]) == CONST_INT)
    {
      HOST_WIDE_INT i = INTVAL (operands[2]);

      if (CONST_OK_FOR_LETTER_P (i, 'I'))
	which_alternative = 0;
      else if (CONST_OK_FOR_LETTER_P (i, 'L'))
	which_alternative = 1;
      else
	which_alternative = 2;
    }
  else
    which_alternative = 3;

  if (get_attr_length (insn) == 1)
    return asms[which_alternative];
  else
    {
      /* Reverse the skip condition and output a JRST.  */
      operands[0] = gen_rtx_fmt_ee (reverse_condition (GET_CODE (operands[0])),
				    VOIDmode, NULL_RTX, NULL_RTX);
      output_asm_insn (asms[which_alternative], operands);
      return pdp10_output_jrst (operands[3]);
    }
}


/**********************************************************************

	Function Prologue and Epilogue

**********************************************************************/

#define emit_frame_insn(PATTERN) RTX_FRAME_RELATED_P (emit_insn (PATTERN)) = 1

static void	expand_save_or_restore_return_address PARAMS ((int save,
							       int clob_size));
static void	expand_save_or_restore_regs PARAMS ((int save,
						     int *clobbered,
						     int clobbered_size,
						     int single));
static void	expand_set_up_frame_pointer PARAMS ((void));

/* Return the number of clobbered registers that the current function
   should save on stack, and mark those registers in the CLOBBERED
   array.  */
static int
clobbered_regs (clobbered)
     int *clobbered;
{
  int i, n;

  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    clobbered[i] = regs_ever_live[i] && !call_used_regs[i];

  clobbered[STACK_POINTER_REGNUM] = 0;

  n = 0;
  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    {
      if (clobbered[i])
	n += UNITS_PER_WORD;
    }

  return n;
}

/* Emit instructions to save or restore the return address on stack.
   This only happens when the register arguments must be stored in the
   stack.  */
static void
expand_save_or_restore_return_address (save, clobbered_size)
     int save;
     int clobbered_size;
{
  int offset1, offset2;

  if (!current_function_pretend_args_size)
    return;

  /* If we need to store pretend args, move the return address out of
     their way.  */

  offset1 = - (current_function_outgoing_args_size +
	       get_frame_size () +
	       clobbered_size +
	       current_function_pretend_args_size) / UNITS_PER_WORD;
  offset2 = - (current_function_outgoing_args_size +
	       get_frame_size () +
	       clobbered_size) / UNITS_PER_WORD;

  if (!save)
    {
      int tmp = offset1;
      offset1 = offset2;
      offset2 = tmp;
    }

  emit_move_insn (gen_rtx_REG (SImode, 0),
		  GEN_MEM_SP_PLUS (SImode, offset1));
  emit_move_insn (GEN_MEM_SP_PLUS (SImode, offset2),
		  gen_rtx_REG (SImode, 0));
}

/* Save or restore clobbered callee-saved registers.  SAVE is nonzero
   when saving, and zero when restoring.  CLOBBERED indicates
   clobbered registers.  SINGLE is nonzero when a single PUSH or POP
   instruction can be used.  */
static void
expand_save_or_restore_regs (save, clobbered, clobbered_size, single)
     int save;
     int *clobbered;
     int clobbered_size;
     int single;
{
  int fp_clobbered = 0;
  int go_back = 1;
  int max_move;
  int min_blt;
  int offset;
  int regs;
  int i, j, k;

  if (clobbered_size == 0)
    return;

  offset = -((current_function_outgoing_args_size +
	      get_frame_size () +
	      clobbered_size)
	     / UNITS_PER_WORD) + 1;

  max_move = MOVE_MAX / UNITS_PER_WORD;

  /* ADDRESS: Don't use BLT when generating multi-section code.
     Otherwise, use BLT if using (D)MOVE instructions would generate
     more code.  */
  if (TARGET_EXTENDED)
    min_blt = 1000;
  else
    min_blt = 3 * max_move + 1;

  /* Start with the frame pointer.  */
  for (i = HARD_FRAME_POINTER_REGNUM; i < 16;
       (i == HARD_FRAME_POINTER_REGNUM && go_back) ? (i = go_back = 0) : (i += regs))
    {
      regs = 1;
      if (!clobbered[i])
	continue;

      /* If a single PUSH or POP instruction can be used, do so and
	 return.  */
      if (single)
	{
	  rtx stack;
	  if (save)
	    {
	      stack = gen_rtx_PRE_INC (Pmode, stack_pointer_rtx);
	      emit_frame_insn (gen_PUSH (gen_rtx_MEM (SImode, stack),
					 gen_rtx_REG (SImode, i)));
	    }
	  else
	    {
	      stack = gen_rtx_POST_DEC (Pmode, stack_pointer_rtx);
	      emit_frame_insn (gen_POP (gen_rtx_REG (SImode, i),
					gen_rtx_MEM (SImode, stack)));
	    }
	  return;
	}

      /* Find a consecutive block of clobbered registers.

	 ADDRESS: When extended addressing is in use, the stack can
	 span multiple sections, so BLT can't be used.  In this case,
	 the search is limited to just examine the next register.  */
      if (i == HARD_FRAME_POINTER_REGNUM)
	{
	  k = i + 1;
	  fp_clobbered = 1;
	  clobbered[i] = 0;
	}
      else if (TARGET_EXTENDED
	  && !TARGET_SMALLISH
	  && i <= FIRST_PSEUDO_REGISTER - 2)
	k = i + 2;
      else
	k = FIRST_PSEUDO_REGISTER;

      for (j = i + 1; j < k; j++)
	{
	  if (clobbered[j])
	    regs++;
	  else
	    break;
	}
      
      if (regs >= min_blt)
	{
	  /* Use a BLT instruction.  */

	  rtx from_start, to_start, to_end;

	  if (save)
	    {
	      from_start = GEN_INT (i);
	      to_start = plus_constant (stack_pointer_rtx, offset);
	      to_end
		= GEN_MEM_SP_PLUS (BLKmode, offset + regs - 1);
	    }
	  else
	    {
	      from_start = plus_constant (stack_pointer_rtx, offset);
	      to_start = GEN_INT (i);
	      to_end
		= gen_rtx_MEM (BLKmode, GEN_INT (i + regs - 1));
	    }

#define REG0 gen_rtx_REG (Pmode, 0)
	  emit_frame_insn (gen_MOVEI (REG0, to_start));
	  emit_frame_insn (gen_HRLI (REG0, from_start));
	  emit_frame_insn (gen_BLT (REG0, to_end));
#undef REG0
	}
      else
	{
	  /* Use a single MOVE or DMOVE instruction.  */

	  enum machine_mode mode;
	  rtx insn;

	  if (regs > max_move)
	    regs = max_move;
	  mode = (regs == 1 ? SImode : DImode);

	  if (save)
	    insn = emit_move_insn (GEN_MEM_SP_PLUS (mode, offset),
				   gen_rtx_REG (mode, i));
	  else
	    insn = emit_move_insn (gen_rtx_REG (mode, i),
				   GEN_MEM_SP_PLUS (mode, offset));
	  RTX_FRAME_RELATED_P (insn) = 1;
	}

      offset += regs;
    }

  clobbered[HARD_FRAME_POINTER_REGNUM] = fp_clobbered;
}

/* Emit an instruction to set up the frame pointer.  */
static void
expand_set_up_frame_pointer (void)
{
  int offset;

  if (!frame_pointer_needed)
    return;

  INITIAL_ELIMINATION_OFFSET (HARD_FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM,
			      offset);
  emit_frame_insn (gen_MOVEI (hard_frame_pointer_rtx,
			      plus_constant (stack_pointer_rtx, offset)));
}

/* Emit an instruction to adjust the stack pointer by an amount
   specified by ADJUST.  */
rtx
pdp10_gen_stack_adjust (adjust)
     rtx adjust;
{
  rtx insn;

  /* ADDRESS: Use addition in extended mode, ADJSP otherwise.  */
  if (TARGET_EXTENDED)
    {
      if (GET_CODE (adjust) == CONST_INT)
	insn = gen_MOVEI (stack_pointer_rtx,
			  gen_rtx_PLUS (Pmode, stack_pointer_rtx, adjust));
      else
	insn = gen_addsi3 (stack_pointer_rtx,
			   stack_pointer_rtx,
			   adjust);
    }
  else
    {
      insn = gen_ADJSP (stack_pointer_rtx, adjust);
    }

  return insn;
}

/* Emit function prologue instructions.  */
void
pdp10_expand_prologue ()
{
  int clobbered[FIRST_PSEUDO_REGISTER];
  int clobbered_size;
  int stack_adjust;
  int single_push;

  clobbered_size = clobbered_regs (clobbered);

  /* If only one word is to be saved on stack, it can be stored with a
     single PUSH instruction.  */
  single_push =
    clobbered_size == UNITS_PER_WORD
    && (optimize_size || !TARGET_EXTENDED)
    && current_function_outgoing_args_size == 0
    && current_function_pretend_args_size == 0
    && get_frame_size () == 0;

  stack_adjust =
    current_function_outgoing_args_size
    + get_frame_size ()
    + clobbered_size
    + current_function_pretend_args_size;
  if (!single_push && stack_adjust > 0)
    emit_frame_insn (pdp10_gen_stack_adjust (GEN_INT (stack_adjust / 4)));

  expand_save_or_restore_return_address (SAVE, clobbered_size);
  expand_save_or_restore_regs (SAVE, clobbered, clobbered_size, single_push);
  expand_set_up_frame_pointer ();
}

/* Emit function epilogue instructions.  Emit the final popj
   instruction only if POPJ is nonzero.  */
void
pdp10_expand_epilogue (popj)
     int popj;
{
  int clobbered[FIRST_PSEUDO_REGISTER];
  int clobbered_size;
  int stack_adjust;
  int single_pop;

  clobbered_size = clobbered_regs (clobbered);

  /* If only one word was saved on stack, it can be restored with a
     single POP instruction.  */
  single_pop =
    clobbered_size == UNITS_PER_WORD
    && (optimize_size || !TARGET_EXTENDED)
    && current_function_outgoing_args_size == 0
    && current_function_pretend_args_size == 0
    && get_frame_size () == 0;

  expand_save_or_restore_regs (RESTORE, clobbered, clobbered_size, single_pop);
  expand_save_or_restore_return_address (RESTORE, clobbered_size);

  stack_adjust =
    current_function_outgoing_args_size
    + get_frame_size ()
    + clobbered_size
    + current_function_pretend_args_size;
  if (!single_pop && stack_adjust > 0)
    emit_frame_insn (pdp10_gen_stack_adjust (GEN_INT (-stack_adjust / 4)));

  if (popj)
    emit_jump_insn (gen_POPJ_17 ());
}

const char *
pdp10_output_return ()
{
  if (lookup_attribute ("fastcall",
			TYPE_ATTRIBUTES (TREE_TYPE (current_function_decl))))
    return "jrst (?)";
  else
    return "popj 17,";
}


/**********************************************************************

	Unsorted

**********************************************************************/

static rtx	convert_local_pointer PARAMS ((rtx op0, int from,
					       int to, int stack));
static rtx	convert_global_pointer PARAMS ((rtx op0, int from,
						int to, int stack));
static int	noconv_charp PARAMS ((int bytesize, tree exp));
#if XKL_STUFF
static void	maybe_warn_about_pointer_conversion PARAMS ((tree, int, int));
#endif
static tree	strip_conversions PARAMS ((enum tree_code code, tree type,
					   tree exp));
static void	emit_cmp_and_jump_insn_1 PARAMS ((rtx x, rtx y,
						  enum machine_mode mode,
						  enum rtx_code comparison,
						  int unsignedp, rtx label));
extern void	prepare_cmp_insn PARAMS ((rtx *, rtx *, enum rtx_code *, rtx,
					  enum machine_mode *, int *,
					  enum can_compare_purpose));
static void	pdp10_emit_cmp_and_jump_insns PARAMS ((tree exp, rtx x, rtx y,
						       enum rtx_code
						       comparison,
						       rtx size,
						       enum machine_mode mode,
						       int unsignedp,
						       rtx label));

rtx pdp10_compare_op0, pdp10_compare_op1;

/* Print a number to the stdio stream STREAM.  X is the number to
   print, DIV is a divisor to divide by, BITS is the precision in
   number of bits.  ZERO is zero if zero should be printend, and
   nonzero if zero should be supressed.  */
static void
pdp10_print_number (stream, x, div, bits, zero)
     FILE *stream;
     HOST_WIDE_INT x;
     int div;
     int bits;
     int zero;
{
  HOST_WIDE_INT mask;

  if (x < 0)
    x -= div - 1;
  x /= div;

  if (bits < 0)
    {
      bits = -bits;
      if (x < 0)
	{
	  fputc ('-', stream);
	  x = -x;
	}
    }

  mask = HWINT (-1) << bits;
  if ((x & mask) != 0 && (x & mask) != mask)
    fprintf (stream, "pdp10.c:%d:TOOBIG:", __LINE__);

  x &= ~mask;

  if (zero == 0 || x > 0)
    {
      if (bits == 9)
	fprintf (stream, "%03llo", x);
      else
	fprintf (stream, HOST_WIDE_INT_PRINT_OCT, x);
    }
}

/* Return the expression EXP hiding inside NOP_EXPR and CONVERT_EXPR.  */
static tree
strip_conversions (code, type, exp)
     enum tree_code code;
     tree type;
     tree exp;
{
  if ((TREE_CODE (exp) == NOP_EXPR || TREE_CODE (exp) == CONVERT_EXPR)
      && ((type == ptrdiff_type_node && TREE_TYPE (exp) == ptrdiff_type_node)
	  || (code == MINUS_EXPR && POINTER_TYPE_P (type) &&
	      POINTER_TYPE_P (TREE_TYPE (exp)))))
    {
      while (TREE_CODE (exp) == NOP_EXPR || TREE_CODE (exp) == CONVERT_EXPR)
	exp = TREE_OPERAND (exp, 0);
    }
  return exp;
}

/* Expand RTL instructions from a binary parse tree expression.  */
rtx
pdp10_expand_binop (mode, binoptab, op0, op1, target, unsignedp, methods, exp)
     enum machine_mode mode;
     optab binoptab;
     rtx op0, op1;
     rtx target;
     int unsignedp;
     enum optab_methods methods;
     tree exp;
{
  enum tree_code code = TREE_CODE (exp);
  tree exp0, exp1, type0, type1;
  int ptr0, ptr1, add;

  /* GCC parses a pointer subtraction like this:
        T *x, *y;
        int n = x - y;
     as:
	int n = (ptrdiff_t)x - (ptrdiff_t)y;
     Therefore, we need to strip away the ptrdiff_t conversions from the
     operands in order to see their real types.  */

  exp0 = strip_conversions (code, TREE_TYPE (exp), TREE_OPERAND (exp, 0));
  type0 = TREE_TYPE (exp0);
  ptr0 = POINTER_TYPE_P (type0);

  if (TREE_CODE_CLASS (code) == '<' || TREE_CODE_CLASS (code) == '2')
    {
      exp1 = strip_conversions (code, TREE_TYPE (exp), TREE_OPERAND (exp, 1));
      type1 = TREE_TYPE (exp1);
      ptr1 = POINTER_TYPE_P (type1) && GET_CODE (op1) != CONST_INT;
      if (!ptr1 && TREE_CODE (exp1) == MINUS_EXPR)
	{
	  if (POINTER_TYPE_P (TREE_TYPE (TREE_OPERAND (exp1, 0))))
	    {
	      ptr1 = 1;
	      type1 = TREE_TYPE (TREE_OPERAND (exp1, 0));
	    }
	  else if (POINTER_TYPE_P (TREE_TYPE (TREE_OPERAND (exp1, 1))))
	    {
	      ptr1 = 1;
	      type1 = TREE_TYPE (TREE_OPERAND (exp1, 1));
	    }
	}
    }

  switch (code)
    {
    case PLUS_EXPR:
    case MINUS_EXPR:
      add = (code == PLUS_EXPR);
      if (GET_CODE (op0) == NEG)
	{
	  op0 = XEXP (op0, 0);
	  add = !add;
	}
      if (GET_CODE (op1) == NEG)
	{
	  op1 = XEXP (op1, 0);
	  add = !add;
	}
      if (!add && ptr0 && ptr1)
	return expand_subtract_pointers (mode, target, op0, type0, op1, type1);
      if (ptr0 || ptr1)
	{
	  if (POINTER_TYPE_P (TREE_TYPE (exp)))
	    {
	      type0 = TREE_TYPE (exp);
	      type1 = NULL_TREE;
	    }
	  return expand_add_or_sub_pointer (target, op0, type0, op1, type1,
					    add);
	}
      break;

    case PREINCREMENT_EXPR:
    case PREDECREMENT_EXPR:
    case POSTINCREMENT_EXPR:
    case POSTDECREMENT_EXPR:
      if (ptr0)
	return expand_add_or_sub_pointer (target, op0, type0, op1, type1, 1);
      break;

    default:
      break;
    }

  return expand_binop (mode, binoptab, op0, op1, target, unsignedp, methods);
}

#define emit_TLZ(X, Y, Z) \
  emit_insn (gen_andsi3 (X, Y, gen_int_mode (~(HWINT (Z) << 18), SImode)))
#define emit_TLO(X, Y, Z) \
  emit_insn (gen_iorsi3 (X, Y, gen_int_mode (HWINT (Z) << 18, SImode)))
#define emit_TLC(X, Y, Z) \
  emit_insn (gen_xorsi3 (X, Y, gen_int_mode (HWINT (Z) << 18, SImode)))
#define emit_ADD(X, Y, Z) \
  emit_insn (gen_addsi3 (X, Y, gen_int_mode (HWINT (Z) << 18, SImode)))
#define emit_SUB(X, Y, Z) \
  emit_insn (gen_subsi3 (X, Y, gen_int_mode (HWINT (Z) << 18, SImode)))
#define emit_TLNE(X, Y, L) \
  emit_jump_insn (gen_test_and_skip (GEN_EQ, X, gen_int_mode (HWINT (Y)<<18, SImode), L))
#define emit_TLNN(X, Y, L) \
  emit_jump_insn (gen_test_and_skip (GEN_NE, X, gen_int_mode (HWINT (Y)<<18, SImode), L))
#define emit_TLZN(X, Y, L) \
  emit_jump_insn (gen_TLZN (X, gen_int_mode (HWINT (Y) << 18, SImode), L))

/* Convert a global pointer in OP0 with byte size FROM to a global
   pointer with byte size TO.  Byte size 36 means word pointer.
   Return the result.  */
static rtx
convert_global_pointer (op0, from, to, byte)
     rtx op0;
     int from;
     int to;
     int byte;
{
  rtx label;

  switch (to)
    {
    case 6:
      switch (from)
	{
	case 7:
	  emit_SUB (op0, op0, 0140000);
	  break;
	case 8:
	  emit_SUB (op0, op0, 0070000);
	  break;
	case 9:
	  emit_SUB (op0, op0, 0220000);
	  break;
	case 36:
	  emit_TLO (op0, op0, 0460000 + byte * 0010000);
	  break;
	default:
	  abort ();
	}
      break;

    case 7:
      switch (from)
	{
	case 8:
	  emit_ADD (op0, op0, 0050000);
	  break;
	case 9:
	  emit_SUB (op0, op0, 0060000);
	  break;
	case 36:
	  emit_TLO (op0, op0, 0620000 + byte * 0010000);
	  break;
	default:
	  abort ();
	}
      break;

    case 8:
      switch (from)
	{
	case 9:
	  emit_SUB (op0, op0, 0130000);
	  break;
	case 18:
	  emit_TLO (op0, op0, 0010000);
	  emit_TLZ (op0, op0, 0200000);
	  break;
	case 36:
	  emit_TLO (op0, op0, 0550000 + byte * 0010000);
	  break;
	default:
	  abort ();
	}
      break;

    case 9:
      switch (from)
	{
	case 7:
	  emit_ADD (op0, op0, 0060000);
	  break;
	case 8:
	  emit_ADD (op0, op0, 0130000);
	  break;
	case 18:
	  emit_TLZ (op0, op0, 0050000);
	  break;
	case 36:
	  emit_TLO (op0, op0, 0700000 + byte * 0010000);
	  break;
	default:
	  abort ();
	}
      break;

    case 18:
      switch (from)
	{
	case 8:
	  emit_ADD (op0, op0, 0170000);
	  emit_insn (gen_TLNE_TLZA_TLO (op0, GEN_INT (0020000),
					GEN_INT (0010000), GEN_INT (0010000)));
	  break;
	case 9:
	  emit_TLO  (op0, op0, 0050000);
	  emit_TLNE (op0, 0020000, label = gen_label_rtx ());
	  emit_TLZ  (op0, op0, 0010000);
	  emit_label (label);
	  break;
	case 36:
	  emit_TLO (op0, op0, 0750000 + byte * 0010000);
	  break;
	default:
	  abort ();
	}
      break;

    case 36:
      emit_TLZ (op0, op0, 0770000);
      break;

    default:
      abort ();
    }

  return op0;
}

/* Convert a local pointer in OP0 with byte size FROM to a local
   pointer with byte size TO.  Byte size 36 means word pointer.
   Return the result.  */
static rtx
convert_local_pointer (op0, from, to, byte)
     rtx op0;
     int from;
     int to;
     int byte;
{
  rtx reg = gen_reg_rtx (Pmode);
  rtx label;
  rtx temp;

  switch (to)
    {
    case 6:
      switch (from)
	{
	case 7:
	  emit_TLZ  (reg, op0, 0010100);
	  emit_ADD  (reg, reg, 0020000);
	  emit_TLNN (reg, 0160000, label = gen_label_rtx ());
	  emit_TLO  (reg, reg, 0020000);
	  emit_label (label);
	  emit_TLNN (reg, 0200000, label = gen_label_rtx ());
	  emit_TLC  (reg, reg, 0060000);
	  emit_label (label);
	  break;
	case 8:
	  emit_TLC  (reg, op0, 0001600);
	  emit_TLZ  (reg, reg, 0060000);
	  emit_TLNE (reg, 0200000, label = gen_label_rtx ());
	  emit_TLO  (reg, reg, 0020000);
	  emit_label (label);
	  emit_TLNE (reg, 0100000, label = gen_label_rtx ());
	  emit_TLO  (reg, reg, 0040000);
	  emit_label (label);
	  break;
	case 9:
	  emit_TLC  (reg, op0, 0041700);
	  emit_TLZN (reg, 0010000, label = gen_label_rtx ());
	  emit_TLO  (reg, reg, 0040000);
	  emit_label (label);
	  break;
	case 36:
	  emit_TLO (reg, op0, 0360600 - byte * 0060000);
	  break;
	default:
	  abort ();
	}
      break;

    case 7:
      switch (from)
	{
	case 8:
	  emit_TLC (reg, op0, 0001700);
	  emit_TLZ (reg, op0, 0070000);
	  temp = gen_reg_rtx (Pmode);
	  emit_move_insn (temp, reg);
	  emit_insn (gen_rotlsi3 (temp, temp, GEN_INT (3)));
	  emit_insn (gen_ANDCx (temp, temp, GEN_INT (7)));
	  emit_insn (gen_addsi3 (temp, temp, GEN_INT (1)));
	  emit_insn (gen_rotrsi3 (temp, temp, GEN_INT (6)));
	  emit_insn (gen_iorsi3 (reg, reg, temp));
	  break;
	case 9:
	  emit_TLC (reg, op0, 0071600);
	  emit_ADD (reg, reg, 0010000);
	  break;
	case 36:
	  emit_TLO (reg, op0, 0350700 - byte * 0070000);
	  break;
	default:
	  abort ();
	}
      break;

    case 8:
      switch (from)
	{
	case 9:
	  emit_TLO (reg, op0, 0040000);
	  emit_TLZ (reg, reg, 0030100);
	  break;
	case 18:
	  emit_TLO (reg, op0, 0141000);
	  emit_TLZ (reg, reg, 0022200);
	  break;
	case 36:
	  emit_TLO (reg, op0, 0341000 - byte * 0100000);
	  break;
	default:
	  abort ();
	}
      break;

    case 9:
      switch (from)
	{
	case 8:
	  emit_TLC  (reg, op0, 0040100);
	  emit_TLNE (reg, 0100000, label = gen_label_rtx ());
	  emit_TLO  (reg, reg, 0010000);
	  emit_label (label);
	  emit_TLNE (reg, 0200000, label = gen_label_rtx ());
	  emit_TLO  (reg, reg, 0020000);
	  emit_label (label);
	  break;
	case 18:
	  emit_TLC (reg, op0, 0113300);
	  break;
	case 36:
	  emit_TLO (reg, op0, 0331100 - byte * 0110000);
	  break;
	default:
	  abort ();
	}
      break;

    case 18:
      switch (from)
	{
	case 8:
	  emit_TLZ  (reg, op0, 0141000);
	  emit_TLO  (reg, reg, 0002200);
	  emit_TLNE (reg, 0200000, label = gen_label_rtx ());
	  emit_TLO  (reg, reg, 0020000);
	  emit_label (label);
	  break;
	case 9:
	  emit_TLC (reg, op0, 0003300);
	  emit_TLZ (reg, reg, 0110000);
	  break;
	case 36:
	  emit_TLO (reg, op0, 0222200 - byte * 0220000);
	  break;
	default:
	  abort ();
	}
      break;

    case 36:
      emit_TLZ (reg, op0, 0777777);
      break;

    default:
      abort ();
    }

  return reg;
}

static int
noconv_charp (bytesize, exp)
     int bytesize;
     tree exp;
{
  tree type, id;
  const char *name;

  if (bytesize < 6 || bytesize > 9)
    return 0;

  type = TREE_TYPE (TREE_TYPE (exp));
  if (TREE_CODE (type) == ARRAY_TYPE)
    type = TREE_TYPE (type);

  return TYPE_MODE (type) == QImode;

  id = TYPE_NAME (type);
  if (id == NULL_TREE)
    return 0;
  if (TREE_CODE (id) == TYPE_DECL)
    id = DECL_NAME (id);

  name = IDENTIFIER_POINTER (id);
  if (strcmp (name, "char") == 0
      || strcmp (name, "signed char") == 0
      || strcmp (name, "unsigned char") == 0)
    return 1;

  return 0;
}

#if XKL_STUFF
static void
maybe_warn_about_pointer_conversion (exp, from, to)
     tree exp;
     int from;
     int to;
{
  static int last_lineno = -1;

  if (exp == NULL ||
      VOID_TYPE_P (TREE_TYPE (TREE_TYPE (TREE_OPERAND (exp, 0)))))
    return;

  if (from <= 18 && to > from && to >= 18 && lineno > last_lineno)
    {
      if (to == 36)
	warning ("converting %d-bit byte pointer to word address", from);
      else
	warning ("converting %d-bit byte pointer to %d-bit byte pointer",
		 from, to);
    }

  last_lineno = lineno;
}
#else
#define maybe_warn_about_pointer_conversion(EXP, FROM, TO)
#endif

/* Emit instructions to perform the pointer conversion in EXP.  Store
   the result in TARGET if convenient, and return the result.  If OP0
   is not NULL_RTX, it's RTL for the unconverted pointer.  */
rtx
pdp10_convert_pointer (exp, op0, target, mod, stack, not_null)
     tree exp;
     rtx op0;
     rtx target ATTRIBUTE_UNUSED;
     int mod;
     int stack;
     int not_null;
{
  enum expand_modifier modifier = mod;
  int from, to, byte = 0, emit_jump;
  rtx label = gen_label_rtx ();

  if (stack)
    not_null = 1;

  if (modifier == EXPAND_INITIALIZER)
    return NULL_RTX;

#ifdef ASM_NETBSD
  if (TREE_CODE (TREE_TYPE (exp)) == INTEGER_TYPE
      && POINTER_TYPE_P (TREE_TYPE (TREE_OPERAND (exp, 0))))
    {
      rtx temp1, temp2;
      int size, shift, base;

      if (!op0)
	op0 = expand_expr (TREE_OPERAND (exp, 0), NULL_RTX, Pmode, 0);

      size = pdp10_bytesize (TREE_TYPE (TREE_TYPE (TREE_OPERAND (exp, 0))));

      if (size == 36)
	return expand_shift (LSHIFT_EXPR, Pmode, op0, build_int_2 (2, 0),
			     NULL_RTX, 1);

      shift = 30 - (size == 18);
      base = ps_base_for_bytesize (size);

      temp1 = expand_shift (RSHIFT_EXPR, Pmode, op0, build_int_2 (shift, 0),
			    NULL_RTX, 1);
      temp1 = expand_binop (Pmode, sub_optab, temp1, GEN_INT (base), NULL_RTX,
			    1, OPTAB_LIB_WIDEN);
      temp1 = expand_binop (Pmode, and_optab, temp1, GEN_INT (0777777),
			    NULL_RTX, 1, OPTAB_LIB_WIDEN);
      temp2 = expand_shift (LSHIFT_EXPR, Pmode, op0, build_int_2 (2, 0),
			    NULL_RTX, 1);
      temp2 = expand_binop (Pmode, add_optab, temp2, temp1, NULL_RTX, 1,
			    OPTAB_LIB_WIDEN);
      temp2 = expand_binop (Pmode, and_optab, temp2, GEN_INT (07777777777),
			    NULL_RTX, 1, OPTAB_LIB_WIDEN);

      return temp2;
    }
  else if (POINTER_TYPE_P (TREE_TYPE (exp))
	   && TREE_CODE (TREE_TYPE (TREE_OPERAND (exp, 0))) == INTEGER_TYPE)
    {
      rtx temp1, temp2;
      int size, shift, base;

      if (!op0)
	op0 = expand_expr (TREE_OPERAND (exp, 0), NULL_RTX, Pmode, 0);

      size = pdp10_bytesize (TREE_TYPE (TREE_TYPE (exp)));

      if (size == 36)
	return expand_shift (RSHIFT_EXPR, Pmode, op0, build_int_2 (2, 0),
			     NULL_RTX, 1);

      shift = 30 - (size == 18);
      base = ps_base_for_bytesize (size);

      temp1 = expand_binop (Pmode, and_optab, op0, GEN_INT (3), NULL_RTX, 1,
			    OPTAB_LIB_WIDEN);
      temp1 = expand_binop (Pmode, add_optab, temp1, GEN_INT (base), NULL_RTX,
			    1, OPTAB_LIB_WIDEN);
      temp1 = expand_shift (LSHIFT_EXPR, Pmode, temp1, build_int_2 (shift, 0),
			    NULL_RTX, 1);
      temp2 = expand_shift (RSHIFT_EXPR, Pmode, op0, build_int_2 (2, 0),
			    NULL_RTX, 1);
      temp2 = expand_binop (Pmode, ior_optab, temp2, temp1, NULL_RTX, 1,
			    OPTAB_LIB_WIDEN);

      return temp2;
    }
#endif

  if (!(POINTER_TYPE_P (TREE_TYPE (exp))
	 && (stack || POINTER_TYPE_P (TREE_TYPE (TREE_OPERAND (exp, 0))))))
    return NULL_RTX;

  if (target_flags & OPT_VOIDP)
    {
      /* The XKL people want conversions to or from void * to do
         nothing.  */
      if (TREE_CODE (exp) == NOP_EXPR
	  && (VOID_TYPE_P (TREE_TYPE (TREE_TYPE (exp)))
	      || VOID_TYPE_P (TREE_TYPE (TREE_TYPE (TREE_OPERAND (exp, 0))))))
	return NULL_RTX;
    }

  to = pdp10_bytesize (TREE_TYPE (TREE_TYPE (exp)));
  if (stack)
    from = 36;
  else
    {
      from = pdp10_bytesize (TREE_TYPE (TREE_TYPE (TREE_OPERAND (exp, 0))));
      if (to == from)
	return NULL_RTX;
    }

  /* If converting an "((un)signed) charN *" to or from an
     "((un)signed) char *", do nothing.  */
  if ((from <= 9 && noconv_charp (to, exp))
      || (to <= 9 && noconv_charp (from, TREE_OPERAND (exp, 0))))
    return NULL_RTX;

  /* When taking the address of a scalar variable, refer to the
     right-most byte.  */
  if (TREE_CODE (exp) == ADDR_EXPR
      && !AGGREGATE_TYPE_P (TREE_TYPE (TREE_OPERAND (exp, 0))))
    {
      /* Scalar char variables of all sizes are referred to as the
	 right half of a word.  */
      if (to <= 9)
	to = 18;
      byte = BITS_PER_WORD / to - 1;
    }

  if (!op0)
    op0 = expand_expr (TREE_OPERAND (exp, 0), NULL_RTX, Pmode, 0);

  if (from == to)
    return op0;

  if (CONSTANT_ADDRESS_P (op0))
    {
      HOST_WIDE_INT offset = 0, ps;
      rtx x = op0;

      /* ADDRESS: Support for local addressing not yet done.  */
      if (!TARGET_EXTENDED)
	abort ();

      if (GET_CODE (x) == CONST)
	x = XEXP (x, 0);

      if (GET_CODE (x) == PLUS)
	{
	  offset = INTVAL (XEXP (x, 1));
	  x = XEXP (x, 0);
	}

      ps = (offset >> 30) & 077;
      offset &= ADDRESS_MASK;

      from = bytesize_for_ps (ps);

      maybe_warn_about_pointer_conversion (exp, from, to);

      switch (from)
	{
	case 7:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + (ps - 062);
	      break;
	    case 7:
	      break;
	    case 8:
	      if (ps == 066)
		ps = 060;
	      else
		ps = 055 + (ps - 062);
	      break;
	    case 9:
	      if (ps == 066)
		ps = 073;
	      else
		ps = 070 + (ps - 062);
	      break;
	    case 18:
	      if (ps < 064)
		ps = 075;
	      else
		ps = 076;
	      break;
	    case 36:
	      ps = 0;
	      break;
	    default:
	      abort ();
	    }
	  break;

	case 8:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + (ps - 055);
	      break;
	    case 7:
	      ps = 062 + (ps - 055);
	      break;
	    case 8:
	      break;
	    case 9:
	      ps = 070 + (ps - 055);
	      break;
	    case 18:
	      ps = 075 + (ps - 055) / 2;
	      break;
	    case 36:
	      ps = 0;
	      break;
	    default:
	      abort ();
	    }
	  break;

	case 9:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + (ps - 070);
	      break;
	    case 7:
	      ps = 062 + (ps - 070);
	      break;
	    case 8:
	      ps = 055 + (ps - 070);
	      break;
	    case 9:
	      break;
	    case 18:
	      ps = 075 + (ps - 070) / 2;
	      break;
	    case 36:
	      ps = 0;
	      break;
	    default:
	      abort ();
	    }
	  break;

	case 18:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + (ps - 075) * 2;
	      break;
	    case 7:
	      ps = 062 + (ps - 075) * 2;
	      break;
	    case 8:
	      ps = 055 + (ps - 075) * 2;
	      break;
	    case 9:
	      ps = 070 + (ps - 075) * 2;
	      break;
	    case 18:
	      break;
	    case 36:
	      ps = 0;
	      break;
	    default:
	      abort ();
	    }
	  break;

	case 36:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + byte;
	      break;
	    case 7:
	      ps = 062 + byte;
	      break;
	    case 8:
	      ps = 055 + byte;
	      break;
	    case 9:
	      ps = 070 + byte;
	      break;
	    case 18:
	      ps = 075 + byte;
	      break;
	    case 36:
	      break;
	    default:
	      abort ();
	    }
	  break;

	default:
	  abort ();
	}

      return
	plus_constant (x, trunc_int_for_mode (offset + (ps << 30), Pmode));
    }

  {
    rtx old = op0;
    op0 = gen_reg_rtx (Pmode);
    emit_move_insn (op0, old);
  }

  emit_jump = !(not_null
		|| to == 36
		|| (to == 9 && from == 18 && TARGET_EXTENDED));
  if (emit_jump)
    emit_jump_insn (gen_cbranchsi (label, op0, GEN_INT (0), GEN_EQ));

  maybe_warn_about_pointer_conversion (exp, from, to);

  if (TARGET_EXTENDED)
    op0 = convert_global_pointer (op0, from, to, byte);
  else
    {
      if (stack)
	emit_insn (gen_andsi3 (op0, op0, GEN_INT (0777777)));
      op0 = convert_local_pointer (op0, from, to, byte);
    }

  if (emit_jump)
    emit_label (label);

  return op0;
}

/* Emit instructions to convert the pointer in EXP or OP0 (at least
   one must be non-NULL) from a pointer to bytes of FROM bits to a
   pointer to bytes of TO bits.  Store the result in TARGET if
   convenient, and return the result.  */
rtx
pdp10_convert_ptr (exp, op0, target, from, to, not_null)
     tree exp;
     rtx op0;
     rtx target ATTRIBUTE_UNUSED;
     int from;
     int to;
     int not_null;
{
  int byte = 0, emit_jump;
  rtx label = gen_label_rtx ();

  /* When taking the address of a scalar variable, refer to the
     right-most byte.  */
  if (exp && TREE_CODE (exp) == ADDR_EXPR
      && !AGGREGATE_TYPE_P (TREE_TYPE (TREE_OPERAND (exp, 0))))
    {
      if (to == 9)
	to = 18;
      byte = BITS_PER_WORD / to - 1;
    }

  if (!op0)
    op0 = expand_expr (TREE_OPERAND (exp, 0), NULL_RTX, Pmode, 0);

  if (from == to)
    return op0;

  if (from == 16)
    from = 18;
  else if (from == 32)
    from = 36;

  if (to == 16)
    to = 18;
  else if (to == 32)
    to = 36;

  if (CONSTANT_ADDRESS_P (op0))
    {
      HOST_WIDE_INT offset = 0, ps;
      rtx x = op0;

      /* ADDRESS: support for local addressing not yet done.  */
      if (!TARGET_EXTENDED)
	abort ();

      if (GET_CODE (x) == CONST)
	x = XEXP (x, 0);

      if (GET_CODE (x) == PLUS)
	{
	  offset = INTVAL (XEXP (x, 1));
	  x = XEXP (x, 0);
	}

      ps = (offset >> 30) & 077;
      offset &= ADDRESS_MASK;

      from = bytesize_for_ps (ps);
      if (from == to)
	return op0;

      maybe_warn_about_pointer_conversion (exp, from, to);

      switch (from)
	{
	case 7:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + (ps - 062);
	      break;
	    case 8:
	      if (ps == 066)
		ps = 060;
	      else
		ps = 055 + (ps - 062);
	      break;
	    case 9:
	      if (ps == 066)
		ps = 073;
	      else
		ps = 070 + (ps - 062);
	      break;
	    case 36:
	      ps = 0;
	      break;
	    default:
	      abort ();
	    }
	  break;

	case 8:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + (ps - 055);
	      break;
	    case 7:
	      ps = 062 + (ps - 055);
	      break;
	    case 9:
	      ps = 070 + (ps - 055);
	      break;
	    case 36:
	      ps = 0;
	      break;
	    default:
	      abort ();
	    }
	  break;

	case 9:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + (ps - 070);
	      break;
	    case 7:
	      ps = 062 + (ps - 070);
	      break;
	    case 8:
	      ps = 055 + (ps - 070);
	      break;
	    case 18:
	      ps = 075 + (ps - 070) / 2;
	      break;
	    case 36:
	      ps = 0;
	      break;
	    default:
	      abort ();
	    }
	  break;

	case 18:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + (ps - 075) * 2;
	      break;
	    case 7:
	      ps = 062 + (ps - 075) * 2;
	      break;
	    case 8:
	      ps = 055 + (ps - 075) * 2;
	      break;
	    case 9:
	      ps = 070 + (ps - 075) * 2;
	      break;
	    case 36:
	      ps = 0;
	      break;
	    default:
	      abort ();
	    }
	  break;

	case 36:
	  switch (to)
	    {
	    case 6:
	      ps = 046 + byte;
	      break;
	    case 7:
	      ps = 062 + byte;
	      break;
	    case 8:
	      ps = 055 + byte;
	      break;
	    case 9:
	      ps = 070 + byte;
	      break;
	    case 18:
	      ps = 075 + byte;
	      break;
	    default:
	      abort ();
	    }
	  break;

	default:
	  /* TODO: 6,7,8-bit bytes.  */
	  abort ();
	}

      return
	plus_constant (x, trunc_int_for_mode (offset + (ps << 30), Pmode));
    }

  {
    rtx old = op0;
    op0 = gen_reg_rtx (Pmode);
    emit_move_insn (op0, old);
  }

  emit_jump = !(not_null
		|| to == 36
		|| (to == 9 && from == 18 && TARGET_EXTENDED));
  if (emit_jump)
    emit_jump_insn (gen_cbranchsi (label, op0, GEN_INT (0), GEN_EQ));

  maybe_warn_about_pointer_conversion (exp, from, to);

  if (TARGET_EXTENDED)
    op0 = convert_global_pointer (op0, from, to, byte);
  else
    op0 = convert_local_pointer (op0, from, to, byte);

  if (emit_jump)
    emit_label (label);

  return op0;
}

/* Expand the tree ADDR_EXP tree expression in EXP into RTL instructions.
   Store the result in TARGET if convenient, and return the result.  */
rtx
pdp10_expand_ADDR_EXP (exp, type, target, ignore, mod)
     tree exp;
     tree type;
     rtx target ATTRIBUTE_UNUSED;
     int ignore;
     int mod;
{
  enum expand_modifier modifier = mod;
  rtx op0, temp;

  /* If nonzero, TEMP will be set to the address of something that might
     be a MEM corresponding to a stack slot.  */
  temp = 0;

  /* Are we taking the address of a nested function?  */
  if (TREE_CODE (TREE_OPERAND (exp, 0)) == FUNCTION_DECL
      && decl_function_context (TREE_OPERAND (exp, 0)) != 0
      && !DECL_NO_STATIC_CHAIN (TREE_OPERAND (exp, 0))
      && !TREE_STATIC (exp))
    op0 = trampoline_address (TREE_OPERAND (exp, 0));
  /* If we are taking the address of something erroneous, just
     return a zero.  */
  else if (TREE_CODE (TREE_OPERAND (exp, 0)) == ERROR_MARK)
    return const0_rtx;
  else
    {
      /* We make sure to pass const0_rtx down if we came in with
	 ignore set, to avoid doing the cleanups twice for something.  */
      op0 = expand_expr (TREE_OPERAND (exp, 0),
			 ignore ? const0_rtx : NULL_RTX, VOIDmode,
			 (modifier == EXPAND_INITIALIZER
			  ? modifier : EXPAND_CONST_ADDRESS));

      /* If we are going to ignore the result, OP0 will have been set
	 to const0_rtx, so just return it.  Don't get confused and
	 think we are taking the address of the constant.  */
      if (ignore)
	return op0;

      /* Pass 1 for MODIFY, so that protect_from_queue doesn't get
	 clever and returns a REG when given a MEM.  */
      op0 = protect_from_queue (op0, 1);

      /* We would like the object in memory.  If it is a constant, we can
	 have it be statically allocated into memory.  For a non-constant,
	 we need to allocate some memory and store the value into it.  */

      if (CONSTANT_P (op0))
	op0 = force_const_mem (TYPE_MODE (TREE_TYPE (TREE_OPERAND (exp, 0))),
			       op0);
      else if (GET_CODE (op0) == MEM)
	{
	  mark_temp_addr_taken (op0);
	  temp = XEXP (op0, 0);
	}

      else if (GET_CODE (op0) == REG
	       || (GET_CODE (op0) == SUBREG
		   && GET_CODE (XEXP (op0, 0)) != ZERO_EXTRACT
		   && GET_CODE (XEXP (XEXP (op0, 0), 0)) != MEM)
	       || GET_CODE (op0) == CONCAT || GET_CODE (op0) == ADDRESSOF
	       || GET_CODE (op0) == PARALLEL)
	{
	  /* If this object is in a register, it must be not
	     be BLKmode.  */
	  tree inner_type = TREE_TYPE (TREE_OPERAND (exp, 0));
	  tree nt = build_qualified_type (inner_type,
					  (TYPE_QUALS (inner_type)
					   | TYPE_QUAL_CONST));
	  rtx memloc = assign_temp (nt, 1, 1, 1);

	  mark_temp_addr_taken (memloc);
	  if (GET_CODE (op0) == PARALLEL)
	    /* Handle calls that pass values in multiple non-contiguous
	       locations.  The Irix 6 ABI has examples of this.  */
	    emit_group_store (memloc, op0,
			      int_size_in_bytes (inner_type));
	  else
	    emit_move_insn (memloc, op0);
	  op0 = memloc;
	}

      if (GET_CODE (op0) == SUBREG && GET_CODE (XEXP (op0, 0)) == ZERO_EXTRACT)
	op0 = XEXP (XEXP (op0, 0), 0);

      if (GET_CODE (op0) != MEM)
	abort ();

      op0 = XEXP (op0, 0);

      if (modifier == EXPAND_INITIALIZER || CONSTANT_ADDRESS_P (op0))
	return op0;
    }

  switch (GET_CODE (op0))
    {
    case REG:
      if (REGNO_PTR_FRAME_P (REGNO (op0)))
	{
	  rtx x = gen_reg_rtx (Pmode);
	  emit_insn (gen_MOVEI (x, op0));
	  op0 = x;
	  break;
	}
      /* Fall through.  */
    case SUBREG:
    case MEM:
    case ADDRESSOF:
      break;
    case PLUS:
      {
	rtx temp = gen_reg_rtx (Pmode);
	if (!address_operand (op0, Pmode))
	  op0 = force_reg (Pmode, op0);
	emit_insn (gen_MOVEI (temp, op0));
	op0 = temp;
      }
      break;
    case ZERO_EXTRACT:
      if (GET_CODE (XEXP (op0, 0)) != MEM)
	abort ();
      op0 = XEXP (XEXP (op0, 0), 0);
      break;
    default:
      if (CONSTANT_P (op0))
	break;
      abort ();
    }

  if (GET_CODE (op0) != LABEL_REF)
    {
      rtx temp = pdp10_convert_pointer (exp, op0, NULL_RTX, EXPAND_NORMAL,
					1, 1);
      if (temp)
	op0 = temp;
    }

  if (flag_force_addr && GET_CODE (op0) != REG)
    op0 = force_reg (Pmode, op0);

  if (GET_CODE (op0) == REG
      && !REG_USERVAR_P (op0))
    mark_reg_pointer (op0, TYPE_ALIGN (TREE_TYPE (type)));

  /* If we might have had a temp slot, add an equivalent address
     for it.  */
  if (temp != 0)
    update_temp_slot_address (temp, op0);

  return op0;
}

/* Adjust the 12-bit P and S field (or PS code) in PS by *ADJUST
   bytes.  Return the new P and S field (or -1 on failure), and the
   store the number of words to adjust a word pointer in *ADJUST.  */
int
adjust_ps (ps, adjust)
     int ps;
     int *adjust;
{
  int s, n;

  if (ps >= 04600)
    {
      int base, oldps;

      if (ps & 077)
	return -1;

      ps = ps >> 6;
      switch (ps)
	{
	case 046: case 047: case 050: case 051:	case 052: case 053:
	  base = 046;
	  s = 6;
	  break;
	case 062: case 063: case 064: case 065: case 066:
	  base = 062;
	  s = 7;
	  break;
	case 055: case 056: case 057: case 060:
	  base = 055;
	  s = 8;
	  break;
	case 070: case 071: case 072: case 073:
	  base = 070;
	  s = 9;
	  break;
	case 075: case 076:
	  base = 075;
	  s = 18;
	  break;
	default:
	  return -1;
	}

      n = BITS_PER_WORD / s;
      oldps = ps - base;
      ps = (*adjust + oldps) % n;
      if (ps < 0)
	{
	  ps += n;
	  *adjust -= n;
	}
      else if (ps < oldps)
	{
	  *adjust += n;
	}
      ps += base;
      ps <<= 6;
    }
  else
    {
      int p = ps >> 6;
      s = ps & 077;

      if (!VALID_OWGBP_BYTE_SIZE_P (s)
	  || p % s != 0)
	return -1;

      n = BITS_PER_WORD / s;
      p -= s * (*adjust % n);
      if (p < 0)
	p += s;
      ps = (p << 6) + s;
    }

  *adjust /= n;
  return ps;
}

/* Called by the MD_SIMPLIFY_UNSPEC macro.  */
rtx
pdp10_simplify_unspec (x, insn)
     rtx x;
     rtx insn;
{
  switch (XINT (x, 1))
    {
    case UNSPEC_ADJBP:
      {
	HOST_WIDE_INT offset;
	rtx pointer = XVECEXP (x, 0, 0);
	rtx address = NULL_RTX;
	rtx adj = XVECEXP (x, 0, 1);
	int ps, adjust;

	/* If the pointer adjustment is a register, find the last
	   value that register was set to.  */
	if (GET_CODE (adj) == REG)
	  {
	    rtx i = insn;
	    if (insn == NULL_RTX)
	      break;
	    adj = pdp10_find_last_value (adj, &i);
	  }
	if (adj == NULL_RTX || GET_CODE (adj) != CONST_INT)
	  break;

	adjust = INTVAL (adj);
	if (adjust == 0)
	  return pointer;

	/* If the pointer to be adjusted is a register, find the last
	   value that register was set to.  */
	if (GET_CODE (pointer) == REG)
	  {
	    if (insn == NULL_RTX)
	      break;
	    pointer = pdp10_find_last_value (pointer, &insn);
	    if (pointer == NULL_RTX)
	      break;
	  }

	if (GET_CODE (pointer) == CONST)
	  pointer = XEXP (pointer, 0);

	/* The pointer should look like (plus ptr (const_int n)).
	   Extract the PS field and offset from n.  */
	if (GET_CODE (pointer) != PLUS
	    || GET_CODE (XEXP (pointer, 1)) != CONST_INT)
	  break;
	offset = INTVAL (XEXP (pointer, 1));
	ps = (offset >> 24) & (TARGET_EXTENDED ? 07700 : 07777);
	offset &= ADDRESS_MASK;
	pointer = XEXP (pointer, 0);

	if (offset & ((HOST_WIDE_INT)1 << 29))
	  {
	    ps += 0100;
	    offset ^= (HOST_WIDE_INT)1 << 29;
	    offset -= (HOST_WIDE_INT)1 << 29;
	  }

	/* If the pointer inside the plus expression is a register,
	   find the last value that register was set to.  */
	if (GET_CODE (pointer) == REG)
	  {
	    address = pdp10_find_last_value (pointer, &insn);
	    if (address == NULL_RTX
		|| GET_CODE (address) != UNSPEC
		|| XINT (address, 1) != UNSPEC_ADDRESS)
	      break;
	  }

	/* Adjust the PS field.  */
	ps = adjust_ps (ps, &adjust);
	if (ps == -1)
	  break;

	/* If there was a word offset, add it to the pointer.  */
	offset += adjust;
	//offset &= ADDRESS_MASK;
	if (offset)
	  pointer = plus_constant (pointer, offset);

	return plus_constant (pointer,
			      trunc_int_for_mode ((HOST_WIDE_INT)ps << 24,
						  Pmode));
      }
      break;
    }

  return NULL_RTX;
}

/* Add to *OFFSET the offset in storage units resulting from the array
   reference made by EXP.  INDEX is the array index, counting from
   zero.  UNIT_SIZE is the size of an array element, in storage units.
   If an offset in bits is desired, *BIT_OFFSET may be added to
   instead, but note that the sum must be a constant expression in
   this case.  */
void pdp10_md_array_offset (tree exp, tree index, tree unit_size,
			    tree *offset, tree *bit_offset)
{
  if (!TREE_CONSTANT (index) || !TREE_CONSTANT (unit_size))
    {
      if (tree_low_cst (unit_size, 1) >= UNITS_PER_WORD)
	unit_size = size_binop (TRUNC_DIV_EXPR,
				unit_size,
				convert (sizetype,
					 build_int_2 (UNITS_PER_WORD, 0)));
      else if (tree_low_cst (unit_size, 1) == 2)
	unit_size = size_binop (TRUNC_DIV_EXPR,
				unit_size,
				convert (sizetype,
					 build_int_2 (2, 0)));
#if 0
      *offset = size_binop (TRUNC_DIV_EXPR, *offset,
			    convert (sizetype,
				     build_int_2 (UNITS_PER_WORD, 0)));
#endif
      *offset = size_binop (PLUS_EXPR, *offset,
			    size_binop (MULT_EXPR,
					convert (sizetype, index),
					unit_size));
    }
  else
    {
      tree unit_bitsize = TYPE_SIZE (TREE_TYPE (exp));
      tree bits_per_word = convert (bitsizetype,
				    build_int_2 (BITS_PER_WORD, 0));
      tree leftover_bits = bitsize_zero_node;

      if (!TREE_CONSTANT (unit_bitsize))
	abort ();

      *bit_offset = size_binop (PLUS_EXPR, *bit_offset,
				size_binop (MULT_EXPR,
					    convert (bitsizetype,
						     index),
					    unit_bitsize));

      if (compare_tree_int (unit_bitsize, BITS_PER_WORD) == -1)
	leftover_bits = size_binop (FLOOR_MOD_EXPR, bits_per_word,
				    unit_bitsize);
      if (compare_tree_int (leftover_bits, 0) != 0)
	{
	  tree units_per_word = size_binop (FLOOR_DIV_EXPR,
					    bits_per_word,
					    unit_bitsize);
	  tree words = size_binop (FLOOR_DIV_EXPR,
				   convert (bitsizetype, index),
				   units_per_word);
	  *bit_offset = size_binop (PLUS_EXPR, *bit_offset,
				    size_binop (MULT_EXPR, words,
						leftover_bits));
	}
    }
}

static void
emit_cmp_and_jump_insn_1 (x, y, mode, comparison, unsignedp, label)
     rtx x, y;
     enum machine_mode mode;
     enum rtx_code comparison;
     int unsignedp;
     rtx label;
{
  rtx test = gen_rtx_fmt_ee (comparison, mode, x, y);
  enum mode_class class = GET_MODE_CLASS (mode);
  enum machine_mode wider_mode = mode;

  /* Try combined insns first.  */
  do
    {
      enum insn_code icode;
      PUT_MODE (test, wider_mode);

      if (label)
	{	  
	  icode = cbranch_optab->handlers[(int)wider_mode].insn_code;
	  
	  if (icode != CODE_FOR_nothing
	      && (*insn_data[icode].operand[0].predicate) (test, wider_mode))
	    {
	      x = prepare_operand (icode, x, 1, mode, wider_mode, unsignedp);
	      y = prepare_operand (icode, y, 2, mode, wider_mode, unsignedp);
	      emit_jump_insn (GEN_FCN (icode) (test, x, y, label));
	      return;
	    }
	}

      /* Handle some compares against zero.  */
      icode = (int) tst_optab->handlers[(int) wider_mode].insn_code;
      if (y == CONST0_RTX (mode) && icode != CODE_FOR_nothing)
	{
	  x = prepare_operand (icode, x, 0, mode, wider_mode, unsignedp);
	  emit_insn (GEN_FCN (icode) (x));
	  if (label)
	    emit_jump_insn ((*bcc_gen_fctn[(int) comparison]) (label));
	  return;
	}

      /* Handle compares for which there is a directly suitable insn.  */

      icode = (int) cmp_optab->handlers[(int) wider_mode].insn_code;
      if (icode != CODE_FOR_nothing)
	{
	  x = prepare_operand (icode, x, 0, mode, wider_mode, unsignedp);
	  y = prepare_operand (icode, y, 1, mode, wider_mode, unsignedp);
	  emit_insn (GEN_FCN (icode) (x, y));
	  if (label)
	    emit_jump_insn ((*bcc_gen_fctn[(int) comparison]) (label));
	  return;
	}

      if (class != MODE_INT && class != MODE_FLOAT
	  && class != MODE_COMPLEX_FLOAT)
	break;

      wider_mode = GET_MODE_WIDER_MODE (wider_mode);
    } while (wider_mode != VOIDmode);

  abort ();
}

static void
pdp10_emit_cmp_and_jump_insns (exp, x, y, comparison, size, mode, unsignedp,
			       label)
     tree exp;
     rtx x, y;
     enum rtx_code comparison;
     rtx size;
     enum machine_mode mode;
     int unsignedp;
     rtx label;
{
  rtx op0 = x, op1 = y;

  /* Swap operands and condition to ensure canonical RTL.  */
  if (swap_commutative_operands_p (x, y))
    {
      /* If we're not emitting a branch, this means some caller
         is out of sync.  */
      if (!label)
	abort ();

      op0 = y, op1 = x;
      comparison = swap_condition (comparison);
    }

#ifdef HAVE_cc0
  /* If OP0 is still a constant, then both X and Y must be constants.  Force
     X into a register to avoid aborting in emit_cmp_insn due to non-canonical
     RTL.  */
  if (CONSTANT_P (op0))
    op0 = force_reg (mode, op0);
#endif

  emit_queue ();
  prepare_cmp_insn (&op0, &op1, &comparison, size, &mode, &unsignedp,
		    ccp_jump);

  if (WORD_TYPE_P (TREE_TYPE (TREE_TYPE (TREE_OPERAND (exp, 0))))
      || comparison == EQ || comparison == NE)
    emit_cmp_and_jump_insn_1 (op0, op1, mode, comparison, unsignedp, label);
  else if (HAVE_CMPBP)
    {
      rtx comparison_rtx = gen_rtx_fmt_ee (comparison, VOIDmode, 0, 0);
      if (!register_operand (op0, Pmode))
	op0 = force_reg (Pmode, op0);
      if (!reg_or_mem_operand (op1, Pmode))
	op1 = force_reg (Pmode, op1);
      emit_jump_insn (gen_CMPBP (comparison_rtx, op0, op1, label));
    }
  else
    {
      rtx temp0 = gen_reg_rtx (Pmode);
      rtx temp1 = gen_reg_rtx (Pmode);
      rtx temp0a, temp1a;

      if (TARGET_EXTENDED)
	{
	  temp0a = force_reg (Pmode, op0);
	  temp1a = force_reg (Pmode, op1);
	}
      else
	{
	  temp0a = gen_reg_rtx (Pmode);
	  temp1a = gen_reg_rtx (Pmode);
	  emit_insn (gen_xorsi3 (temp0a, op0,
				 gen_int_mode (HWINT (077) << 30, SImode)));
	  emit_insn (gen_xorsi3 (temp1a, op1,
				 gen_int_mode (HWINT (077) << 30, SImode)));
	}

      emit_insn (gen_rotlsi3 (temp0, temp0a, GEN_INT (6)));
      emit_insn (gen_rotlsi3 (temp1, temp1a, GEN_INT (6)));
      emit_cmp_and_jump_insn_1 (temp0, temp1, mode, comparison, unsignedp,
				label);
    }
}

void
pdp10_compare_and_jump (exp, op0, op1, code, unsignedp, mode, size,
			if_false_label, if_true_label)
     tree exp;
     rtx op0, op1;
     enum rtx_code code;
     int unsignedp;
     enum machine_mode mode;
     rtx size;
     rtx if_false_label, if_true_label;
{
  rtx tem;
  int dummy_true_label = 0;

  if (!POINTER_TYPE_P (TREE_TYPE (TREE_OPERAND (exp, 0))))
    {
      do_compare_rtx_and_jump (op0, op1, code, unsignedp, mode, size,
			       if_false_label, if_true_label);
      return;
    }

  /* No need to make pointer comparisons unsigned.  */
  code = signed_condition (code);

  /* Reverse the comparison if that is safe and we want to jump if it is
     false.  */
  if (!if_true_label && !FLOAT_MODE_P (mode))
    {
      if_true_label = if_false_label;
      if_false_label = 0;
      code = reverse_condition (code);
    }

  /* If one operand is constant, make it the second one.  Only do this
     if the other operand is not constant as well.  */

  if (swap_commutative_operands_p (op0, op1))
    {
      tem = op0;
      op0 = op1;
      op1 = tem;
      code = swap_condition (code);
    }

  if (flag_force_mem)
    {
      op0 = force_not_mem (op0);
      op1 = force_not_mem (op1);
    }

  do_pending_stack_adjust ();

  if (GET_CODE (op0) == CONST_INT && GET_CODE (op1) == CONST_INT
      && (tem = simplify_relational_operation (code, mode, op0, op1)) != 0)
    {
      if (tem == const_true_rtx)
	{
	  if (if_true_label)
	    emit_jump (if_true_label);
	}
      else
	{
	  if (if_false_label)
	    emit_jump (if_false_label);
	}
      return;
    }

#if 0
  /* There's no need to do this now that combine.c can eliminate lots of
     sign extensions.  This can be less efficient in certain cases on other
     machines.  */

  /* If this is a signed equality comparison, we can do it as an
     unsigned comparison since zero-extension is cheaper than sign
     extension and comparisons with zero are done as unsigned.  This is
     the case even on machines that can do fast sign extension, since
     zero-extension is easier to combine with other operations than
     sign-extension is.  If we are comparing against a constant, we must
     convert it to what it would look like unsigned.  */
  if ((code == EQ || code == NE) && !unsignedp
      && GET_MODE_BITSIZE (GET_MODE (op0)) <= HOST_BITS_PER_WIDE_INT)
    {
      if (GET_CODE (op1) == CONST_INT
	  && (INTVAL (op1) & GET_MODE_MASK (GET_MODE (op0))) != INTVAL (op1))
	op1 = GEN_INT (INTVAL (op1) & GET_MODE_MASK (GET_MODE (op0)));
      unsignedp = 1;
    }
#endif

  if (!if_true_label)
    {
      dummy_true_label = 1;
      if_true_label = gen_label_rtx ();
    }

  pdp10_emit_cmp_and_jump_insns (exp, op0, op1, code, size, mode, unsignedp,
				 if_true_label);

  if (if_false_label)
    emit_jump (if_false_label);
  if (dummy_true_label)
    emit_label (if_true_label);
}
