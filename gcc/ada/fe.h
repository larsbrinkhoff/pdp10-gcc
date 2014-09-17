/****************************************************************************
 *                                                                          *
 *                         GNAT COMPILER COMPONENTS                         *
 *                                                                          *
 *                                    FE                                    *
 *                                                                          *
 *                              C Header File                               *
 *                                                                          *
 *                                                                          *
 *          Copyright (C) 1992-2002 Free Software Foundation, Inc.          *
 *                                                                          *
 * GNAT is free software;  you can  redistribute it  and/or modify it under *
 * terms of the  GNU General Public License as published  by the Free Soft- *
 * ware  Foundation;  either version 2,  or (at your option) any later ver- *
 * sion.  GNAT is distributed in the hope that it will be useful, but WITH- *
 * OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License *
 * for  more details.  You should have  received  a copy of the GNU General *
 * Public License  distributed with GNAT;  see file COPYING.  If not, write *
 * to  the Free Software Foundation,  59 Temple Place - Suite 330,  Boston, *
 * MA 02111-1307, USA.                                                      *
 *                                                                          *
 * As a  special  exception,  if you  link  this file  with other  files to *
 * produce an executable,  this file does not by itself cause the resulting *
 * executable to be covered by the GNU General Public License. This except- *
 * ion does not  however invalidate  any other reasons  why the  executable *
 * file might be covered by the  GNU Public License.                        *
 *                                                                          *
 * GNAT was originally developed  by the GNAT team at  New York University. *
 * It is now maintained by Ada Core Technologies Inc (http://www.gnat.com). *
 *                                                                          *
 ****************************************************************************/

/* This file contains definitions to access front-end functions and
   variables used by gigi.  */

/* atree: */

#define Is_Rewrite_Substitution	atree__is_rewrite_substitution
#define Original_Node		atree__original_node

extern Boolean Is_Rewrite_Subsitution	PARAMS ((Node_Id));
extern Node_Id Original_Node		PARAMS ((Node_Id));

/* comperr:  */

#define Compiler_Abort comperr__compiler_abort
extern int Compiler_Abort PARAMS ((Fat_Pointer, int)) ATTRIBUTE_NORETURN;

/* csets: Definitions to access the front-end's character translation
   tables.  */

#define Fold_Lower(C) csets__fold_lower[C]
#define Fold_Upper(C) csets__fold_upper[C]
extern char Fold_Lower[], Fold_Upper[];

/* debug: */

#define Debug_Flag_XX debug__debug_flag_xx
#define Debug_Flag_NN debug__debug_flag_nn

extern Boolean Debug_Flag_XX;
extern Boolean Debug_Flag_NN;

/* einfo: We will be setting Esize for types, Component_Bit_Offset for fields,
   Alignment for types and objects, Component_Size for array types, and
   Present_Expr for N_Variant nodes.  */

#define Set_Alignment			einfo__set_alignment
#define Set_Esize			einfo__set_esize
#define Set_RM_Size			einfo__set_rm_size
#define Set_Component_Bit_Offset	einfo__set_component_bit_offset
#define Set_Component_Size		einfo__set_component_size
#define Set_Present_Expr		sinfo__set_present_expr

extern void Set_Alignment		PARAMS ((Entity_Id, Uint));
extern void Set_Component_Size		PARAMS ((Entity_Id, Uint));
extern void Set_Esize			PARAMS ((Entity_Id, Uint));
extern void Set_RM_Size			PARAMS ((Entity_Id, Uint));
extern void Set_Component_Bit_Offset	PARAMS ((Entity_Id, Uint));
extern void Set_Present_Expr		PARAMS ((Node_Id, Uint));

/* Test if the node N is the name of an entity (i.e. is an identifier,
   expanded name, or an attribute reference that returns an entity).  */
#define Is_Entity_Name einfo__is_entity_name
extern Boolean Is_Entity_Name		PARAMS ((Node_Id));

#define Get_Attribute_Definition_Clause einfo__get_attribute_definition_clause
extern Node_Id Get_Attribute_Definition_Clause PARAMS ((Entity_Id, char));

/* errout: */

#define Error_Msg_N	errout__error_msg_n
#define Error_Msg_NE	errout__error_msg_ne
#define Error_Msg_Node_2 errout__error_msg_node_2
#define Error_Msg_Uint_1 errout__error_msg_uint_1
#define Error_Msg_Uint_2 errout__error_msg_uint_2

extern void Error_Msg_N		PARAMS ((Fat_Pointer, Node_Id));
extern void Error_Msg_NE	PARAMS ((Fat_Pointer, Node_Id, Entity_Id));

extern Entity_Id Error_Msg_Node_2;
extern Uint Error_Msg_Uint_1;
extern Uint Error_Msg_Uint_2;

/* exp_code:  */
#define Asm_Input_Constraint exp_code__asm_input_constraint
#define Asm_Input_Value exp_code__asm_input_value
#define Asm_Output_Constraint exp_code__asm_output_constraint
#define Asm_Output_Variable exp_code__asm_output_variable
#define Asm_Template exp_code__asm_template
#define Clobber_Get_Next exp_code__clobber_get_next
#define Clobber_Setup exp_code__clobber_setup
#define Is_Asm_Volatile exp_code__is_asm_volatile
#define Next_Asm_Input exp_code__next_asm_input
#define Next_Asm_Output exp_code__next_asm_output
#define Setup_Asm_Inputs exp_code__setup_asm_inputs
#define Setup_Asm_Outputs exp_code__setup_asm_outputs

extern Node_Id Asm_Input_Constraint	PARAMS ((void));
extern Node_Id Asm_Input_Value		PARAMS ((void));
extern Node_Id Asm_Output_Constraint	PARAMS ((void));
extern Node_Id Asm_Output_Variable	PARAMS ((void));
extern Node_Id Asm_Template		PARAMS ((Node_Id));
extern char *Clobber_Get_Next		PARAMS ((void));
extern void Clobber_Setup		PARAMS ((Node_Id));
extern Boolean Is_Asm_Volatile		PARAMS ((Node_Id));
extern void Next_Asm_Input		PARAMS ((void));
extern void Next_Asm_Output		PARAMS ((void));
extern void Setup_Asm_Inputs		PARAMS ((Node_Id));
extern void Setup_Asm_Outputs		PARAMS ((Node_Id));

/* exp_dbug:  */

#define Get_Encoded_Name exp_dbug__get_encoded_name
#define Get_External_Name_With_Suffix exp_dbug__get_external_name_with_suffix

extern void Get_Encoded_Name	PARAMS ((Entity_Id));
extern void Get_External_Name_With_Suffix PARAMS ((Entity_Id, Fat_Pointer));

/* lib: */

#define Cunit 				lib__cunit
#define Ident_String			lib__ident_string
#define In_Extended_Main_Code_Unit	lib__in_extended_main_code_unit

extern Node_Id Cunit				PARAMS ((Unit_Number_Type));
extern Node_Id Ident_String			PARAMS ((Unit_Number_Type));
extern Boolean In_Extended_Main_Code_Unit	PARAMS ((Entity_Id));

/* opt: */

#define Global_Discard_Names opt__global_discard_names
#define Exception_Mechanism  opt__exception_mechanism

typedef enum {Setjmp_Longjmp, Front_End_ZCX, GCC_ZCX} Exception_Mechanism_Type;

extern Boolean Global_Discard_Names;
extern Exception_Mechanism_Type Exception_Mechanism;

/* restrict: */

#define Check_Elaboration_Code_Allowed restrict__check_elaboration_code_allowed
#define No_Exception_Handlers_Set restrict__no_exception_handlers_set

extern void Check_Elaboration_Code_Allowed	PARAMS ((Node_Id));
extern Boolean No_Exception_Handlers_Set	PARAMS ((void));

/* sem_eval: */

#define Compile_Time_Known_Value	sem_eval__compile_time_known_value
#define Expr_Value			sem_eval__expr_value
#define Expr_Value_S			sem_eval__expr_value_s
#define Is_OK_Static_Expression		sem_eval__is_ok_static_expression

extern Uint Expr_Value			PARAMS ((Node_Id));
extern Node_Id Expr_Value_S		PARAMS ((Node_Id));
extern Boolean Compile_Time_Known_Value PARAMS((Node_Id));
extern Boolean Is_OK_Static_Expression  PARAMS((Node_Id));

/* sem_util: */

#define Defining_Entity			sem_util__defining_entity
#define First_Actual			sem_util__first_actual
#define Next_Actual			sem_util__next_actual
#define Requires_Transient_Scope	sem_util__requires_transient_scope

extern Entity_Id Defining_Entity	PARAMS ((Node_Id));
extern Node_Id First_Actual		PARAMS ((Node_Id));
extern Node_Id Next_Actual		PARAMS ((Node_Id));
extern Boolean Requires_Transient_Scope PARAMS ((Entity_Id));

/* sinfo: These functions aren't in sinfo.h since we don't make the
   setting functions, just the retrieval functions.  */
#define Set_Has_No_Elaboration_Code sinfo__set_has_no_elaboration_code
extern void Set_Has_No_Elaboration_Code	PARAMS ((Node_Id, Boolean));

/* targparm: */

#define Stack_Check_Probes_On_Target targparm__stack_check_probes_on_target

extern Boolean Stack_Check_Probes_On_Target;
