/* Definitions for PDP-10 target machine.
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

#ifndef __PDP10_H__
#define __PDP10_H__

/* XKL-specific stuff.  */
#define XKL_STUFF 0


/**********************************************************************

	Index

**********************************************************************/

/*	Front Page
	Index
	To-do List
	Controlling the Compilation Driver, gcc
	Run-time Target Specification
	Storage Layout
	Layout of Source Language Data Types
	Register Usage
		Basic Characteristics of Registers
		Order of Allocation of Registers
		How Values Fit in Registers
	Register Classes
	Stack Layout and Calling Conventions
		Basic Stack Layout
		Registers That Address the Stack Frame
		Eliminating Frame Pointer and Arg Pointer
		Passing Function Arguments on the Stack
		Passing Arguments in Registers
		How Scalar Function Values Are Returned
		How Large Values Are Returned
		Function Entry and Exit
		Generating Code for Profiling
		Permitting Tail Calls
	Implementing the Varargs Macros
	Implicit Calls To Library Routines
	Addressing Modes
	Describing Relative Costs of Operations
	Dividing the Output into Sections
	Position Independent Code
	Defining the Output Assembler Language
		The Overall Framework of an Assembler File
		Output of Data
		Output of Uninitialized Variables
		Output and Generation of Labels
		Output of Assembler Instructions
		Output of Dispatch Tables
		Assembler Commands for Alignment
	Controlling Debugging Information Format
		Macros Affecting All Debugging Formats
		Specific Options for DBX Output
		Macros for SDB and DWARF Output
	Cross Compilation and Floating Point
	Miscellaneous Parameters
	External variable declarations  */


/**********************************************************************

	To-do List

**********************************************************************/

/* PROMOTE_FUNCTION_RETURN, FUNCTION_VALUE: promote QImode and HImode
   to SImode?

   FUNCTION_PROFILER
   MACHINE_STATE_SAVE
   MACHINE_STATE_RESTORE
   LEGITIMIZE_ADDRESS
   LEGITIMIZE_RELOAD_ADDRESS
   RTX_COSTS
   ADDRESS_COST
   BRANCH_COST
   ADJUST_COST
   
   CALLER_SAVE_PROFITABLE: tune the cost of allocating call-clobbered
   registers for pseudos.  */


/**********************************************************************

	Controlling the Compilation Driver, gcc

**********************************************************************/

/* If defined, a list of pairs of strings, the first of which is a
   potential command line target to the gcc driver program, and the
   second of which is a space-separated list of options with which to
   replace the first option.  */
#if XKL_STUFF
#define TARGET_OPTION_TRANSLATE_TABLE		\
  { "-g", "-g -O0 -mregparm=0" },		\
  { "-g1", "-g1 -O0 -mregparm=0" },		\
  { "-g2", "-g2 -O0 -mregparm=0" },		\
  { "-g3", "-g3 -O0 -mregparm=0" }
#endif

/* A C string constant that tells the GCC driver program options to
   pass to CPP.  It can also specify how to translate options you give
   to GCC into options for GCC to pass to the CPP.  */
#define CPP_SPEC "%(cpp_cpu_arch)"

#define CPP_CPU_ARCH_SPEC "\
%{march=166:-D__PDP10_166__} \
%{march=ka10:-D__PDP10_KA10__} \
%{march=ki10:-D__PDP10_KI10__} \
%{march=ks10:-D__PDP10_KS10__} \
%{march=kl10:-D__PDP10_KL10__} \
%{march=xkl1:-D__PDP10_XKL1__} \
%{march=xkl2:-D__PDP10_XKL2__} \
%{!march=*: \
 %{mcpu=166:-D__PDP10_166__} \
 %{mcpu=ka10:-D__PDP10_KA10__} \
 %{mcpu=ki10:-D__PDP10_KI10__} \
 %{mcpu=ks10:-D__PDP10_KS10__} \
 %{mcpu=kl10:-D__PDP10_KL10__} \
 %{mcpu=xkl1:-D__PDP10_XKL1__} \
 %{mcpu=xkl2:-D__PDP10_XKL2__} \
 %{!mcpu*:%(cpp_cpu_arch_default)}} \
%{mregparm=0:-D__REGPARM__=0} \
%{mregparm=1:-D__REGPARM__=1} \
%{mregparm=2:-D__REGPARM__=2} \
%{mregparm=3:-D__REGPARM__=3} \
%{mregparm=4:-D__REGPARM__=4} \
%{mregparm=5:-D__REGPARM__=5} \
%{mregparm=6:-D__REGPARM__=6} \
%{mregparm=7:-D__REGPARM__=7} \
%{!mregparm*:-D__REGPARM__=7} \
%{mlong-long-71bit:-D__LONG_LONG_71BIT__=1} \
"

/* FIXME: this should depend on TARGET_CPU_DEFAULT.  */
#define CPP_ARCH_DEFAULT_SPEC "-D__PDP10_XKL1__"

/* This macro defines names of additional specifications to put in the
   specs that can be used in various specifications like CC1_SPEC.
   Its definition is an initializer with a subgrouping for each
   command option.

   Each subgrouping contains a string constant, that defines the
   specification name, and a string constant that used by the GNU CC
   driver program.  */
#define EXTRA_SPECS						\
  { "cpp_cpu_arch",		CPP_CPU_ARCH_SPEC },		\
  { "cpp_cpu_arch_default",	CPP_ARCH_DEFAULT_SPEC },

/* Define this macro as a C expression for the initializer of an array
   of strings to tell the driver program which options are defaults
   for this target and thus do not need to be handled specially when
   using MULTILIB_OPTIONS.  */
#define MULTILIB_DEFAULTS { "mregparm=7" }


/**********************************************************************

	Run-time Target Specification

**********************************************************************/

/* Define this string to be a string constant containing '-D' options
   to define the pre-defined macros that identify this machine and
   system.  */
#ifndef CPP_PREDEFINES
#if XKL_STUFF
#define CPP_PREDEFINES  "-Dpdp10 -DPDP10 -Dtops20 -DTOPS20 -DGNUCC -DGCC -Acpu=pdp10 -Amachine=pdp10"
#else
#define CPP_PREDEFINES  "-Dpdp10 -Dtops20 -Acpu=pdp10 -Amachine=pdp10"
#endif
#endif

/* Run-time compilation parameters selecting different hardware
   subsets.  */
extern int target_flags;

/* Run-time compilation parameter selecting different instruction set
   tunings.  */
extern int target_tune;

/* Run-time compilation parameter selecting the code model to use.  */
extern int target_model;

/* This series of macros is to allow compiler command arguments to
   enable or disable the use of optional features of the target
   machine.  */

#define CPU_MASK	0xff
#define CPU_166		0x01	/* DEC Type 166.  */
#define CPU_KA10	0x02	/* DEC KA10.  */
#define CPU_KI10	0x03	/* DEC KI10.  */
#define CPU_KS10	0x04	/* DEC KS10.  */
#define CPU_KL10	0x05	/* DEC KL10.  */
#define CPU_XKL1	0x06	/* XKL XKL-1.  */
#define CPU_XKL2	0x07	/* XKL XKL-2.  */

#define MODEL_NONE	0	/* No code model selected.  */
#define MODEL_SMALL	1	/* Everything in section zero.  */
#define MODEL_SMALLISH	2	/* Most things in one section.  */
#define MODEL_MEDIUM	3	/* Reserved.  */
#define MODEL_LARGE	4	/* Everything in multiple sections.  */
/*#define MODEL_HUGE		   Multi-section functions.  */

#define OPT_EXTENDED	0x00000100	/* Extended addressing.  */
#define OPT_UNEXTENDED	0x00000200	/* Local addressing.  */
#define OPT_VOIDP	0x00000400	/* Don't convert to/from void *.  */
#define OPT_NOCLRSUBR	0x00000800	/* Don't clear subregs.  */
#define CPU_EXTENDED	0x00001000	/* Extended addressing.  */
#define CPU_GFLOAT	0x00002000	/* Giant format floating-point.  */
#define CPU_STRING	0x00004000	/* String instructions.  */
#define CPU_71BIT	0x00008000	/* Use double-word integers.  */
#define ASM_TAB		0x00010000	/* Tab between opcode and operands.  */
#define ASM_UPPER	0x00020000	/* Upper-case opcodes.  */
#define ASM_INDENT_SKIPPED 0x00040000	/* Indent skiped insns.  */
#define ASM_AC0		0x00080000	/* Output AC0 even if it's not used. */

#define TARGET_CPU	(target_flags & CPU_MASK)
#define TARGET_166	(TARGET_CPU == CPU_166)
#define TARGET_KA10	(TARGET_CPU == CPU_KA10)
#define TARGET_KI10	(TARGET_CPU == CPU_KI10)
#define TARGET_KL10	(TARGET_CPU == CPU_KL10)
#define TARGET_KS10	(TARGET_CPU == CPU_KS10)
#define TARGET_XKL1	(TARGET_CPU == CPU_XKL1)
#define TARGET_XKL2	(TARGET_CPU == CPU_XKL2)

#define TARGET_EXTENDED	(target_flags & CPU_EXTENDED)
#define TARGET_GFLOAT	(target_flags & CPU_GFLOAT)
/* String instructions aren't implemented, and their use is discouraged.  */
#define TARGET_STRING	0
#define TARGET_71BIT	(target_flags & CPU_71BIT)
#define TARGET_CACHE	TARGET_KL10up

#define TARGET_SMALL	(target_model == MODEL_SMALL)
#define TARGET_SMALLISH	(target_model == MODEL_SMALLISH)
#define TARGET_MEDIUM	(target_model == MODEL_MEDIUM)
#define TARGET_LARGE	(target_model == MODEL_LARGE)
#define TARGET_HUGE	0  /* Multi-section functions.  */

#define TARGET_KA10up	(TARGET_KA10 || TARGET_KI10up)
#define TARGET_KI10up	(TARGET_KI10 || TARGET_KL10up)
#define TARGET_KL10up	(TARGET_KL10 || TARGET_KS10 || TARGET_XKL1up)
#define TARGET_XKL1up	(TARGET_XKL1 || TARGET_XKL2)

#define TUNE_166	(target_tune == CPU_166)
#define TUNE_KA10	(target_tune == CPU_KA10)
#define TUNE_KI10	(target_tune == CPU_KI10)
#define TUNE_KL10	(target_tune == CPU_KL10)
#define TUNE_KS10	(target_tune == CPU_KS10)
#define TUNE_XKL1	(target_tune == CPU_XKL1)
#define TUNE_XKL2	(target_tune == CPU_XKL2)

/* Just in case configure has failed to define anything.  */
#ifndef TARGET_CPU_DEFAULT
#define TARGET_CPU_DEFAULT 0
#endif

/* This macro defines names of command options to set and clear bits
   in target_flags.  Its definition is an initializer with a
   subgrouping for each command option.  */
#define TARGET_SWITCHES							   \
{									   \
  { "extended",	    OPT_EXTENDED,   "Use extended addressing"		}, \
  { "unextended",   OPT_UNEXTENDED, "Don't use extended addressing"	}, \
  { "gfloat",	    CPU_GFLOAT,	    "Use giant-format doubles"		}, \
  { "dfloat",	   -CPU_GFLOAT,	    "Use double-format doubles"		}, \
  { "string",	    CPU_STRING,	    "Use string instructions"		}, \
  { "no-string",   -CPU_STRING,	    "Don't use string instructions"	}, \
  { "long-long-71bit", CPU_71BIT,   "Use 71-bit long long"		}, \
  { "long-long-72bit", -CPU_71BIT,  "Use 72-bit long long"		}, \
  { "tab",	    ASM_TAB,        "Tab between opcode and operands"	}, \
  { "upper",	    ASM_UPPER,	    "Upper-case opcodes"		}, \
  { "indent-skipped", ASM_INDENT_SKIPPED, "Indent skipped instructions"	}, \
  { "ac0",	    ASM_AC0,	    "Output AC0 even if it's not used"  }, \
  { "no-convert-voidp", OPT_VOIDP,  "Don't convert to/from void *"	}, \
  { "convert-voidp", -OPT_VOIDP,    "Do convert to/from void *"		}, \
  { "no-clear-subregs", OPT_NOCLRSUBR, "Don't clear subregs"		}, \
  { "clear-subregs",OPT_NOCLRSUBR,  "Clear subregs"			}, \
  { "",		    TARGET_CPU_DEFAULT, ""				}  \
}

#ifndef TARGET_DEFAULT
#define TARGET_DEFAULT  0
#endif

/* This macro is similar to TARGET_SWITCHES but defines names of
   command options that have values.  Its definition is an initializer
   with a subgrouping for each command option.  */
#define TARGET_OPTIONS							\
{									\
  {									\
    "cpu=",								\
    (const char **)&pdp10_cpu,						\
    "Specify the name of the target CPU"				\
  },									\
  {									\
    "arch=",								\
    (const char **)&pdp10_arch,						\
    "Specify the name of the target CPU to generate code for"		\
  },									\
  {									\
    "tune=",								\
    (const char **)&pdp10_tune,						\
    "Specify the name of the target CPU to tune code generation for"	\
  },									\
  {									\
    "text-psect=",							\
    (const char **)&pdp10_text_psect,					\
    "Specify the name of the PSECT to put code in"			\
  },									\
  {									\
    "rodata-psect=",							\
    (const char **)&pdp10_rodata_psect,					\
    "Specify the name of the PSECT to put read-only data in"		\
  },									\
  {									\
    "data-psect=",							\
    (const char **)&pdp10_data_psect,					\
    "Specify the name of the PSECT to put read-write data in"		\
  },									\
  {									\
    "bss-psect=",							\
    (const char **)&pdp10_bss_psect,					\
    "Specify the name of the PSECT to put uninitialized data in"	\
  },									\
  {									\
    "model=",								\
    (const char **)&pdp10_model,					\
    "Specify program model"						\
  },									\
  {									\
    "regparm=",								\
    (const char **)&pdp10_argument_regs,				\
    "Number of registers used to pass arguments"			\
  },									\
  {									\
    "return-regs=",							\
    (const char **)&pdp10_return_regs,					\
    "Number of registers used to return a value"			\
  },									\
  {									\
    "call-clobbered=",							\
    (const char **)&pdp10_call_clobbered,				\
    "Number of registers that are call clobbered"			\
  },									\
  {									\
    "string-bytesize=",							\
    (const char **)&pdp10_string_bytesize,				\
    "What byte size to use for string literals"				\
  },									\
  {									\
    "char-bytesize=",							\
    (const char **)&pdp10_char_bytesize_arg,				\
    "What byte size to use for the char type"				\
  }									\
}
extern char *pdp10_cpu;
extern char *pdp10_arch;
extern char *pdp10_tune;
extern char *pdp10_text_psect;
extern char *pdp10_rodata_psect;
extern char *pdp10_data_psect;
extern char *pdp10_bss_psect;
extern char *pdp10_model;
extern char *pdp10_argument_regs;
extern char *pdp10_return_regs;
extern char *pdp10_call_clobbered;
extern char *pdp10_string_bytesize;
extern char *pdp10_char_bytesize_arg;

/* This macro is a C statement to print on stderr a string describing
   the particular machine description choise.  */
#ifndef TARGET_VERSION
#define TARGET_VERSION fputs (" (PDP-10)", stderr);
#endif

/* Sometimes certain combinations of command options do not make sense
   on a particular target machine.  You can define a macro
   OVERRIDE_OPTIONS to take account of this.  This macro is executed
   once just after all the command options have been parsed.  */
#define OVERRIDE_OPTIONS pdp10_override_options ()

/* Define this macro if debugging can be performed even without a
   frame pointer.  If this macro is defined, GCC will turn on the
   -fomit-frame-pointer option whenever -O is specified.  */
#define CAN_DEBUG_WITHOUT_FP 1


/**********************************************************************

	Storage Layout

**********************************************************************/

/* Define this macro to have the value 1 if the most significant bit
   in a byte has the lowest number; otherwise define it to be zero.  */
#define BITS_BIG_ENDIAN 1

/* Define this macro to to have the value 1 if most significant byte
   of a word has the lowest number.  */
#define BYTES_BIG_ENDIAN 1

/* Define this macro to have the value 1 if, in a multiword object,
   the most significant word has the lowest number.  This applies to
   both memory locations and registers; GCC fundamentally assumes that
   the order of words in memory is the same as the order in registers.  */
#define WORDS_BIG_ENDIAN  1

/* Define this macro to be the number of bits in an addressable
   storage unit.  */
#define BITS_PER_UNIT 9

/* Number of bits in a word.  */
#define BITS_PER_WORD 36

/* Number of storage units in a word.  */
/*#define UNITS_PER_WORD 4*/
#define UNITS_PER_WORD (BITS_PER_WORD / BITS_PER_UNIT)

/* The horrible truth.  */
#define REAL_BITS_PER_UNIT 36
#define REAL_UNITS_PER_WORD 1

/* Number of bits per memory unit used to access a bitfield.  */
#define BITS_PER_BITFIELD_UNIT 36

/* Width of a pointer, in bits.  You must specify a value no wider
   than the width of Pmode.  If it is not equal to the width of Pmode,
   you must define POINTERS_EXTEND_UNSIGNED.  */
#define POINTER_SIZE 36

/* A macro to update MODE and UNSIGNEDP when an object whose type is
   TYPE and which has the specified mode and signedness is to be
   stored in a register.  This macro is only called when TYPE is a
   scalar type.  */
#if BITS_PER_UNIT == 9
#define PROMOTE_MODE(MODE, UNSIGNEDP, TYPE)	\
  if (GET_MODE_CLASS (MODE) == MODE_INT		\
      && GET_MODE_SIZE (MODE) < 4)      	\
    {						\
      UNSIGNEDP = 1;				\
      (MODE) = SImode;				\
    }
#else
#define PROMOTE_MODE(MODE, UNSIGNEDP, TYPE)	\
  if (GET_MODE_CLASS (MODE) == MODE_INT		\
      && GET_MODE_SIZE (MODE) < 1)      	\
    {						\
      UNSIGNEDP = 1;				\
      (MODE) = QImode;				\
    }
#endif

/* Define this macro if the promotion described by PROMOTE_MODE should
   also be done for outgoing function arguments.  */
#define PROMOTE_FUNCTION_ARGS

/* Define this macro if the promotion described by PROMOTE_MODE should
   also be done for the return value of functions.

   If this macro is defined, FUNCTION_VALUE must perform the same
   promotions done by PROMOTE_MODE. (FIXME: is this true?)  */
#define PROMOTE_FUNCTION_RETURN

/* Normal alignment required for function parameters on the stack, in
   bits.  */
#define PARM_BOUNDARY 36

/* Normal alignment required for stack variables, in bits.  */
#define STACKVAR_BOUNDARY 36

/* Define this macro if there is a guaranteed alignment for the stack
   pointer on this machine.  */
#define STACK_BOUNDARY 36

/* Alignment required for a function entry point, in bits.  */
#define FUNCTION_BOUNDARY 36

/* Biggest alignment that any data type can require on this machine,
   in bits.  */
#define BIGGEST_ALIGNMENT 36

/* A C expression to compute the alignment for a variable in the
   static store.  TYPE is the data type, and ALIGN is the alignment
   that the object would ordinarily have.  The value of this macro is
   used instead of that alignment to align the object.  */
#define DATA_ALIGNMENT(TYPE, ALIGN) \
  pdp10_data_alignment ((ALIGN), (TYPE))

#define DATA_PADDING(DECL, X) \
  pdp10_data_padding (DECL, X)

/* A C expression to compute the alignment given to a constant that is
   being placed in memory.  EXP is the constant and ALIGN is the
   alignment that the object would ordinarily have.  The value of this
   macro is used instead of that alignment to align the object.  */
#define CONSTANT_ALIGNMENT(EXP, ALIGN) \
  pdp10_constant_alignment ((ALIGN), (EXP))

/* A C expression to compute the alignment for a variable in the local
   store.  TYPE is the data type, and ALIGN is the alignment that the
   object would ordinarily have.  The value of this macro is used
   instead of that alignment to align the object.  */
#define LOCAL_ALIGNMENT(TYPE, ALIGN) \
  pdp10_local_alignment ((ALIGN), (TYPE))

/* A C expression to compute the size of a variable in the local
   store.  TYPE is the data type, and SIZE is the size that the
   object would ordinarily have.  */
#define LOCAL_SIZE(TYPE, SIZE) pdp10_local_size ((SIZE), (TYPE))

/* A C expression to generate some RTL code to align X, which is RTX
   for the stack pointer.  ALIGN is the alignment in storage units.  */
#define ALIGN_STACK_POINTER(X, ALIGN) \
  pdp10_align_stack_pointer ((X), (ALIGN))

/* Alignment in bits to be given to a structure bit field that follows
   an empty field such as int : 0;  */
#define EMPTY_FIELD_BOUNDARY  36

/* Number of bits which any structure or union's size must be a
   multiple of.  */
#define STRUCTURE_SIZE_BOUNDARY 36

/* Define this macro to be the value 1 if instructions will fail to
   work if given data not on the nominal alignment.  */
#define STRICT_ALIGNMENT 1

/* A code distinguishing the floating point format of the target
   machine.  */
#define TARGET_FLOAT_FORMAT PDP10_FLOAT_FORMAT


/**********************************************************************

	Layout of Source Language Data Types

**********************************************************************/

/* A C expression for the size in bits of the type @code{int} on the
   target machine.  If you don't define this, the default is one word.  */
#define INT_TYPE_SIZE 36

/* A C expression for the size in bits of the type @code{short} on the
   target machine.  If you don't define this, the default is half a word.  */
#define SHORT_TYPE_SIZE 18

/* A C expression for the size in bits of the type @code{long} on the
   target machine.  If you don't define this, the default is one word.  */
#define LONG_TYPE_SIZE 36

/* A C expression for the size in bits of the type @code{long long} on
   the target machine.  If you don't define this, the default is two
   words.  If you want to support GNU Ada on your machine, the value
   of this macro must be at least 64.  */
#define LONG_LONG_TYPE_SIZE 72

/* A C expression for the size in bits of the type @code{char} on the
   target machine.  If you don't define this, the default is
   BITS_PER_UNIT.  */
#if 1
#define CHAR_TYPE_SIZE 9
#else
#define CHAR_TYPE_SIZE pdp10_char_bytesize
#endif
extern int pdp10_char_bytesize;

/* A C expression for the size in bits of the type bool on the target
   machine.  */
#define BOOL_TYPE_SIZE 36

/* A C expression for the size in bits of the type @code{float} on the
   target machine.  If you don't define this, the default is one word.  */
#define FLOAT_TYPE_SIZE 36

/* A C expression for the size in bits of the type @code{double} on
   the target machine.  If you don't define this, the default is two
   words.  */
#define DOUBLE_TYPE_SIZE 72

/* An expression whose value is 1 or 0, according to whether the type
   char should be signed or unsigned by default.  */
#ifndef DEFAULT_SIGNED_CHAR
#define DEFAULT_SIGNED_CHAR 0
#endif


/**********************************************************************

	Register Usage

**********************************************************************/

/*
 * Basic Characteristics of Registers
 */

/* The number of hardware registers know to the compiler.  */
#define FIRST_PSEUDO_REGISTER 18

/* An initializer that says which registers are used for fixed
   purposes all throughout the compiled code and are therefore not
   available for general allocation.

   AC0 is reserved as a scratch register.  AC17 is the stack pointer.  */
#define FIXED_REGISTERS		\
{				\
  1, 0, 0, 0, 0, 0, 0, 0,	\
  0, 0, 0, 0, 0, 0, 0, 1,	\
  1, 1				\
}

/* Like FIXED_REGISTERS, but has 1 for each register that is clobbered
   (in general) by function calls as well as for fixed registers.

   The default is to clobber AC1 - AC4, which are used to pass
   arguments, and AC0.  */
#define CALL_USED_REGISTERS	\
{				\
  1, 1, 1, 1, 1, 1, 1, 1,	\
  0, 0, 0, 0, 0, 0, 0, 1,	\
  1, 1				\
}

/* Zero or more C statements that may conditionally modify five
   variables fixed_regs, call_used_regs, global_regs, reg_names, and
   reg_class_contents, to take into account any dependence of these
   register sets on target flags.  */
#define CONDITIONAL_REGISTER_USAGE \
  pdp10_conditional_register_usage ()

/*
 * Order of Allocation of Registers
 */

/* An initializer for a vector of integers, containing the numbers of
   hard registers in the order in which GCC should prefer to use them
   (from most preferred to last).  */
#define REG_ALLOC_ORDER			\
{					\
     6,  7,  5,  4,  3,  2,  1,  8,	\
     9, 10, 11, 12, 13, 14, 15,  0,	\
    16, 17				\
}

/*
 * How Values Fit in Registers
 */

/* A C expression for the number of consecutive hard registers,
   starting at register number REGNO, required to hold a value of mode
   MOD.  */
#define HARD_REGNO_NREGS(REGNO, MODE) \
  ((GET_MODE_SIZE (MODE) + UNITS_PER_WORD - 1) / UNITS_PER_WORD)

/* A C expression that is nonzero if it is permissible to store a
   value of mode MODE in hard register number REGNO (or in several
   registers starting with that one).  */
#define HARD_REGNO_MODE_OK(REGNO, MODE)	1

/* A C expression that is nonzero if a value of mode MODE1 is
   accessible in mode MODE2 without copying.  */
#define MODES_TIEABLE_P(MODE1, MODE2) \
  (GET_MODE_CLASS (MODE1) == GET_MODE_CLASS (MODE2))


/**********************************************************************

	Register Classes

**********************************************************************/

/* An enumeral type that must be defined with all the register class
   names as enumeral values.  NO_REGS must be first.  ALL_REGS must be
   the last register class, followed by one more enumeral value,
   LIM_REG_CLASSES, which is not a register class but rather tells how
   many classes there are.  One of the classes must be named
   GENERAL_REGS.  There is nothing terribly special about the name,
   but the operand constraint letters 'r' and 'g' specify this class.
   If GENERAL_REGS is the same as ALL_REGS, just define it as a macro
   which expands to ALL_REGS.  */
enum reg_class
{
  NO_REGS,
  INDEX_REGS,
  ALL_REGS,
  LIM_REG_CLASSES
};
#define GENERAL_REGS ALL_REGS

/* The number of distinct register classes.  */
#define N_REG_CLASSES (int) LIM_REG_CLASSES

/* An initializer containing the names of the register classes as C
   string constants.  */
#define REG_CLASS_NAMES	\
{			\
  "NO_REGS",		\
  "INDEX_REGS",		\
  "ALL_REGS",		\
}

/* An initializer containing the contents of the register classes, as
   integers which are bit masks.  */
#define REG_CLASS_CONTENTS  		\
{					\
  { 0x0000000 }, /* NO_REGS  */		\
  { 0x000FFFE }, /* INDEX_REGS */	\
  { 0x000FFFF }, /* ALL_REGS */		\
}

/* A C expression whose value is a register class containing the hard
   register REGNO.  In general there is more than one such class;
   choose a class which is minimal, meaning that no smaller class
   contains the register.  */
#define REGNO_REG_CLASS(REGNO) \
  ((REGNO) == 0 ? ALL_REGS : INDEX_REGS)

/* A macro whose definition is the name of the class to which a valid
   base register must belong.  A base register is one used in an
   address which is the register value plus a displacement.  */
#define BASE_REG_CLASS INDEX_REGS

/* A macro whose definition is the name of the class to which a valid
   index register must belong.  An index register is one used in an
   address where its value is either multiplied by a scale factor or
   added to another register.  */
#define INDEX_REG_CLASS INDEX_REGS

/* A C expression which defines the machine-dependent operand
   constraint letters for register classes.  */
#define REG_CLASS_FROM_LETTER(C)		\
  ((C) == 'x' ? INDEX_REGS :			\
   (C) == 'r' ? ALL_REGS :			\
   NO_REGS)

/* A C expression which is nonzero if register number REGNO is
   suitable for use as a base register in operand addresses.  */
#define REGNO_OK_FOR_BASE_P(REGNO) \
  REGNO_REG_CLASS (REGNO) == INDEX_REGS

/* A C expression which is nonzero if register number REGNO is
   suitable for use as an index register in operand addresses.  */
#define REGNO_OK_FOR_INDEX_P(REGNO) 0

/* A C expression that places additional restrictions on the register
   class to use when it is necessary to copy value X into a register
   in class CLASS.  */
#define PREFERRED_RELOAD_CLASS(X, CLASS) (CLASS)

/* A C expression for the maximum number of consecutive registers of
   class CLASS needed to hold a value of mode MODE.  */
#define CLASS_MAX_NREGS(CLASS, MODE) \
  ((GET_MODE_SIZE (MODE) + UNITS_PER_WORD - 1) / UNITS_PER_WORD)

/* A C expression that defines the machine-specific operand constraint
   letters ('I', 'J', 'K', ... 'P') that specify particular ranges of
   integer values.  */
#define CONST_OK_FOR_LETTER_P(VALUE, C) \
  pdp10_const_ok_for_letter_p ((VALUE), (C))

/* A C expression that defines the machine-dependent operand
   constraint letters that specify particular ranges of CONST_DOUBLE
   values ('G' or 'H').  */
#define CONST_DOUBLE_OK_FOR_LETTER_P(X, C) \
  pdp10_const_double_ok_for_letter_p ((X), (C))

/* A C expression that defines the optional machine-dependent
   constraint letters ('Q', 'R', 'S', 'T', 'U') that can be used to
   segregate specific types of operands, usually memory references,
   for the target machine.  */
#define EXTRA_CONSTRAINT(OP, C) pdp10_extra_constraint ((OP), (C))


/**********************************************************************

	Stack Layout and Calling Conventions

**********************************************************************/

/*
 * Basic Stack Layout
 */

/* Define this macro if successive arguments to a function occupy
   decreasing addresses on the stack.  */
#define ARGS_GROW_DOWNWARD 1

/* Offset from the frame pointer to the first local variable slot to
   be allocated.

   Find the next slot's offset by adding the length of the first slot
   to the value STARTING_FRAME_OFFSET.  */
#define STARTING_FRAME_OFFSET pdp10_starting_frame_offset ()

/* Offset from the stack pointer register to the location above the
   first location at which outgoing arguments are placed.  */
#define STACK_POINTER_OFFSET pdp10_stack_pointer_offset ()

/* Offset from the argument pointer register to the location above the
   first argument's address.  */
#define FIRST_PARM_OFFSET(FNDECL) pdp10_first_parm_offset ()

/* Offset from the stack pointer register to an item dynamically
   allocated on the stack, e.g., by alloca.  */
#define STACK_DYNAMIC_OFFSET(FUNDECL) pdp10_stack_dynamic_offset ()

/* A C expression whose value is RTL representing the value of the
   return address for the frame COUNT steps up from the current frame.  */
#define RETURN_ADDR_RTX(COUNT, FRAME) \
  gen_rtx_MEM (Pmode, gen_rtx_PLUS (Pmode, (FRAME), constm1_rtx))

#if 0
#define INCOMING_RETURN_ADDR_RTX \
  gen_rtx_MEM (Pmode, stack_pointer_rtx)
#endif

#define INCOMING_FRAME_SP_OFFSET -UNITS_PER_WORD

/*
 * Registers That Address the Stack Frame
 */

/* The register number of the stack pointer register, which must also
   be a fixed register according to FIXED_REGISTERS.  */
#define STACK_POINTER_REGNUM 017

/* The register number of the frame pointer register, which is used to
   access automatic variables in the stack frame.  */
#define FRAME_POINTER_REGNUM 020

#define HARD_FRAME_POINTER_REGNUM 016

/* The register number of the arg pointer register, which is used to
   access the function's argument list.  */
#define ARG_POINTER_REGNUM 021

/* Reserved for position-independent code.  */
#define PIC_SOMETHING_REGNUM 015

/* Register number used for passing a function's static chain
   pointer.  */
#define STATIC_CHAIN_REGNUM 014

/*
 * Eliminating Frame Pointer and Arg Pointer
 */

/* A C expression which is nonzero if a function must have and use a
   frame pointer.  */
#define FRAME_POINTER_REQUIRED 0

#define ELIMINABLE_REGS						\
{ { ARG_POINTER_REGNUM,		STACK_POINTER_REGNUM },		\
  { FRAME_POINTER_REGNUM,	STACK_POINTER_REGNUM },		\
  { HARD_FRAME_POINTER_REGNUM,	STACK_POINTER_REGNUM },		\
  { ARG_POINTER_REGNUM,		HARD_FRAME_POINTER_REGNUM },	\
  { FRAME_POINTER_REGNUM,	HARD_FRAME_POINTER_REGNUM } }	\

#define CAN_ELIMINATE(FROM, TO) 1

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET) \
  (OFFSET) = pdp10_initial_elimination_offset ((FROM), (TO))

/*
 * Passing Function Arguments on the Stack
 */

/* Define this macro if an argument declared in a prototype as an
   integral type smaller than int should actually be passed as an
   int.  */
#define PROMOTE_PROTOTYPES 1

/* The maximum amount of space required for outgoing argsuments will
   be computed and placed into the variable
   current_function_outgoing_args_size.  No space will be pushed onto
   the stack for each call; instead, the function prologue should
   increase the stack frame size by this amount.  */
#define ACCUMULATE_OUTGOING_ARGS 1

/* Define this macro if functions should assume that stack space has
   been allocated for arguments even when their values are passed in
   registers.

   This space can be allocated by the caller, or be a part of the
   machine-dependent stack frame: OUTGOING_REG_PARM_STACK_SPACE says
   which.
#define REG_PARM_STACK_SPACE(FNDECL)
*/

/* A C expression that should indicate the number of bytes of its own
   arguments that a function pops on returning, or 0 if the function
   pops no arguments and the caller must therefore pop them all after
   the function returns.  */
#define RETURN_POPS_ARGS(FUNDECL, FUNTYPE, SIZE) 0

/*
 * Passing Arguments in Registers
 */

/* The default number of registers used to pass arguments.  */
#define PDP10_DEFAULT_REGPARM 7

/* A C expression that controls whether a function argument is passed
   in a register, and which register.  */
#define FUNCTION_ARG(CUM, MODE, TYPE, NAMED) \
  (((NAMED) && ((CUM) < pdp10_regparm)) ? gen_rtx_REG ((MODE), 1 + (CUM)) : 0)

/* A C expression for the number of words, at the beginning of an
   argument, must be put in registers.  The value must be zero for
   arguments that are passed entirely in registers or that are
   entirely pushed on the stack.  */
#define FUNCTION_ARG_PARTIAL_NREGS(CUM, MODE, TYPE, NAMED)		\
  ((NAMED)								\
   && (CUM) < pdp10_regparm						\
   && pdp10_regparm < (CUM) + (((MODE) == BLKmode			\
				? int_size_in_bytes (TYPE)		\
				: GET_MODE_SIZE (MODE)) + 3) / 4	\
   ? pdp10_regparm - (CUM) : 0)

/* A C type for declaring a variable that is used as the first
   argument of FUNCTION_ARG and other related values.

   For the PDP-10, the type is int and holds the number of registers
   used for arguments so far.  */
#define CUMULATIVE_ARGS int

/* A C statement for initializing the variable CUM for the state at
   the beginning of the argument list.  */
#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, INDIRECT) \
  (CUM) = (FNTYPE && RETURN_IN_MEMORY (TREE_TYPE (FNTYPE))) ? 1 : 0

/* A C statement to update the summarizer variable CUM to advance past
   an argument in the argument list.  */
#define FUNCTION_ARG_ADVANCE(CUM, MODE, TYPE, NAMED)	\
  (CUM) += ((((MODE) == BLKmode				\
	      ? int_size_in_bytes (TYPE)		\
	      : GET_MODE_SIZE (MODE)) + 3) / 4)

/* FUNCTION_ARG_PADDING(MODE, TYPE)

   A C expression which determines wheter, and in which direction, to
   pad out an argument with extra space.  */

/* A C expression that is nonzero if REGNO is the number of a hard
   register in which function arguments are sometimes passed.  This
   does not include implicit arguments such as the static chain and
   the structure-value adress.  */
#define FUNCTION_ARG_REGNO_P(REGNO) \
  ((REGNO) >= 1 && (REGNO) <= pdp10_regparm)

/*
 * How Scalar Function Values Are Returned
 */

/* A C expression to create an RTX representing the place where a
   function returns a value of data type VALTYPE.  */
#define FUNCTION_VALUE(VALTYPE, FUNC) gen_rtx_REG (TYPE_MODE (VALTYPE), 1)

/* A C expression to create an RTX representing the place where a
   library function returns a value of mode MODE.  */
#define LIBCALL_VALUE(MODE) gen_rtx_REG ((MODE), 1)

/* A C expression that is nonzero if REGNO is the number of a hard
   register in which the values of a function may come back.  A
   register whose use for returning values is limited to serving as
   the second of a pair need not be recognized by this macro.  */
#define FUNCTION_VALUE_REGNO_P(REGNO) ((REGNO) == 1)

/*
 * How Large Values Are Returned
 */

/* A C expression which can inhibit the returning of certain function
   values in registers, based on the type of the value.  */
#define RETURN_IN_MEMORY(TYPE)						     \
  ((TYPE_SIZE (TYPE) && !TREE_CONSTANT (TYPE_SIZE (TYPE)))		     \
   || (TYPE_MODE (TYPE) == BLKmode					     \
       ? int_size_in_bytes (TYPE)					     \
       : GET_MODE_SIZE (TYPE_MODE (TYPE))) > pdp10_retregs * UNITS_PER_WORD)

/* Define this macro to be 1 if all structure and union return values
   must be in memory.  Since this results in slower code, this should
   be defined only if needed for compatibility with other compilers or
   with an ABI.  If you define this macro to be 0, then the
   conventions used for structure and union return values are decided
   by the RETURN_IN_MEMORY macro.  */
#define DEFAULT_PCC_STRUCT_RETURN 0

/* If the structure value address is passed in a register, then
   STRUCT_VALUE_REGNUM should be the number of that register.  */
#define STRUCT_VALUE_REGNUM 1

/*
 * Caller-Saves Register Allocation
 */

/* A C expression to determine whether it is worthwhile to consider
   placing a pseudo-register in a call-clobbered hard register and
   saving and restoring it around each function call.  The expression
   should be 1 when this is worth doing, and 0 otherwise.
#define CALLER_SAVE_PROFITABLE(REFS, CALLS) (4 * (CALLS) < (REFS))
*/

/*
 * Function Entry and Exit
 */

/* A C compound statement that outputs the assembler code for a thunk
   function, used to implement C++ virtual function calls with
   multiple inheritance.

   If you do not define this macro, the target-independent code in the
   C++ frontend will generate a less efficient heavyweight thunk that
   calls FUNCTION instead of jumping to it.  The generic approach does
   not support varargs.
#define ASM_OUTPUT_MI_THUNK(FILE, THUNK_FNDECL, DELTA, FUNCTION)
*/

/*
 * Generating Code for Profiling
 */

/* A C statement or compound statement to output to FILE some
   assembler code to call the profiling subroutine mcount.  Before
   calling, the assembler code must load the address of a counter
   variable into a register where mcount expects to find the
   address.
*/
#define FUNCTION_PROFILER(FILE,LABELNO) \
  pdp10_function_profiler ((FILE), (LABELNO))

/* MACHINE_STATE_SAVE
   MACHINE_STATE_RESTORE.  */


/**********************************************************************

	Tail Calls

**********************************************************************/

#define FUNCTION_OK_FOR_SIBCALL(DECL) \
  !(current_function_stdarg || current_function_varargs)


/**********************************************************************

	Implementing the Varargs Macros

**********************************************************************/

#define SETUP_INCOMING_VARARGS(CUM, MODE, TYPE, PRETEND_SIZE, NO_RTL) \
  (PRETEND_SIZE) = pdp10_setup_incoming_varargs ((CUM), (PRETEND_SIZE))

#define STRICT_ARGUMENT_NAMING 1


/**********************************************************************

	Trampolines for Nested Functions

**********************************************************************/

/* A "trampoline" is a small piece of code that is created at run time
when the address of a nested function is taken.  It normally resides
on the stack, in the stack frame of the containing function.  These
macros tell GCC how to generate code to allocate and initialize a
trampoline.

   The instructions in the trampoline must do two things: load a
constant address into the static chain register, and jump to the real
address of the nested function.  On CISC machines such as the m68k,
this requires two instructions, a move immediate and a jump.  Then the
two addresses exist in the trampoline as word-long immediate operands.
On RISC machines, it is often necessary to load each address into a
register in two parts.  Then pieces of each address form separate
immediate operands.

   The code generated to initialize the trampoline must store the
variable parts -- the static chain value and the function address --
into the immediate operands of the instructions.  On a CISC machine,
this is simply a matter of copying each address to a memory reference
at the proper offset from the start of the trampoline.  On a RISC
machine, it may be necessary to take out pieces of the address and
store them separately.  */

/* A C statement to output, on the stream FILE, assembler code for a
   block of data that contains the constant part of a trampoline.

   On the PDP-10 the trampoline looks like this (if 14 is the static
   chain regnum):
	MOVE 14,.+2
	JRST @.+2
	static_chain_value
	funtion_address

   Or, if all code is in one section:
	MOVE 14,.+2
	JRST function_address
	static_chain_value

   Or, if everything is in one section:
	MOVE 14,static_chain_value
	JRST function_address */
#define TRAMPOLINE_TEMPLATE(FILE)					  \
do {									  \
  rtx op;								  \
  op = gen_rtx_REG (SImode, STATIC_CHAIN_REGNUM);			  \
  if (TARGET_SMALL)							  \
    output_asm_insn ("movei %0,0", &op);				  \
  else									  \
    output_asm_insn ("move %0,.+2", &op);				  \
  if (TARGET_SMALL || TARGET_SMALLISH)					  \
    output_asm_insn ("jrst 0", &op);					  \
  else									  \
    {									  \
      output_asm_insn ("jrst @.+2", &op);				  \
      assemble_integer (const0_rtx, UNITS_PER_WORD, BITS_PER_WORD, true); \
    }									  \
  if (! TARGET_SMALL)							  \
    assemble_integer (const0_rtx, UNITS_PER_WORD, BITS_PER_WORD, true);	  \
} while (0)

/* A C expression for the size in bytes of the trampoline, as an
   integer.  */
#define TRAMPOLINE_SIZE (TARGET_SMALL ? 8 : TARGET_SMALLISH ? 12 : 16)

/* A C statement to initialize the variable parts of a trampoline.  */
#define INITIALIZE_TRAMPOLINE(ADDR, FNADDR, CXT)			 \
do {									 \
  if (TARGET_SMALL || TARGET_SMALLISH)					 \
    emit_insn (gen_HRR (gen_rtx_MEM (SImode, plus_constant ((ADDR), 1)), \
			(FNADDR)));					 \
  else									 \
    emit_move_insn (gen_rtx_MEM (SImode, plus_constant ((ADDR), 3)),	 \
		    (FNADDR));						 \
  if (TARGET_SMALL)							 \
    emit_insn (gen_HRR (gen_rtx_MEM (SImode, (ADDR)), (CXT)));		 \
  else									 \
    emit_move_insn (gen_rtx_MEM (SImode, plus_constant ((ADDR), 2)),	 \
		    (CXT));						 \
} while (0)


/**********************************************************************

	Implicit Calls To Library Routines

**********************************************************************/

/* Define this macro if GCC should generate calls to the System V (and
   ANSI C) library functions memcpy and memset rather than the BSD
   functions bcopy and bzero.  */
#define TARGET_MEM_FUNCTIONS 1


/**********************************************************************

	Addressing Modes

**********************************************************************/

/* A C expression that is nonzero if the machine supports pre-increment
   addressing.  */
#define HAVE_PRE_INCREMENT 1

/* A C expression that is nonzero if the machine supports
   post-increment addressing.  */
#define HAVE_POST_INCREMENT TARGET_XKL2

/* A C expression that is 1 if the RTX is a constant which is a valid
   address.  */
#define CONSTANT_ADDRESS_P(X)			\
  (GET_CODE (X) == CONST			\
   || GET_CODE (X) == CONST_INT			\
   || GET_CODE (X) == LABEL_REF			\
   || GET_CODE (X) == SYMBOL_REF)

/* A number, the maximum number of registers that can appear in a
   valid memory address.  Note that it is up to you to specify a value
   equal to the maximum number that GO_IF_LEGITIMATE_ADDRESS would
   ever accept.  */
#define MAX_REGS_PER_ADDRESS 1

/* A C compound statement with a contitional goto LABEL; executed if X
   (an RTX) is a legitimate memory address on the target machine for a
   memory operand of mode MODE.

   This macro must exist in two variants: a strict variant and a
   non-scrict one.  The strict variant is used in the reload pass.  It
   must be defined so that any pseudo-register that has not been
   allocated a hard register is considered a memory reference.  In
   contexts where some kind of register is required, a pseudo-register
   with no hard register must be rejected.  */
#ifdef REG_OK_STRICT
#define GO_IF_LEGITIMATE_ADDRESS(MODE, X, LABEL)  	\
{							\
  if (pdp10_legitimate_address_p ((MODE), (X), 1))	\
    goto LABEL;						\
}
#else
#define GO_IF_LEGITIMATE_ADDRESS(MODE, X, LABEL)  	\
{							\
  if (pdp10_legitimate_address_p ((MODE), (X), 0))	\
    goto LABEL;						\
}
#endif

/* A C expression that is nonzero if X (assumed to be a reg RTX) is
   valid for use as a base register of a memory reference in mode
   MODE.  For hard registers, it should always accept those which the
   hardware permits and reject the others.  Whether the macro accepts
   or rejects pseudo-registers must be controlled by REG_OK_STRICT
   as described above.

   On the PDP-10, byte pointers are usable in any register, but word
   pointers can only be used in an index register.  */
#ifdef REG_OK_STRICT
#define REG_MODE_OK_FOR_BASE_P(X, MODE)		\
  (GET_MODE_SIZE (MODE) < UNITS_PER_WORD ||	\
   REGNO_REG_CLASS (REGNO (X)) == INDEX_REGS)
#else
#define REG_MODE_OK_FOR_BASE_P(X, MODE)		\
  (REGNO (X) >= FIRST_PSEUDO_REGISTER ||	\
   GET_MODE_SIZE (MODE) < UNITS_PER_WORD ||	\
   REGNO_REG_CLASS (REGNO (X)) == INDEX_REGS)
#endif

/* A C expression that is nonzero if X (assumed to be a reg RTX) is
   valid for use as an index register.  */
#define REG_OK_FOR_INDEX_P(X) 0

/* A C compound statement that attempts to replace X with a valid
   memory address for an operand of mode MODE.  WIN will be a C
   statement label elsewhere in the code.  */
#define LEGITIMIZE_ADDRESS(X, OLDX, MODE, WIN)			\
do {								\
  rtx _tmpx = pdp10_legitimize_address ((X), (OLDX), (MODE));	\
  if (_tmpx)							\
    {								\
      (X) = _tmpx;						\
      goto WIN;							\
    }								\
} while (0)

/* A C compound statement that attempts to replace X, which is an
   address that needs reloading, with a valid memory address for an
   operand of mode MODE.  WIN will be a C statement label elsewhere in
   the code.  It is not necessary to define this macro, but it might
   be useful for performance reasons
#define LEGITIMIZE_RELOAD_ADDRESS(X, MODE, OPNUM, TYPE, IND_LEVELS, WIN)
*/

/* A C statement or compound statement with a conditional goto LABEL;
   executed if memory address X (an RTX) can have different meanings
   depending on the machine mode of the memory reference it is used
   for or if the address is valid for some modes but not others.  */
#define GO_IF_MODE_DEPENDENT_ADDRESS(ADDR,LABEL)	\
{							\
  if (GET_CODE (ADDR) == PRE_INC)			\
    goto LABEL;						\
}

/* A C expression that is nonzero if X is a legitimate constant for an
   immediate operand on the target machine.  You can assume that X
   satisfies CONSTANT_P, so you need not check this.  */
#define LEGITIMATE_CONSTANT_P(X) 1


/**********************************************************************

	Describing Relative Costs of Operations

**********************************************************************/

/* A part of a C switch statement that describes the relative consts
   of constant RTL expressions.  Each case must ultimately reach a
   return statement to return the relative cost of the use of that
   kind of constant value in an expression.  */
#define CONST_COSTS(RTX, CODE, OUTER_CODE)				\
  case CONST_INT:							\
    return pdp10_const_int_costs (INTVAL (RTX), OUTER_CODE);		\
  case CONST:								\
  case LABEL_REF:							\
  case SYMBOL_REF:							\
    return TARGET_EXTENDED ? COSTS_N_INSNS (1) / 2: 0;			\
  case CONST_DOUBLE:							\
    return pdp10_const_double_costs ((RTX), (CODE), (OUTER_CODE));

/* Like CONST_COSTS but applies to nonconstant RTL expressions.  This
   can be used, for example, to indicate how costly a multiply
   instruction is.  In writing this macro, you can use the construct
   COSTS_N_INSNS (N) to specify a cost equal to N fast instructions.
   OUTER_CODE is the code of the expression in which X is contained.  */
#define RTX_COSTS(X, CODE, OUTER_CODE)					     \
  case PLUS:								     \
  case MINUS:								     \
  case AND:								     \
  case IOR:								     \
  case XOR:								     \
    return								     \
      (GET_MODE (X) == SImode						     \
       ? COSTS_N_INSNS (1)						     \
       : HAVE_adddi3 ? COSTS_N_INSNS (2) : COSTS_N_INSNS (3))		     \
      + pdp10_operand_cost (XEXP ((X), 0), (OUTER_CODE))		     \
      + pdp10_operand_cost (XEXP ((X), 1), (OUTER_CODE));		     \
    /* Register-register ADDs and ADDIs are fast.  */			     \
/*									     \
    if (GET_MODE (X) == SImode						     \
	&& GET_CODE (XEXP (X, 0)) == REG				     \
	&& (GET_CODE (XEXP (X, 1)) == REG				     \
	    || (GET_CODE (XEXP (X, 1)) == CONST_INT			     \
		&& CONST_OK_FOR_LETTER_P (INTVAL (XEXP (X, 1)), 'I'))))	     \
      return COSTS_N_INSNS (1);						     \
*/									     \
    /* Register-memory ADDs and register-register DADDs are not as fast.  */ \
/*									     \
    else if (GET_MODE (X) == SImode					     \
	     || (GET_MODE (X) == DImode					     \
		 && GET_CODE (XEXP ((X), 0)) == REG			     \
		 && GET_CODE (XEXP ((X), 1)) == REG))			     \
      return COSTS_N_INSNS (2);						     \
*/									     \
    /* Register-memory DADDs are slow.  */				     \
/*									     \
    else								     \
      return COSTS_N_INSNS (4);						     \
*/									     \
  case ASHIFT:								     \
  case ASHIFTRT:							     \
  case LSHIFTRT:							     \
  case ROTATE:								     \
  case ROTATERT:							     \
    return COSTS_N_INSNS (1);						     \
  case MULT:								     \
    if (optimize_size)							     \
      /* Shift instructions are preferred, so multiplications are	     \
	 slightly more costly than one instruction.  */			     \
      return COSTS_N_INSNS (3) / 2;					     \
    else if (GET_MODE (X) == SImode)					     \
      return COSTS_N_INSNS (5);						     \
    else if (GET_CODE (X) == DImode)					     \
      return COSTS_N_INSNS (10);					     \
    else								     \
      return COSTS_N_INSNS (20);					     \
  case UDIV:								     \
  case UMOD:								     \
    if (!TARGET_XKL2)							     \
      return COSTS_N_INSNS (50);					     \
    /* Fall through.  */						     \
  case DIV:								     \
  case MOD:								     \
    if (optimize_size)							     \
      /* Shift instructions are preferred, so divisions are slightly	     \
	 more costly than one instruction.  */				     \
      return COSTS_N_INSNS (3) / 2;					     \
    else								     \
      return COSTS_N_INSNS (20);					     \
  case NE:								     \
    return COSTS_N_INSNS (2);						     \
  case EQ: case GE: case GT: case LE: case LT:				     \
  case GEU: case GTU: case LEU: case LTU:				     \
    return COSTS_N_INSNS (optimize_size ? 3 : 2);			     \
  case UNSPEC:								     \
    return pdp10_operand_cost (X, OUTER_CODE);

/* An expression giving the cost of an addressing mode that contains
   ADDRESS.  */
#define ADDRESS_COST(ADDRESS) pdp10_address_cost (ADDRESS)

/* A C expression for the cost of moving data of mode MODE from a
   register in class FROM to a register in class TO.

   On the PDP-10, a register-to-register move is always one, fast
   instruction.  */
#define REGISTER_MOVE_COST(MODE, FROM, TO) 1

/* A C expression for the cost of moving data of mode MODE from a
   register of class CLASS and memory; IN is zero if the value is to
   be written to memory, nonzero if it is to be read in.  This cost is
   relative to REGISTER_MOVE_COST.

   Moves to and from memory are quite expensive.  */
#define MEMORY_MOVE_COST(MODE, CLASS, IN) \
  (1 + GET_MODE_SIZE (MODE) / UNITS_PER_WORD)

/* A C expression for the cost of a branch instruction.  A value of 1
   is the default; other values are interpreted relative to that.  */
#define BRANCH_COST 1

/* Define this macro as a C expression which is nonzero if accessing
   less than a word of memory is no faster than accessing a word of
   memory.  */
#define SLOW_BYTE_ACCESS 1

/* A C expression used to determine whether a load preincrement is a
   good thing to use for a given mode.  */
#define USE_LOAD_POST_INCREMENT(MODE) GET_MODE_SIZE (MODE) < 4

/* A C expression used to determine whether a store preincrement is a
   good thing to use for a given mode.  */
#define USE_STORE_POST_INCREMENT(MODE) GET_MODE_SIZE (MODE) < 4

/* A C expression used to determine whether a load preincrement is a
   good thing to use for a given mode.  */
#define USE_LOAD_PRE_INCREMENT(MODE) GET_MODE_SIZE (MODE) < 4

/* A C expression used to determine whether a store preincrement is a
   good thing to use for a given mode.  */
#define USE_STORE_PRE_INCREMENT(MODE) GET_MODE_SIZE (MODE) < 4


/**********************************************************************

	Dividing the Output into Sections

**********************************************************************/

/* A C expression whose value is a string containing the assembler
   operation that should precede instructions and read-only data.  */
#ifndef TEXT_SECTION_ASM_OP
#define TEXT_SECTION_ASM_OP  	"\t.text"
#endif

/* A C expression whose value is a string containing the assembler
   operation to identify the following data as writable initialized
   data.  */
#ifndef DATA_SECTION_ASM_OP
#define DATA_SECTION_ASM_OP  	"\t.data"
#endif

/* A C expression whose value is a string containing the assembler
   operation to identify the following data as uninitialized data.  */
#ifndef BSS_SECTION_ASM_OP
#define BSS_SECTION_ASM_OP   	"\t.bss"
#endif

/* Define this macro if references to a symbol must be treated
   differently depending on something about the variable or function
   named by the symbol.  */
#define ENCODE_SECTION_INFO(DECL, NEW_DECL_P) \
  pdp10_encode_section_info (DECL)


/**********************************************************************

	Position Independent Code

**********************************************************************/

/* This section describes macros that help implement generation of
position independent code.  Simply defining these macros is not enough
to generate valid PIC; you must also add support to the macros
GO_IF_LEGITIMATE_ADDRESS and PRINT_OPERAND_ADDRESS well as
LEGITIMIZE_ADDRESS.  You must modify the definition of movsi to do
something appropriate when the source operand contains a symbolic
address.  You may also need to alter the handling of switch statements
so that they use relative addresses.  */

/* The register number of the register used to address a table of
   static data addresses in memory.  In some cases this register is
   defined by a processor's ABI.
#define PIC_OFFSET_TABLE_REGNUM 015
*/

/* FINALIZE_PIC */

/* LEGITIMATE_PIC_OPERAND */


/**********************************************************************

	Defining the Output Assembler Language

**********************************************************************/

/*
 * The Overall Framework of an Assembler File
 */

/* A C epression which outputs to the stdio stream STREAM some
   appropriate text to go at the start of an assembler file.  */
#ifndef ASM_FILE_START
#define ASM_FILE_START(STREAM) do { } while (0)
#endif

/* A C expression which outputs to the stdio stream STREAM some
   appropriate text to go at the end of an assembler file.  */
#ifndef ASM_FILE_END
#define ASM_FILE_END(STREAM) do { } while (0)
#endif

/* A C string constant describing how to begin a comment in the target
   assembler language.  The compiler assumes that the comment will end
   at the end of the line.
#ifndef ASM_COMMENT_START
*/

/* A C string constant for test to be output before each asm statement
   or group of consecutive ones.

   PDP-10 assemblers should define this to the empty string.
#define ASM_APP_ON
*/

/* A C string constant for test to be output after each asm statement
   or group of consecutive ones.

   PDP-10 assemblers should define this to the empty string.
#define ASM_APP_OFF
*/

/*
 * Output of Data
 */

/* C statements for outputting to the stdio stream STREAM an assembler
   instruction to assemble a floating-point constant.  */

#ifndef ASM_OUTPUT_DOUBLE
#define ASM_OUTPUT_DOUBLE(STREAM, VALUE)  		\
do { char str[30];					\
     REAL_VALUE_TO_DECIMAL ((VALUE), "%.19g", str);	\
     fprintf ((STREAM), "\t.double %s\n", str);		\
   } while (0)
#endif

#ifndef ASM_OUTPUT_FLOAT
#define ASM_OUTPUT_FLOAT(STREAM, VALUE)			\
do { char str[30];					\
     REAL_VALUE_TO_DECIMAL ((VALUE), "%.8g", str);	\
     fprintf ((STREAM), "\t.float %s\n", str);		\
   } while (0);
#endif

/* C statements for outputting to the stdio stream STREAM an assembler
   instruction to assemble an integer constant.  */

/*  #ifndef ASM_OUTPUT_DOUBLE_INT */
/*  #define ASM_OUTPUT_DOUBLE_INT(STREAM, EXP)	\ */
/*    fputs ("\t.dword\t", (STREAM));		\ */
/*    output_addr_const ((STREAM), (EXP));		\ */
/*    fputc ('\n', (STREAM)); */
/*  #endif */

/*  #ifndef ASM_OUTPUT_INT	   */
/*  #define ASM_OUTPUT_INT(STREAM, EXP)		\ */
/*    fputs ("\t.word\t", (STREAM));		\ */
/*    output_addr_const ((STREAM), (EXP));		\ */
/*    fputc ('\n', (STREAM)); */
/*  #endif */

/*  #ifndef ASM_OUTPUT_SHORT */
/*  #define ASM_OUTPUT_SHORT(STREAM, EXP)	\ */
/*    fputs ("\t.hword\t", (STREAM));	\ */
/*    output_addr_const ((STREAM), (EXP)),	\ */
/*    fputc ('\n', (STREAM)); */
/*  #endif */

/*  #ifndef ASM_OUTPUT_CHAR */
/*  #define ASM_OUTPUT_CHAR(STREAM, EXP)	\ */
/*    fputs ("\t.qword\t", (STREAM));	\ */
/*    output_addr_const ((STREAM), (EXP)),	\ */
/*    fputc ('\n', (STREAM)); */
/*  #endif */

/* A C statement to output to the stdio stream STREAM an assembler
   instruction to assemble a single byte containing the number
   VALUE.  */
/*  #ifndef ASM_OUTPUT_BYTE */
/*  #define ASM_OUTPUT_BYTE(STREAM, VALUE) \ */
/*    fprintf ((STREAM), "\t.byte\t%d\n", (VALUE)) */
/*  #endif */

/* A C statement to output to the stdio stream STREAM an assembler
   instruction to assemble a string constant.

   PDP-10 assemblers should use ASCIZ or BYTE.  GAS will use .ascii.
#define ASM_OUTPUT_ASCII(STREAM, PTR, LEN)
*/

/* Define this macro as a C expression which is nonzero if the
   character C is used as a logical line separator by the
   assembler.

   GAS uses ';'.  MIDAS uses '?'.
#define IS_ASM_LOGICAL_LINE_SEPARATOR(C)
*/

/* These macros are defined as C string constants, describing the
   syntax in the assembler for grouping arithmetic expressions.  */
#ifndef ASM_OPEN_PAREN
#define ASM_OPEN_PAREN	"<"
#endif
#ifndef ASM_CLOSE_PAREN
#define ASM_CLOSE_PAREN	">"
#endif

/*
 * Output of Uninitialized Variables
 */

/* A C statement to output to the stdio stream STREAM the assembler
   definition of a common-label named NAME whose size is SIZE bytes.
   The variable ROUNDED is the size rounded up to whatever alignment
   the caller wants.  */
#ifndef ASM_OUTPUT_COMMON
#define ASM_OUTPUT_COMMON(STREAM, NAME, SIZE, ROUNDED)	\
  fputs ("\t.comm\t", (STREAM));			\
  assemble_name ((STREAM), (NAME));			\
  fprintf ((STREAM), ", %d\n", (ROUNDED))
#endif
     
/* A C statement to output to the stdio stream STREAM the assembler
   definition of uninitialized global DECL named NAME whose size is
   SIZE bytes.  The variable ROUNDED is the size rounded up to
   whatever alignment the caller wants.  */
#ifndef ASM_OUTPUT_ALIGNED_BSS
#define ASM_OUTPUT_ALIGNED_BSS(STREAM, DECL, NAME, SIZE, ALIGN) \
  asm_output_aligned_bss ((STREAM), (DECL), (NAME), (SIZE), (ALIGN))
#endif
     
/* A C statement to output to the stdio stream STREAM the assembler
   definition of a local-common-label named NAME whose size is SIZE
   bytes.  The variable ROUNDED is the size rounded up to whatever
   alignment the caller wants.  */
#ifndef ASM_OUTPUT_LOCAL
#define ASM_OUTPUT_LOCAL(STREAM, NAME, SIZE, ROUNDED)	\
  do {							\
    bss_section ();					\
    ASM_OUTPUT_LABEL ((STREAM), (NAME));		\
    fprintf ((STREAM), "\t.space\t%d\n", (ROUNDED));	\
  } while (0)
#endif
     
/*
 * Output and Generation of Labels
 */

/* A C statement to output to the stdio stream STREAM the assembler
   definition of a label named NAME.  */
#ifndef ASM_OUTPUT_LABEL
#define ASM_OUTPUT_LABEL(STREAM, NAME)	\
  assemble_name ((STREAM), (NAME));	\
  fputs (":\n", (STREAM))
#endif

/* SYMBOL_REF_FLAG is used to indicate that the symbol is referring to
   a function known to be defined in the current translation unit.  */
#define SYMBOL_REF_INTERNAL SYMBOL_REF_FLAG

/* A C statement to output to the stdio stream STREAM any text
   necessary for declaring the name NAME of a function which is being
   defined.  This macro is responsible for outputting the label
   definition (perhaps using ASM_OUTPUT_LABEL).  The argument DECL is
   the FUNCTION_DECL tree node representing the function.

   If this macro is not defined, then the function name is defined in
   the usual manner as a label (by means of ASM_OUTPUT_LABEL).  */
#ifndef ASM_DECLARE_FUNCTION_NAME
#define ASM_DECLARE_FUNCTION_NAME(STREAM, NAME, DECL)		\
do {								\
  if (!DECL_WEAK (DECL))					\
    SYMBOL_REF_INTERNAL (XEXP (DECL_RTL (DECL), 0)) = 1;	\
  ASM_OUTPUT_LABEL ((STREAM), (NAME));				\
} while (0)
#endif

/* A C statement to output to the stdio stream STREAM any text
   necessary for declaring the name NAME of an initialized variable
   which is being defined.  This macro must output the label
   definition (perhaps using ASM_OUTPUT_LABEL).  */
#define ASM_DECLARE_OBJECT_NAME(STREAM, NAME, DECL) \
  pdp10_asm_declare_object_name ((STREAM), (NAME), (DECL))

/* A C statement to output to the stdio stream STREAM some commands
   that will make the label NAME global; that is, available for
   reference from other files.  */
#ifndef ASM_GLOBALIZE_LABEL
#define ASM_GLOBALIZE_LABEL(STREAM,NAME)	\
  fputs ("\t.globl\t", (STREAM)),		\
  assemble_name ((STREAM), (NAME));		\
  fputc ('\n', (STREAM))
#endif

/* A C statement to output to the stdio stream STREAM any text
   necessary for declaring the name of an external symbol named NAME
   which is referenced in this compilation but not defined.

   This is not necessary in GAS, but is used for other PDP-10
   assemblers.
#define ASM_OUTPUT_EXTERNAL(STREAM, DECL, NAME)
*/

/* A C statement to output on STREAM an assembler pseudo-op to declare
   a library function name external.

   This is not necessary in GAS, but is used for other PDP-10
   assemblers.
#define ASM_OUTPUT_EXTERNAL_LIBCALL(STREAM, SYMREF)
*/

/* A C statement to output to the stdio stream STREAM a reference in
   assembler syntax to a label named NAME.  */
#define ASM_OUTPUT_LABELREF(STREAM, NAME) \
  pdp10_asm_output_labelref ((STREAM), (NAME))

/* A C statement to output a reference to SYMBOL_REF SYM.  If not
   defined, assemble_output will be used to output the name of the
   symbol.  This macro may be used to modify the way a symbol is
   referenced depending on information encoded by
   ENCODE_SECTION_INFO.  */
#define ASM_OUTPUT_SYMBOL_REF(STREAM, SYM) \
  pdp10_asm_output_symbol_ref ((STREAM), (SYM))

/* A C statement to output to the stdio stream STREAM a label whose
   name is made from the string PREFIX and the numer NUM.  */
#ifndef ASM_OUTPUT_INTERNAL_LABEL
#define ASM_OUTPUT_INTERNAL_LABEL(STREAM, PREFIX, NUM) \
  fprintf ((STREAM), "%%%s%u:\n", (PREFIX), (unsigned)(NUM))
#endif

/* A C statement to store into the string STRING a label whose name is
   made from the string PREFIX and the numer NUM.  */
#ifndef ASM_GENERATE_INTERNAL_LABEL
#define ASM_GENERATE_INTERNAL_LABEL(STRING, PREFIX, NUM) \
  sprintf ((STRING), "*%%%s%u", (PREFIX), (unsigned)(NUM))
#endif

/* Construct a private name.  */
#define ASM_FORMAT_PRIVATE_NAME(OUTVAR,NAME,NUMBER)	\
  (OUTVAR) = (char *)alloca (strlen (NAME) + 10);	\
  sprintf ((OUTVAR), "%s%%%d", (NAME), (NUMBER))

/* A C statement to output to the stdio stream STREAM assembler code
   which defines the symbol NAME to have the value VALUE.

   PDP-10 assemblers define this to use "=" or "==".
#define ASM_OUTPUT_DEF(FILE, LABEL, VALUE)
*/

/* This works for GAS.  */
#define SET_ASM_OP ".set"

/*
 * Output of Assembler Instructions
 */

/* A C initializer containing the assembler's names for the machine
   registers, eash one as a C string constant.  */
#ifndef REGISTER_NAMES
#define REGISTER_NAMES					\
{							\
   "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",	\
  "10", "11", "12", "13", "14", "15", "16", "17",	\
  "<fp>", "<ap>"					\
}
#endif

#define pdp10_print_tab(STREAM) \
  fputc (target_flags & ASM_TAB ? '\t' : ' ', (STREAM));

/* Define this macro if you are using an unusual assembler that
   requires different names for the machine instructions.

   The definition is a C statement or statements which output an
   assembler instruction opcode to the stdio stream STREAM.  PTR is a
   variable of type char * which points to the opcode name.  The
   definition should output the opcode name to STREAM, performing any
   translation you require, and increment the variable PTR to point at
   the end of the opcode.

   On the PDP-10, this is used to optionally change lower-case
   characters to upper-case, and a space into a tab character.  */
#define ASM_OUTPUT_OPCODE(STREAM, PTR)					\
  if (target_flags & (ASM_UPPER | ASM_TAB | ASM_INDENT_SKIPPED))	\
    {									\
      (PTR) = pdp10_print_insn ((STREAM), (PTR));			\
      if (*(PTR) == ' ')						\
	{								\
	  pdp10_print_tab (STREAM);					\
	  (PTR)++;							\
	}								\
    }

#if 0
  (PTR) = (target_flags & (ASM_UPPER | ASM_TAB | ASM_INDENT_SKIPPED)) ?	\
	  pdp10_asm_output_opcode ((STREAM), (PTR)) : (PTR)

/* Called by the ASM_OUTPUT_OPCODE macro.  */
const char *
pdp10_asm_output_opcode (FILE *stream, const char *ptr)
{
  ptr = pdp10_print_insn (stream, ptr);
  if (*ptr == ' ')
    {
      pdp10_print_tab (stream);
      ptr++;
    }
  return ptr;
}
#endif

/* A C compound statement to output to the stdio stream STREAM the
   assembler syntax for an instruction operand X.  */
#define PRINT_OPERAND(STREAM, X, CODE) \
  pdp10_print_operand ((STREAM), (X), (CODE))

/* A C expression which evaluates to true if CODE is a valid
   punctuation character for use in the PRINT_OPERAND macro.

   %_ is used to output a space if -mindent-skipped is enabled.
   %@ is used to output "0," if -mac0 is enabled.
   %; is used to start a comment.  */
#define PRINT_OPERAND_PUNCT_VALID_P(CODE) \
  ((CODE) == '_' || (CODE) == '@' || (CODE) == ';')

/* A C compound statement to output to the stdio stream STREAM the
   assembler syntax for an instruction operand that is a memory
   reference whose address is X.  */
#define PRINT_OPERAND_ADDRESS(STREAM,X) \
  pdp10_print_operand_address ((STREAM), (X), word_mode, 0)

/* C string expressions to be used for the %R, %L, %U, and %I options
   of asm_printf.  */
#ifndef REGISTER_PREFIX
#define REGISTER_PREFIX ""
#endif
#ifndef LOCAL_LABEL_PREFIX
#define LOCAL_LABEL_PREFIX "."
#endif
/* USER_LABEL_PREFIX */
/* IMMEDIATE_PREFIX */

/* A C expression to output to STREAM some assembler code which will
   push hard register number REGNO onto the stack.  */
#define ASM_OUTPUT_REG_PUSH(STREAM, REGNO) \
  fprintf ((STREAM), "\tPUSH %o,%o\n", STACK_POINTER_REGNUM, (REGNO))

/* A C expression to output to STREAM some assembler code which will
   pop hard register number REGNO off of the stack.  */
#define ASM_OUTPUT_REG_POP(STREAM, REGNO) \
  fprintf ((STREAM), "\tPOP %o,%o\n", STACK_POINTER_REGNUM, (REGNO))

/*
 * Output of Dispatch Tables
 */

#ifndef ASM_OUTPUT_ADDR_VEC_ELT
#define ASM_OUTPUT_ADDR_VEC_ELT(STREAM, VALUE) \
 fprintf ((STREAM), "\t.word\t%s%d\n", LOCAL_LABEL_PREFIX, (VALUE))
#endif

/*
 * Assembler Commands for Alignment
 */

/* A C statement to output to the stdio stream STREAM an assembler
   instruction to advance the location counter by NBYTES bytes.  Those
   bytes should be zero when loaded.  */
#ifndef ASM_OUTPUT_SKIP
#define ASM_OUTPUT_SKIP(STREAM, NBYTES) \
   fprintf ((STREAM), "\t.space\t%d\n", (NBYTES))
#endif

/* A C statement to output to the stdio stream STREAM an assembler
   command to advance the locatino counter to a multiple of 2 to the
   POWER bytes.  */
#ifndef ASM_OUTPUT_ALIGN  
#define ASM_OUTPUT_ALIGN(STREAM, POWER) \
  fprintf ((STREAM), "\t.align\t%d\n", (POWER))
#endif


/**********************************************************************

	Controlling Debugging Information Format

**********************************************************************/

/*
 * Macros Affecting All Debugging Formats
 */

#define TOPS20_DEBUGGING_INFO 1

/* DBX register number for a given compiler register number.
#define DBX_REGISTER_NUMBER(REGNO) (REGNO)
*/

/*
 * Specific Options for DBX Output
 */

/* Define this macro if GCC should produce debugging output for DBX in
   response to the -g option.
#define DBX_DEBUGGING_INFO 1
*/

/* Define this macro to control wether GCC should by default generate
   GDB's extended version of DBX debugging information.
#define DEFAULT_GDB_EXTENSIONS 1
*/

/*
 * Macros for SDB and DWARF Output
 */

/* Define this macro if GCC should produce dwarf version 2 debuggging
   output in response to the -g option.
#define DWARF2_DEBUGGING_INFO 1
*/

/* Define this macro to a nonzero value if GCC should always output
   Dwarf 2 frame information.
#define DWARF2_FRAME_INFO 1
*/


/**********************************************************************

	Miscellaneous Parameters

**********************************************************************/

/* Define this if you have defined special-purpose predicates in the
   file pdp10.c.  This macro is called within an initializer of an
   array of structures.  The first field in the structure is the name
   of a predicate and the second field is an array of rtl codes.  For
   each predicate, list all rtl codes that can be in expressions
   matched by the predicate.  The list should have a trailing
   comma.  */
#define PREDICATE_CODES							\
  { "pdp10_comparison_operator",	{ EQ, NE, LE, LT, GE, GT } },	\
  { "pdp10_equality_operator",		{ EQ, NE } },			\
  { "pdp10_rotate_operator",		{ ROTATE, ROTATERT } },		\
  { "reg_or_mem_operand",		{ SUBREG, REG, MEM } },		\
  { "preferably_register_operand",	{ SUBREG, REG, MEM } },		\
  { "reg_or_mem_word_pointer",		{ SUBREG, REG, MEM } },		\
  { "pdp10_maybe_volatile_memory_operand", { MEM } },			\
  { "pdp10_constaddr_memory_operand",	{ MEM } },			\
  { "pdp10_push_operand",		{ MEM } },			\
  { "pdp10_pop_operand",		{ MEM } },			\
  { "pdp10_const_double_0_operand",	{ CONST_INT, CONST_DOUBLE } },

/* An alias for a machine mode name.  This is the machine mode that
   elements of a jump-table should have.  */
#define CASE_VECTOR_MODE SImode

/* Define this macro if operations between registers with integral
   mode smaller than a word are alway performed on the entire
   register.  */
#define WORD_REGISTER_OPERATIONS

/* Define this macro to be a C expression indicating when insns that
   read memory in MODE, an integral mode narrower than a word, set the
   bits outside of MODE to be either the sign-extension or the
   zero-extension of the data read.

   PDP-10 byte instructions zero-extend the registers.  */
#define LOAD_EXTEND_OP(MODE) ZERO_EXTEND

/* The maximum number of bytes that a single instruction can move
   quickly between memory and registers or between two memory
   locations.  */
#define MOVE_MAX (TARGET_KI10up ? 2 * UNITS_PER_WORD : UNITS_PER_WORD)

/* The maximum number of bytes that a single instruction can move
   quickly between memory and registers or between two memory
   locations.  If this is undefined, the default is MOVE_MAX.
   Otherwise, it is the constant value that is the largest value that
   MOVE_MAX can have at run-time.  */
#define MAX_MOVE_MAX 8

/* A C expression which is nonzero if on this machine it is safe to
   "convert" an integer of INPREC bits to one of OUTPREC bits (where
   OUTPREC is smaller than INPREC) by merely operating on it as if it
   had only OUTPREC bits.  */
#define TRULY_NOOP_TRUNCATION(OUTPREC, INPREC) 1

/* A C expression describing the value returned by a comparison
   operator with an integral mode and stored by a store-flag
   instruction when the condition is true.  */
#define STORE_FLAG_VALUE 1

/* An alias for the machine mode for pointers.  */
#define Pmode SImode

/* An alias for the machine mode used for memory references to
   functions being called.  */
#define FUNCTION_MODE SImode

#define REGISTER_TARGET_PRAGMAS(PFILE) \
  pdp10_register_target_pragmas (PFILE)

/* Define this macro to control use of the character '$' in identifier
   names.  0 means '$' is not allowed by default; 1 means it is
   allowed.  */
#ifndef DOLLARS_IN_IDENTIFIERS
#define DOLLARS_IN_IDENTIFIERS 1
#endif

/* In rare cases, correct code generation requires extra machine
   dependent processing between the second jump optimization pass and
   delayed branch scheduling.  On those machines, define this macro as
   a C statement to act on the code starting at INSN.  */
#define MACHINE_DEPENDENT_REORG(FIRST) \
  pdp10_machine_dependent_reorg (FIRST)

/* Define this to the largest integer machine mode which can be used
   for operations other than load, store and copy operations.  */
#define MAX_INTEGER_COMPUTATION_MODE TImode

/* Define this macro as a C string constant for the linker argument to
   link in the system math library, or "" if the target machine does
   not have a separate math library.  */
#define MATH_LIBRARY ""

#define NAME__MAIN "__main"

enum pdp10_builtins
{
  PDP10_BUILTIN_JFFO,
  PDP10_BUILTIN_FSC,
  PDP10_BUILTIN_DFSC
};

#define BUILD_VA_LIST_TYPE(TYPE) ((TYPE) = pdp10_build_va_list_type ())

#define EXPAND_BUILTIN_VA_START(STDARG_P, VALIST, NEXTARG) \
  pdp10_expand_builtin_va_start ((STDARG_P), (VALIST), (NEXTARG))

#define EXPAND_BUILTIN_VA_ARG(VALIST, TYPE) \
  pdp10_expand_builtin_va_arg ((VALIST), (TYPE))


/**********************************************************************

	External Variable Declarations

**********************************************************************/

#ifdef RTX_CODE
extern rtx pdp10_compare_op0, pdp10_compare_op1;
#endif

extern int pdp10_regparm;
extern int pdp10_retregs;


/**********************************************************************

	Miscellaneous

**********************************************************************/

#define MD_ARRAY_OFFSET(EXP, INDEX, UNIT_SIZE, OFFSET, BIT_OFFSET) \
  pdp10_md_array_offset ((EXP), (INDEX), (UNIT_SIZE), &(OFFSET), &(BIT_OFFSET))

#define MD_EXPAND_BINOP(MODE,OPTAB,OP0,OP1,TARGET,UNSIGNEDP,METHODS,EXP) \
  pdp10_expand_binop (MODE, OPTAB, OP0, OP1, TARGET, UNSIGNEDP, METHODS, EXP)

#define MD_CONVERT_POINTER(EXP, TARGET, MODIFIER, NONNULL) \
  pdp10_convert_pointer ((EXP), NULL_RTX, (TARGET), (MODIFIER), 0, (NONNULL))

#define MD_SIMPLIFY_UNSPEC(X, INSN) pdp10_simplify_unspec ((X), (INSN))

#define MD_EXPAND_ADDR_EXP(EXP, TYPE, TARGET, IGNORE, MODIFIER) \
  pdp10_expand_ADDR_EXP ((EXP), (TYPE), (TARGET), (IGNORE), (MODIFIER))

#define MD_COMPARE_AND_JUMP(EXP, OP0, OP1, CODE, UNSIGNEDP, MODE, SIZE,	    \
			    FALSE_LABEL, TRUE_LABEL)			    \
  pdp10_compare_and_jump ((EXP), (OP0), (OP1), (CODE), (UNSIGNEDP), (MODE), \
			  (SIZE), (FALSE_LABEL), (TRUE_LABEL))

#define gen_rtx_ADJBP(MODE, OP0, OP1) \
  gen_rtx_UNSPEC ((MODE), gen_rtvec (2, (OP0), (OP1)), UNSPEC_ADJBP)

#endif /* __PDP10_H__ */
