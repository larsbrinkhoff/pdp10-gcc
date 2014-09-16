/* Prototypes for exported functions defined in pdp10.c.
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

#ifndef GCC_PDP10_PROTOS_H
#define GCC_PDP10_PROTOS_H

#ifndef HOST_WIDE_INT
#include "hwint.h"
#endif

#ifndef HAVE_MACHINE_MODES
#include "machmode.h"
#endif

#ifdef RTX_CODE
#define Rcode enum rtx_code
#else
#define Rcode int
#endif

#ifdef GCC_OPTABS_H
#define Optab optab
#define Optab_methods enum optab_methods
#else
#define Optab void *
#define Optab_methods int
#endif

int		pdp10_pointer_alignment PARAMS ((rtx x));
int		pdp10_pointer_offset PARAMS ((rtx x));

/* Run-time Target Specification.  */
extern void	pdp10_override_options PARAMS ((void));

/* Storage Layout.  */
extern rtx	pdp10_align_stack_pointer PARAMS ((rtx x, unsigned align));
extern int	pdp10_data_alignment PARAMS ((int align, tree type));
extern rtx	pdp10_data_padding PARAMS ((tree decl, rtx x));
extern int	pdp10_constant_alignment PARAMS ((int align, tree exp));
extern int	pdp10_local_alignment PARAMS ((int align, tree type));
extern int	pdp10_local_size PARAMS ((int size, tree type));

/* Register usage.  */
extern void	pdp10_conditional_register_usage PARAMS ((void));

/* Register Classes.  */
extern int	pdp10_const_ok_for_letter_p PARAMS ((HOST_WIDE_INT, int));
extern int	pdp10_const_double_ok_for_letter_p PARAMS ((rtx x, int c));
extern int	pdp10_extra_constraint PARAMS ((rtx x, int c));

/* Stack Layout and Calling Conventions.  */
extern int	pdp10_starting_frame_offset PARAMS ((void));
extern int	pdp10_stack_pointer_offset PARAMS ((void));
extern int	pdp10_first_parm_offset PARAMS ((void));
extern int	pdp10_stack_dynamic_offset PARAMS ((void));
extern int	pdp10_initial_elimination_offset PARAMS ((int, int));
extern rtx	pdp10_gen_stack_adjust PARAMS ((rtx adjust));
extern void	pdp10_output_func_prologue PARAMS ((FILE *, HOST_WIDE_INT));
extern void	pdp10_output_func_epilogue PARAMS ((FILE *, HOST_WIDE_INT));
extern void	pdp10_function_profiler PARAMS ((FILE *, int));
extern int	pdp10_setup_incoming_varargs PARAMS ((CUMULATIVE_ARGS, int));

/* Addressing Modes.  */
extern int	pdp10_legitimate_address_p PARAMS ((enum machine_mode mode,
						    rtx x, int strict));
extern rtx	pdp10_legitimize_address PARAMS ((rtx x, rtx oldx,
						  enum machine_mode mode));

/* Describing Relative Costs of Operations.  */
extern int	pdp10_const_int_costs PARAMS ((HOST_WIDE_INT i,
					       Rcode outer_code));
extern int	pdp10_const_double_costs PARAMS ((rtx x, Rcode code,
						  Rcode outer_code));
extern int	pdp10_operand_cost PARAMS ((rtx x, Rcode outer_code));
extern int	pdp10_address_cost PARAMS ((rtx address));

/* Dividing the Output into Sections.  */
extern void	pdp10_encode_section_info PARAMS ((tree decl));

/* Defining the Output Assembler Language.  */
extern const char *
		pdp10_print_insn PARAMS ((FILE *stream, const char *insn));
extern int	bytesize_for_ps PARAMS ((int ps));
extern void	pdp10_asm_declare_object_name PARAMS ((FILE *stream,
						       const char *name,
						       tree decl));
extern void	pdp10_asm_output_labelref PARAMS ((FILE *file,
						   const char *name));
extern void	pdp10_asm_output_symbol_ref PARAMS ((FILE *stream, rtx sym));
extern const char *
		pdp10_asm_output_opcode PARAMS ((FILE *stream,
						 const char *ptr));
extern void	pdp10_print_operand PARAMS ((FILE *stream, rtx x, int code));
extern void	pdp10_print_operand_address PARAMS ((FILE *stream, rtx x,
						     enum machine_mode,
						     int flags));

/* Miscellaneous Parameters.  */
extern void	pdp10_register_target_pragmas PARAMS ((void *pfile));
extern void	pdp10_machine_dependent_reorg PARAMS ((rtx first));

/* Built-in Functions.  */
extern void	pdp10_init_builtins PARAMS ((void));
extern rtx	pdp10_expand_builtin PARAMS ((tree exp, rtx target,
					      rtx subtarget,
					      enum machine_mode mode,
					      int ignore));
extern tree	pdp10_build_va_list_type PARAMS ((void));
extern void	pdp10_expand_builtin_va_start PARAMS ((int stdarg_p,
						       tree valist,
						       rtx nextarg));
extern rtx	pdp10_expand_builtin_va_arg PARAMS ((tree valist, tree type));

/* Optimizations.  */
extern const char *
		pdp10_output_range_compare PARAMS ((rtx insn, rtx *operands,
						    int trna));

/* Data Movement.  */
extern int	zero_extended_p PARAMS ((rtx x, int bits, rtx insn));
extern int	sign_extended_p PARAMS ((rtx x, int bits, rtx insn));
extern const char *
		pdp10_output_load_unsigned_byte PARAMS ((rtx insn,
							 rtx *operands));
extern const char *
		pdp10_output_load_signed_byte PARAMS ((rtx insn,
						       rtx *operands));
extern const char *
		pdp10_output_store_byte PARAMS ((rtx insn, rtx *operands));
extern const char *
		pdp10_output_movhi PARAMS ((rtx *operands, int sign_extend));
extern const char *
		pdp10_output_halfword_insv PARAMS ((rtx insn, rtx *operands,
						    int pos));
extern const char *
		pdp10_output_halfword_extv PARAMS ((rtx insn, rtx *operands,
						    int pos));
extern int	pdp10_expand_extv PARAMS ((rtx *operands));
extern int	pdp10_expand_extzv PARAMS ((rtx *operands));
extern const char *
		pdp10_output_extv PARAMS ((rtx insn, rtx *operands));
extern const char *
		pdp10_output_extv_sequence PARAMS ((rtx *operands));
extern const char *
		pdp10_output_extzv PARAMS ((rtx insn, rtx *operands));
extern const char *
		pdp10_output_extzv_sequence PARAMS ((rtx *operands));
extern const char *
		pdp10_output_extzv_foo PARAMS ((rtx insn, rtx *operands));
extern const char *
		pdp10_output_insv PARAMS ((rtx insn, rtx *operands));
extern const char *
		pdp10_output_movsi PARAMS ((rtx insn, int which_alternative));
extern int	pdp10_expand_dmove PARAMS ((rtx destination, rtx source));
extern int	pdp10_expand_move_4 PARAMS ((rtx destination, rtx source));
extern int	pdp10_expand_movstrsi PARAMS ((rtx *operands));
extern int	pdp10_expand_clrstrsi PARAMS ((rtx *operands));
extern int	pdp10_expand_cmpstrsi PARAMS ((rtx *operands));

/* Pointer Arithmetic.  */
extern int	pdp10_bytesize PARAMS ((tree type));
extern rtx	pdp10_expand_addbp3 PARAMS ((tree type, rtx target, rtx op0,
					     rtx op1));

/* Unconditional Jumps.  */
extern int	pdp10_output_call_as_jrst_p PARAMS ((rtx insn));
extern int	pdp10_output_call_as_jsp_p PARAMS ((rtx sym));

/* Conditional Jumps.  */
extern int	pdp10_generate_cbranchdi PARAMS ((Rcode, rtx op0, rtx op1));
extern const char *
		pdp10_output_jrst (rtx label);
extern rtx	pdp10_flip_sign_bit PARAMS ((rtx x));
#define pdp10_remove_unnecessary_label(INSN, LABEL)			\
do {									\
  /* Also see maybe_remove_label_and_following_code.  */		\
  if (get_attr_length (INSN) == 1 && --LABEL_NUSES (LABEL) == 0		\
      && !(LABEL_PRESERVE_P (LABEL) || LABEL_NAME (LABEL)))		\
    INSN_DELETED_P (LABEL) = 1;						\
} while (0)
extern const char *
		pdp10_output_test_and_skip PARAMS ((rtx insn, rtx *operands));

/* Function Prologue and Epilogue.  */
extern void	pdp10_expand_prologue PARAMS ((void));
extern void	pdp10_expand_epilogue PARAMS ((int));
extern const char *
		pdp10_output_return PARAMS ((void));

/* Random stuff.  */
#ifdef RTX_CODE
extern void	pdp10_output_byte PARAMS ((FILE *stream, rtx exp,
					   int byte_size));
extern rtx	pdp10_compare_op0, pdp10_compare_op1;
#endif

extern int	pdp10_byte_count;

extern void	macro_file_start PARAMS ((FILE *));
extern void	macro_file_end PARAMS ((FILE *));
extern void	output_ascii_pseudo_op PARAMS ((FILE *stream, unsigned char *p,
						int len));
extern void	asm_output_ascii PARAMS ((FILE *stream, const char *ptr,
					  int len));
extern void	macro_section_end PARAMS ((FILE *stream));
extern void	macro_output_ascii PARAMS ((FILE *stream, const char *ptr,
					    int len));
extern int	pdp10_align_with_pad PARAMS ((int pad));
extern void	macro_globalize_label PARAMS ((FILE *stream,
					       const char *name));
extern void	macro_output_def PARAMS ((FILE *stream, const char *label,
					  const char *value));
extern void	pdp10_extern_symbol PARAMS ((const char *name, int flags));
extern void	pdp10_rodata_section PARAMS ((void));
extern rtx	pdp10_expand_binop PARAMS ((enum machine_mode mode,
					    Optab optab, rtx op0, rtx op1,
					    rtx target, int unsignedp,
					    Optab_methods methods, tree exp));
extern rtx	pdp10_expand_ADDR_EXP PARAMS ((tree exp, tree type, rtx target,
					       int ignore, int mod));
extern rtx	pdp10_convert_pointer PARAMS ((tree exp, rtx op0, rtx target,
					       int modifier, int sp, int nn));
extern rtx	pdp10_convert_ptr PARAMS ((tree exp, rtx op0, rtx target,
					   int from, int to, int not_null));
extern rtx	pdp10_simplify_unspec PARAMS ((rtx x, rtx insn));
extern void	pdp10_md_array_offset PARAMS ((tree exp, tree index,
					       tree unit_size, tree *offset,
					       tree *bit_offset));
extern void	pdp10_compare_and_jump PARAMS ((tree exp, rtx op0, rtx op1,
						Rcode code, int unsignedp,
						enum machine_mode mode,
						rtx size,
						rtx if_false_label,
						rtx if_true_label));

extern int	pdp10_opcode_indentation;
extern int	pdp10_register_args;
extern int	pdp10_string_literal_byte_size;
extern int	pdp10_bits_per_unit;

extern int	pdp10_giw;
extern tree	char6_signed_type_node;
extern tree	char6_unsigned_type_node;
extern tree	char7_signed_type_node;
extern tree	char7_unsigned_type_node;
extern tree	char8_signed_type_node;
extern tree	char8_unsigned_type_node;
extern tree	char9_signed_type_node;
extern tree	char9_unsigned_type_node;
#define pdp10_char7_unsigned_type_node \
  ((void) pdp10_build_machine_types (), char7_unsigned_type_node)

extern void	pdp10_build_machine_types PARAMS ((void));

#endif /* GCC_PDP10_PROTOS_H */
