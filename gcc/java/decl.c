/* Process declarations and variables for the GNU compiler for the
   Java(TM) language.
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001
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
Boston, MA 02111-1307, USA.

Java and all Java-based marks are trademarks or registered trademarks
of Sun Microsystems, Inc. in the United States and other countries.
The Free Software Foundation is independent of Sun Microsystems, Inc.  */

/* Hacked by Per Bothner <bothner@cygnus.com> February 1996. */

#include "config.h"
#include "system.h"
#include "tree.h"
#include "rtl.h"
#include "toplev.h"
#include "flags.h"
#include "java-tree.h"
#include "jcf.h"
#include "function.h"
#include "expr.h"
#include "libfuncs.h"
#include "except.h"
#include "java-except.h"
#include "ggc.h"

#if defined (DEBUG_JAVA_BINDING_LEVELS)
extern void indent PROTO((void));
#endif

static tree push_jvm_slot PARAMS ((int, tree));
static tree lookup_name_current_level PARAMS ((tree));
static tree push_promoted_type PARAMS ((const char *, tree));
static struct binding_level *make_binding_level PARAMS ((void));
static tree create_primitive_vtable PARAMS ((const char *));
static tree check_local_named_variable PARAMS ((tree, tree, int, int *));
static tree check_local_unnamed_variable PARAMS ((tree, tree, tree));

/* Set to non-zero value in order to emit class initilization code
   before static field references.  */
extern int always_initialize_class_p;

/* The DECL_MAP is a mapping from (index, type) to a decl node.
   If index < max_locals, it is the index of a local variable.
   if index >= max_locals, then index-max_locals is a stack slot.
   The DECL_MAP mapping is represented as a TREE_VEC whose elements
   are a list of decls (VAR_DECL or PARM_DECL) chained by
   DECL_LOCAL_SLOT_CHAIN; the index finds the TREE_VEC element, and then
   we search the chain for a decl with a matching TREE_TYPE. */

tree decl_map;

/* A list of local variables VAR_DECLs for this method that we have seen
   debug information, but we have not reached their starting (byte) PC yet. */

static tree pending_local_decls = NULL_TREE;

/* Push a local variable or stack slot into the decl_map,
   and assign it an rtl. */

#if defined(DEBUG_JAVA_BINDING_LEVELS)
int binding_depth = 0;
int is_class_level = 0;
int current_pc;

void
indent ()
{
  register unsigned i;

  for (i = 0; i < binding_depth*2; i++)
    putc (' ', stderr);
}
#endif /* defined(DEBUG_JAVA_BINDING_LEVELS) */

static tree
push_jvm_slot (index, decl)
     int index;
     tree decl;
{
  struct rtx_def *rtl = NULL;
  tree type = TREE_TYPE (decl);
  tree tmp;

  DECL_CONTEXT (decl) = current_function_decl;
  layout_decl (decl, 0);

  /* See if we have an appropriate rtl (i.e. same mode) at this index.
     If so, we must use it. */ 
  tmp = TREE_VEC_ELT (decl_map, index);
  while (tmp != NULL_TREE)
    {
      if (TYPE_MODE (type) == TYPE_MODE (TREE_TYPE (tmp)))
	rtl = DECL_RTL_IF_SET (tmp);
      if (rtl != NULL)
	break;
     tmp = DECL_LOCAL_SLOT_CHAIN (tmp);
    }
  if (rtl != NULL)
    SET_DECL_RTL (decl, rtl);
  else
    {
      if (index >= DECL_MAX_LOCALS (current_function_decl))
	DECL_REGISTER (decl) = 1;
      expand_decl (decl);
    }

  /* Now link the decl into the decl_map. */
  if (DECL_LANG_SPECIFIC (decl) == NULL)
    {
      MAYBE_CREATE_VAR_LANG_DECL_SPECIFIC (decl);
      DECL_LOCAL_START_PC (decl) = 0;
      DECL_LOCAL_END_PC (decl) = DECL_CODE_LENGTH (current_function_decl);
      DECL_LOCAL_SLOT_NUMBER (decl) = index;
    }
  DECL_LOCAL_SLOT_CHAIN (decl) = TREE_VEC_ELT (decl_map, index);
  TREE_VEC_ELT (decl_map, index) = decl;
  return decl;
}

/* Find out if 'decl' passed in fits the defined PC location better than
   'best'.  Return decl if it does, return best if it doesn't.  If decl
   is returned, then updated is set to true.  */

static tree
check_local_named_variable (best, decl, pc, updated)
     tree best;
     tree decl;
     int pc;
     int *updated;
{
  if (pc >= DECL_LOCAL_START_PC (decl)
      && pc < DECL_LOCAL_END_PC (decl))
    {
      if (best == NULL_TREE
	  || (DECL_LOCAL_START_PC (decl) > DECL_LOCAL_START_PC (best)
	      && DECL_LOCAL_END_PC (decl) < DECL_LOCAL_END_PC (best)))
        {
	  *updated = 1;
	  return decl;
	}
    }
  
  return best;
}

/* Find the best declaration based upon type.  If 'decl' fits 'type' better
   than 'best', return 'decl'.  Otherwise return 'best'.  */

static tree
check_local_unnamed_variable (best, decl, type)
     tree best;
     tree decl;
     tree type;
{
    if (TREE_TYPE (decl) == type
	|| (TREE_CODE (TREE_TYPE (decl)) == TREE_CODE (type)
	    && TYPE_PRECISION (TREE_TYPE (decl)) <= 32
	    && TYPE_PRECISION (type) <= 32
	    && TREE_CODE (type) != POINTER_TYPE)
	|| (TREE_CODE (TREE_TYPE (decl)) == POINTER_TYPE
	    && type == ptr_type_node))
      {
	if (best == NULL_TREE
	    || (TREE_TYPE (decl) == type && TREE_TYPE (best) != type))
	  return decl;
      }

    return best;
}


/* Find a VAR_DECL (or PARM_DECL) at local index INDEX that has type TYPE,
   that is valid at PC (or -1 if any pc).
   If there is no existing matching decl, allocate one.  */

tree
find_local_variable (index, type, pc)
     int index;
     tree type;
     int pc;
{
  tree decl = TREE_VEC_ELT (decl_map, index);
  tree best = NULL_TREE;
  int found_scoped_var = 0;

  /* Scan through every declaration that has been created in this slot. */
  while (decl != NULL_TREE)
    {
       /* Variables created in give_name_to_locals() have a name and have
 	 a specified scope, so we can handle them specifically.  We want
 	 to use the specific decls created for those so they are assigned
 	 the right variables in the debugging information. */
      if (DECL_NAME (decl) != NULL_TREE)
	{
	  /* This is a variable we have a name for, so it has a scope
	     supplied in the class file.  But it only matters when we
	     actually have a PC to use.  If pc<0, then we are asking
	     for a stack slot and this decl won't be one of those. */
 	  if (pc >= 0)
 	    best = check_local_named_variable (best, decl, pc,
 					       &found_scoped_var);
 	}
      /* We scan for type information unless we found a variable in the
	 proper scope already. */
      else if (!found_scoped_var)
 	{
 	  /* If we don't have scoping information for a variable, we use
 	     a different method to look it up. */
 	  best = check_local_unnamed_variable (best, decl, type);
 	}

      decl = DECL_LOCAL_SLOT_CHAIN (decl);
    }

  if (best != NULL_TREE)
    return best;

  /* If we don't find a match, create one with the type passed in. */
  return push_jvm_slot (index, build_decl (VAR_DECL, NULL_TREE, type));
}


/* Same as find_local_index, except that INDEX is a stack index. */

tree
find_stack_slot (index, type)
     int index;
     tree type;
{
  return find_local_variable (index + DECL_MAX_LOCALS (current_function_decl),
			      type, -1);
}

struct binding_level
  {
    /* A chain of _DECL nodes for all variables, constants, functions,
     * and typedef types.  These are in the reverse of the order supplied.
     */
    tree names;

    /* For each level, a list of shadowed outer-level local definitions
       to be restored when this level is popped.
       Each link is a TREE_LIST whose TREE_PURPOSE is an identifier and
       whose TREE_VALUE is its old definition (a kind of ..._DECL node).  */
    tree shadowed;

    /* For each level (except not the global one),
       a chain of BLOCK nodes for all the levels
       that were entered and exited one level down.  */
    tree blocks;

    /* The BLOCK node for this level, if one has been preallocated.
       If 0, the BLOCK is allocated (if needed) when the level is popped.  */
    tree this_block;

    /* The binding level which this one is contained in (inherits from).  */
    struct binding_level *level_chain;

    /* The bytecode PC that marks the end of this level. */
    int end_pc;
    /* The bytecode PC that marks the start of this level. */
    int start_pc;

#if defined(DEBUG_JAVA_BINDING_LEVELS)
    /* Binding depth at which this level began.  */
    unsigned binding_depth;
#endif /* defined(DEBUG_JAVA_BINDING_LEVELS) */
  };

#define NULL_BINDING_LEVEL (struct binding_level *) NULL

/* The binding level currently in effect.  */

static struct binding_level *current_binding_level;

/* A chain of binding_level structures awaiting reuse.  */

static struct binding_level *free_binding_level;

/* The outermost binding level, for names of file scope.
   This is created when the compiler is started and exists
   through the entire run.  */

static struct binding_level *global_binding_level;

/* A PC value bigger than any PC value we may ever may encounter. */

#define LARGEST_PC (( (unsigned int)1 << (HOST_BITS_PER_INT - 1)) - 1)

/* Binding level structures are initialized by copying this one.  */

static struct binding_level clear_binding_level
  = {NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE,
       NULL_BINDING_LEVEL, LARGEST_PC, 0};

#if 0
/* A list (chain of TREE_LIST nodes) of all LABEL_DECLs in the function
   that have names.  Here so we can clear out their names' definitions
   at the end of the function.  */

static tree named_labels;

/* A list of LABEL_DECLs from outer contexts that are currently shadowed.  */

static tree shadowed_labels;
#endif

tree java_global_trees[JTI_MAX];
  
/* Build (and pushdecl) a "promoted type" for all standard
   types shorter than int.  */

static tree
push_promoted_type (name, actual_type)
     const char *name;
     tree actual_type;
{
  tree type = make_node (TREE_CODE (actual_type));
#if 1
  tree in_min = TYPE_MIN_VALUE (int_type_node);
  tree in_max = TYPE_MAX_VALUE (int_type_node);
#else
  tree in_min = TYPE_MIN_VALUE (actual_type);
  tree in_max = TYPE_MAX_VALUE (actual_type);
#endif
  TYPE_MIN_VALUE (type) = copy_node (in_min);
  TREE_TYPE (TYPE_MIN_VALUE (type)) = type;
  TYPE_MAX_VALUE (type) = copy_node (in_max);
  TREE_TYPE (TYPE_MAX_VALUE (type)) = type;
  TYPE_PRECISION (type) = TYPE_PRECISION (int_type_node);
  layout_type (type);
  pushdecl (build_decl (TYPE_DECL, get_identifier (name), type));
  return type;
}

/* Return a definition for a builtin function named NAME and whose data type
   is TYPE.  TYPE should be a function type with argument types.
   FUNCTION_CODE tells later passes how to compile calls to this function.
   See tree.h for its possible values.

   If LIBRARY_NAME is nonzero, use that for DECL_ASSEMBLER_NAME,
   the name to be called if we can't opencode the function.  */

tree
builtin_function (name, type, function_code, class, library_name)
     const char *name;
     tree type;
     int function_code;
     enum built_in_class class;
     const char *library_name;
{
  tree decl = build_decl (FUNCTION_DECL, get_identifier (name), type);
  DECL_EXTERNAL (decl) = 1;
  TREE_PUBLIC (decl) = 1;
  if (library_name)
    SET_DECL_ASSEMBLER_NAME (decl, get_identifier (library_name));
  make_decl_rtl (decl, NULL);
  pushdecl (decl);
  DECL_BUILT_IN_CLASS (decl) = class;
  DECL_FUNCTION_CODE (decl) = function_code;
  return decl;
}

/* Return tree that represents a vtable for a primitive array.  */
static tree
create_primitive_vtable (name)
     const char *name;
{
  tree r;
  char buf[50];

  sprintf (buf, "_Jv_%sVTable", name);
  r = build_decl (VAR_DECL, get_identifier (buf), ptr_type_node);
  DECL_EXTERNAL (r) = 1;
  return r;
}

void
java_init_decl_processing ()
{
  register tree endlink;
  tree field = NULL_TREE;
  tree t;

  init_class_processing ();

  current_function_decl = NULL;
  current_binding_level = NULL_BINDING_LEVEL;
  free_binding_level = NULL_BINDING_LEVEL;
  pushlevel (0);	/* make the binding_level structure for global names */
  global_binding_level = current_binding_level;

  /* The code here must be similar to build_common_tree_nodes{,_2} in
     tree.c, especially as to the order of initializing common nodes.  */
  error_mark_node = make_node (ERROR_MARK);
  TREE_TYPE (error_mark_node) = error_mark_node;

  /* Create sizetype first - needed for other types. */
  initialize_sizetypes ();

  byte_type_node = make_signed_type (8);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("byte"), byte_type_node));
  short_type_node = make_signed_type (16);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("short"), short_type_node));
  int_type_node = make_signed_type (32);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("int"), int_type_node));
  long_type_node = make_signed_type (64);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("long"), long_type_node));

  unsigned_byte_type_node = make_unsigned_type (8);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("unsigned byte"),
			unsigned_byte_type_node));
  unsigned_short_type_node = make_unsigned_type (16);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("unsigned short"),
			unsigned_short_type_node));
  unsigned_int_type_node = make_unsigned_type (32);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("unsigned int"),
			unsigned_int_type_node));
  unsigned_long_type_node = make_unsigned_type (64);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("unsigned long"),
			unsigned_long_type_node));

  set_sizetype (make_unsigned_type (POINTER_SIZE));

  /* Define these next since types below may used them.  */
  integer_type_node = java_type_for_size (INT_TYPE_SIZE, 0);
  integer_zero_node = build_int_2 (0, 0);
  integer_one_node = build_int_2 (1, 0);
  integer_two_node = build_int_2 (2, 0);
  integer_four_node = build_int_2 (4, 0);
  integer_minus_one_node = build_int_2 (-1, -1);

  size_zero_node = size_int (0);
  size_one_node = size_int (1);
  bitsize_zero_node = bitsize_int (0);
  bitsize_one_node = bitsize_int (1);
  bitsize_unit_node = bitsize_int (BITS_PER_UNIT);

  long_zero_node = build_int_2 (0, 0);
  TREE_TYPE (long_zero_node) = long_type_node;

  void_type_node = make_node (VOID_TYPE);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("void"), void_type_node));
  layout_type (void_type_node);	/* Uses size_zero_node */
  ptr_type_node = build_pointer_type (void_type_node);
  t = make_node (VOID_TYPE);
  layout_type (t); /* Uses size_zero_node */
  return_address_type_node = build_pointer_type (t);

  null_pointer_node = build_int_2 (0, 0);
  TREE_TYPE (null_pointer_node) = ptr_type_node;

  /* Used by the parser to represent empty statements and blocks. */
  empty_stmt_node = build1 (NOP_EXPR, void_type_node, size_zero_node);
  CAN_COMPLETE_NORMALLY (empty_stmt_node) = 1;

#if 0
  /* Make a type to be the domain of a few array types
     whose domains don't really matter.
     200 is small enough that it always fits in size_t
     and large enough that it can hold most function names for the
     initializations of __FUNCTION__ and __PRETTY_FUNCTION__.  */
  short_array_type_node = build_prim_array_type (short_type_node, 200);
#endif
  char_type_node = make_node (CHAR_TYPE);
  TYPE_PRECISION (char_type_node) = 16;
  fixup_unsigned_type (char_type_node);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("char"), char_type_node));

  boolean_type_node = make_node (BOOLEAN_TYPE);
  TYPE_PRECISION (boolean_type_node) = 1;
  fixup_unsigned_type (boolean_type_node);
  pushdecl (build_decl (TYPE_DECL, get_identifier ("boolean"),
			boolean_type_node));
  boolean_false_node = TYPE_MIN_VALUE (boolean_type_node);
  boolean_true_node = TYPE_MAX_VALUE (boolean_type_node);

  promoted_byte_type_node
    = push_promoted_type ("promoted_byte", byte_type_node);
  promoted_short_type_node
    = push_promoted_type ("promoted_short", short_type_node);
  promoted_char_type_node
    = push_promoted_type ("promoted_char", char_type_node);
  promoted_boolean_type_node
    = push_promoted_type ("promoted_boolean", boolean_type_node);

  float_type_node = make_node (REAL_TYPE);
  TYPE_PRECISION (float_type_node) = 32;
  pushdecl (build_decl (TYPE_DECL, get_identifier ("float"),
                        float_type_node));
  layout_type (float_type_node);

  double_type_node = make_node (REAL_TYPE);
  TYPE_PRECISION (double_type_node) = 64;
  pushdecl (build_decl (TYPE_DECL, get_identifier ("double"),
                        double_type_node));
  layout_type (double_type_node);

  float_zero_node = build_real (float_type_node, dconst0);
  double_zero_node = build_real (double_type_node, dconst0);

  /* These are the vtables for arrays of primitives.  */
  boolean_array_vtable = create_primitive_vtable ("boolean");
  byte_array_vtable = create_primitive_vtable ("byte");
  char_array_vtable = create_primitive_vtable ("char");
  short_array_vtable = create_primitive_vtable ("short");
  int_array_vtable = create_primitive_vtable ("int");
  long_array_vtable = create_primitive_vtable ("long");
  float_array_vtable = create_primitive_vtable ("float");
  double_array_vtable = create_primitive_vtable ("double");

  /* As you're adding items here, please update the code right after
     this section, so that the filename containing the source code of
     the pre-defined class gets registered correctly. */
  unqualified_object_id_node = get_identifier ("Object");
  object_type_node = lookup_class (get_identifier ("java.lang.Object"));
  object_ptr_type_node = promote_type (object_type_node);
  string_type_node = lookup_class (get_identifier ("java.lang.String"));
  string_ptr_type_node = promote_type (string_type_node);
  class_type_node = lookup_class (get_identifier ("java.lang.Class"));
  throwable_type_node = lookup_class (get_identifier ("java.lang.Throwable"));
  exception_type_node = lookup_class (get_identifier ("java.lang.Exception"));
  runtime_exception_type_node = 
    lookup_class (get_identifier ("java.lang.RuntimeException"));
  error_exception_type_node = 
    lookup_class (get_identifier ("java.lang.Error"));
  class_not_found_type_node = 
    lookup_class (get_identifier ("java.lang.ClassNotFoundException"));
  no_class_def_found_type_node = 
    lookup_class (get_identifier ("java.lang.NoClassDefFoundError"));

  rawdata_ptr_type_node
    = promote_type (lookup_class (get_identifier ("gnu.gcj.RawData")));

  add_predefined_file (get_identifier ("java/lang/Class.java"));
  add_predefined_file (get_identifier ("java/lang/Error.java"));
  add_predefined_file (get_identifier ("java/lang/Object.java"));
  add_predefined_file (get_identifier ("java/lang/RuntimeException.java"));
  add_predefined_file (get_identifier ("java/lang/String.java"));
  add_predefined_file (get_identifier ("java/lang/Throwable.java"));
  add_predefined_file (get_identifier ("gnu/gcj/RawData.java"));
  add_predefined_file (get_identifier ("java/lang/Exception.java"));
  add_predefined_file (get_identifier ("java/lang/ClassNotFoundException.java"));
  add_predefined_file (get_identifier ("java/lang/NoClassDefFoundError.java"));
  add_predefined_file (get_identifier ("gnu/gcj/RawData.java"));

  methodtable_type = make_node (RECORD_TYPE);
  layout_type (methodtable_type);
  build_decl (TYPE_DECL, get_identifier ("methodtable"), methodtable_type);
  methodtable_ptr_type = build_pointer_type (methodtable_type);

  TYPE_identifier_node = get_identifier ("TYPE");
  init_identifier_node = get_identifier ("<init>");
  clinit_identifier_node = get_identifier ("<clinit>");
  finit_identifier_node = get_identifier ("finit$");
  instinit_identifier_node = get_identifier ("instinit$");
  void_signature_node = get_identifier ("()V");
  length_identifier_node = get_identifier ("length");
  finalize_identifier_node = get_identifier ("finalize");
  this_identifier_node = get_identifier ("this");
  super_identifier_node = get_identifier ("super");
  continue_identifier_node = get_identifier ("continue");
  access0_identifier_node = get_identifier ("access$0");
  classdollar_identifier_node = get_identifier ("class$");

  /* for lack of a better place to put this stub call */
  init_expr_processing();

  utf8const_type = make_node (RECORD_TYPE);
  PUSH_FIELD (utf8const_type, field, "hash", unsigned_short_type_node);
  PUSH_FIELD (utf8const_type, field, "length", unsigned_short_type_node);
  FINISH_RECORD (utf8const_type);
  utf8const_ptr_type = build_pointer_type (utf8const_type);

  constants_type_node = make_node (RECORD_TYPE);
  PUSH_FIELD (constants_type_node, field, "size", unsigned_int_type_node);
  PUSH_FIELD (constants_type_node, field, "tags", ptr_type_node);
  PUSH_FIELD (constants_type_node, field, "data", ptr_type_node);
  FINISH_RECORD (constants_type_node);
  build_decl (TYPE_DECL, get_identifier ("constants"), constants_type_node);

  access_flags_type_node = unsigned_short_type_node;

  dtable_type = make_node (RECORD_TYPE);
  dtable_ptr_type = build_pointer_type (dtable_type);

  one_elt_array_domain_type = build_index_type (integer_one_node);
  otable_type = build_array_type (integer_type_node, 
				  one_elt_array_domain_type);
  TYPE_NONALIASED_COMPONENT (otable_type) = 1;
  otable_ptr_type = build_pointer_type (otable_type);

  method_symbol_type = make_node (RECORD_TYPE);
  PUSH_FIELD (method_symbol_type, field, "clname", utf8const_ptr_type);
  PUSH_FIELD (method_symbol_type, field, "name", utf8const_ptr_type);
  PUSH_FIELD (method_symbol_type, field, "signature", utf8const_ptr_type);
  FINISH_RECORD (method_symbol_type);

  method_symbols_array_type = build_array_type (method_symbol_type, 
						one_elt_array_domain_type);
  method_symbols_array_ptr_type = build_pointer_type 
				  (method_symbols_array_type);

  otable_decl = build_decl (VAR_DECL, get_identifier ("otable"), otable_type);
  DECL_EXTERNAL (otable_decl) = 1;
  TREE_STATIC (otable_decl) = 1;
  TREE_READONLY (otable_decl) = 1;
  pushdecl (otable_decl);
  
  otable_syms_decl = build_decl (VAR_DECL, get_identifier ("otable_syms"), 
    method_symbols_array_type);
  TREE_STATIC (otable_syms_decl) = 1;
  TREE_CONSTANT (otable_syms_decl) = 1;
  pushdecl (otable_syms_decl);
  
  PUSH_FIELD (object_type_node, field, "vtable", dtable_ptr_type);
  /* This isn't exactly true, but it is what we have in the source.
     There is an unresolved issue here, which is whether the vtable
     should be marked by the GC.  */
  if (! flag_hash_synchronization)
    PUSH_FIELD (object_type_node, field, "sync_info",
		build_pointer_type (object_type_node));
  for (t = TYPE_FIELDS (object_type_node); t != NULL_TREE; t = TREE_CHAIN (t))
    FIELD_PRIVATE (t) = 1;
  FINISH_RECORD (object_type_node);

  field_type_node = make_node (RECORD_TYPE);
  field_ptr_type_node = build_pointer_type (field_type_node);
  method_type_node = make_node (RECORD_TYPE);
  method_ptr_type_node = build_pointer_type (method_type_node);

  set_super_info (0, class_type_node, object_type_node, 0);
  set_super_info (0, string_type_node, object_type_node, 0);
  class_ptr_type = build_pointer_type (class_type_node);

  PUSH_FIELD (class_type_node, field, "next", class_ptr_type);
  PUSH_FIELD (class_type_node, field, "name", utf8const_ptr_type);
  PUSH_FIELD (class_type_node, field, "accflags", access_flags_type_node);
  PUSH_FIELD (class_type_node, field, "superclass", class_ptr_type);
  PUSH_FIELD (class_type_node, field, "constants", constants_type_node);
  PUSH_FIELD (class_type_node, field, "methods", method_ptr_type_node);
  PUSH_FIELD (class_type_node, field, "method_count", short_type_node);
  PUSH_FIELD (class_type_node, field, "vtable_method_count", short_type_node);
  PUSH_FIELD (class_type_node, field, "fields", field_ptr_type_node);
  PUSH_FIELD (class_type_node, field, "size_in_bytes", int_type_node);
  PUSH_FIELD (class_type_node, field, "field_count", short_type_node);
  PUSH_FIELD (class_type_node, field, "static_field_count", short_type_node);
  PUSH_FIELD (class_type_node, field, "vtable", dtable_ptr_type);
  PUSH_FIELD (class_type_node, field, "otable", otable_ptr_type);
  PUSH_FIELD (class_type_node, field, "otable_syms", 
  	      method_symbols_array_ptr_type);
  PUSH_FIELD (class_type_node, field, "interfaces",
	      build_pointer_type (class_ptr_type));
  PUSH_FIELD (class_type_node, field, "loader", ptr_type_node);
  PUSH_FIELD (class_type_node, field, "interface_count", short_type_node);
  PUSH_FIELD (class_type_node, field, "state", byte_type_node);
  PUSH_FIELD (class_type_node, field, "thread", ptr_type_node);
  PUSH_FIELD (class_type_node, field, "depth", short_type_node);
  PUSH_FIELD (class_type_node, field, "ancestors", ptr_type_node);
  PUSH_FIELD (class_type_node, field, "idt", ptr_type_node);  
  PUSH_FIELD (class_type_node, field, "arrayclass", ptr_type_node);  
  PUSH_FIELD (class_type_node, field, "protectionDomain", ptr_type_node);
  for (t = TYPE_FIELDS (class_type_node);  t != NULL_TREE;  t = TREE_CHAIN (t))
    FIELD_PRIVATE (t) = 1;
  push_super_field (class_type_node, object_type_node);

  /* Hash synchronization requires at least double-word alignment. */
  if (flag_hash_synchronization && POINTER_SIZE < 64)
    TYPE_ALIGN (class_type_node) = 64;

  FINISH_RECORD (class_type_node);
  build_decl (TYPE_DECL, get_identifier ("Class"), class_type_node);

  field_info_union_node = make_node (UNION_TYPE);
  PUSH_FIELD (field_info_union_node, field, "boffset", int_type_node);
  PUSH_FIELD (field_info_union_node, field, "addr", ptr_type_node);
#if 0
  PUSH_FIELD (field_info_union_node, field, "idx", unsigned_short_type_node);
#endif
  layout_type (field_info_union_node);

  PUSH_FIELD (field_type_node, field, "name", utf8const_ptr_type);
  PUSH_FIELD (field_type_node, field, "type", class_ptr_type);
  PUSH_FIELD (field_type_node, field, "accflags", access_flags_type_node);
  PUSH_FIELD (field_type_node, field, "bsize", unsigned_short_type_node);
  PUSH_FIELD (field_type_node, field, "info", field_info_union_node);
  FINISH_RECORD (field_type_node);
  build_decl (TYPE_DECL, get_identifier ("Field"), field_type_node);

  nativecode_ptr_array_type_node
    = build_array_type (nativecode_ptr_type_node, one_elt_array_domain_type);

  PUSH_FIELD (dtable_type, field, "class", class_ptr_type);
  PUSH_FIELD (dtable_type, field, "methods", nativecode_ptr_array_type_node);
  FINISH_RECORD (dtable_type);
  build_decl (TYPE_DECL, get_identifier ("dispatchTable"), dtable_type);

#define jint_type int_type_node
#define jint_ptr_type ptr_type_node

  jexception_type = make_node (RECORD_TYPE);
  PUSH_FIELD (jexception_type, field, "start_pc", ptr_type_node);
  PUSH_FIELD (jexception_type, field, "end_pc", ptr_type_node);
  PUSH_FIELD (jexception_type, field, "handler_pc", ptr_type_node);
  PUSH_FIELD (jexception_type, field, "catch_type", class_ptr_type);
  FINISH_RECORD (jexception_type);
  build_decl (TYPE_DECL, get_identifier ("jexception"), field_type_node);
  jexception_ptr_type = build_pointer_type (jexception_type);

  lineNumberEntry_type = make_node (RECORD_TYPE);
  PUSH_FIELD (lineNumberEntry_type, field, "line_nr", unsigned_short_type_node);
  PUSH_FIELD (lineNumberEntry_type, field, "start_pc", ptr_type_node);
  FINISH_RECORD (lineNumberEntry_type);

  lineNumbers_type = make_node (RECORD_TYPE);
  PUSH_FIELD (lineNumbers_type, field, "length", unsigned_int_type_node);
  FINISH_RECORD (lineNumbers_type);

#define instn_ptr_type_node ptr_type_node	/* XXX JH */

#define lineNumbers_ptr_type_node build_pointer_type(lineNumbers_type)

  PUSH_FIELD (method_type_node, field, "name", utf8const_ptr_type);
  PUSH_FIELD (method_type_node, field, "signature", utf8const_ptr_type);
  PUSH_FIELD (method_type_node, field, "accflags", access_flags_type_node);
  PUSH_FIELD (method_type_node, field, "index", unsigned_short_type_node);
  PUSH_FIELD (method_type_node, field, "ncode", nativecode_ptr_type_node);
  PUSH_FIELD (method_type_node, field, "throws", ptr_type_node);
  FINISH_RECORD (method_type_node);
  build_decl (TYPE_DECL, get_identifier ("Method"), method_type_node);

  endlink = end_params_node = tree_cons (NULL_TREE, void_type_node, NULL_TREE);

  t = tree_cons (NULL_TREE, class_ptr_type,
		 tree_cons (NULL_TREE, int_type_node, endlink));
  alloc_object_node = builtin_function ("_Jv_AllocObject",
					build_function_type (ptr_type_node, t),
					0, NOT_BUILT_IN, NULL);
  DECL_IS_MALLOC (alloc_object_node) = 1;
  alloc_no_finalizer_node = 
    builtin_function ("_Jv_AllocObjectNoFinalizer",
		      build_function_type (ptr_type_node, t),
		      0, NOT_BUILT_IN, NULL);
  DECL_IS_MALLOC (alloc_no_finalizer_node) = 1;

  t = tree_cons (NULL_TREE, ptr_type_node, endlink);
  soft_initclass_node = builtin_function ("_Jv_InitClass",
					  build_function_type (void_type_node,
							       t),
					  0, NOT_BUILT_IN, NULL);

  throw_node = builtin_function ("_Jv_Throw",
				 build_function_type (ptr_type_node, t),
				 0, NOT_BUILT_IN, NULL);
  /* Mark throw_nodes as `noreturn' functions with side effects.  */
  TREE_THIS_VOLATILE (throw_node) = 1;
  TREE_SIDE_EFFECTS (throw_node) = 1;

  t = build_function_type (int_type_node, endlink);
  soft_monitorenter_node 
    = builtin_function ("_Jv_MonitorEnter", t, 0, NOT_BUILT_IN, NULL);
  soft_monitorexit_node 
    = builtin_function ("_Jv_MonitorExit", t, 0, NOT_BUILT_IN, NULL);
  
  t = tree_cons (NULL_TREE, int_type_node, 
		 tree_cons (NULL_TREE, int_type_node, endlink));
  soft_newarray_node
      = builtin_function ("_Jv_NewPrimArray",
			  build_function_type(ptr_type_node, t),
			  0, NOT_BUILT_IN, NULL);
  DECL_IS_MALLOC (soft_newarray_node) = 1;

  t = tree_cons (NULL_TREE, int_type_node,
		 tree_cons (NULL_TREE, class_ptr_type,
			    tree_cons (NULL_TREE, object_ptr_type_node, endlink)));
  soft_anewarray_node
      = builtin_function ("_Jv_NewObjectArray",
			  build_function_type (ptr_type_node, t),
			  0, NOT_BUILT_IN, NULL);
  DECL_IS_MALLOC (soft_anewarray_node) = 1;

  t = tree_cons (NULL_TREE, ptr_type_node,
		 tree_cons (NULL_TREE, int_type_node, endlink));
  soft_multianewarray_node
      = builtin_function ("_Jv_NewMultiArray",
			  build_function_type (ptr_type_node, t),
			  0, NOT_BUILT_IN, NULL);
  DECL_IS_MALLOC (soft_multianewarray_node) = 1;

  t = build_function_type (void_type_node, 
			   tree_cons (NULL_TREE, int_type_node, endlink));
  soft_badarrayindex_node
      = builtin_function ("_Jv_ThrowBadArrayIndex", t, 
			  0, NOT_BUILT_IN, NULL);
  /* Mark soft_badarrayindex_node as a `noreturn' function with side
     effects.  */
  TREE_THIS_VOLATILE (soft_badarrayindex_node) = 1;
  TREE_SIDE_EFFECTS (soft_badarrayindex_node) = 1;

  soft_nullpointer_node
    = builtin_function ("_Jv_ThrowNullPointerException",
			build_function_type (void_type_node, endlink),
			0, NOT_BUILT_IN, NULL);
  /* Mark soft_nullpointer_node as a `noreturn' function with side
     effects.  */
  TREE_THIS_VOLATILE (soft_nullpointer_node) = 1;
  TREE_SIDE_EFFECTS (soft_nullpointer_node) = 1;

  t = tree_cons (NULL_TREE, class_ptr_type,
		 tree_cons (NULL_TREE, object_ptr_type_node, endlink));
  soft_checkcast_node
    = builtin_function ("_Jv_CheckCast",
			build_function_type (ptr_type_node, t),
			0, NOT_BUILT_IN, NULL);
  t = tree_cons (NULL_TREE, object_ptr_type_node,
		 tree_cons (NULL_TREE, class_ptr_type, endlink));
  soft_instanceof_node
    = builtin_function ("_Jv_IsInstanceOf",
			build_function_type (boolean_type_node, t),
			0, NOT_BUILT_IN, NULL);
  t = tree_cons (NULL_TREE, object_ptr_type_node,
		 tree_cons (NULL_TREE, object_ptr_type_node, endlink));
  soft_checkarraystore_node
    = builtin_function ("_Jv_CheckArrayStore",
			build_function_type (void_type_node, t),
			0, NOT_BUILT_IN, NULL);
  t = tree_cons (NULL_TREE, ptr_type_node,
		 tree_cons (NULL_TREE, ptr_type_node,
			    tree_cons (NULL_TREE, int_type_node, endlink)));
  soft_lookupinterfacemethod_node 
    = builtin_function ("_Jv_LookupInterfaceMethodIdx",
			build_function_type (ptr_type_node, t),
			0, NOT_BUILT_IN, NULL);

  t = tree_cons (NULL_TREE, object_ptr_type_node,
		 tree_cons (NULL_TREE, ptr_type_node,
			    tree_cons (NULL_TREE, ptr_type_node, endlink)));
  soft_lookupjnimethod_node
    = builtin_function ("_Jv_LookupJNIMethod",
			build_function_type (ptr_type_node, t),
			0, NOT_BUILT_IN, NULL);
  t = tree_cons (NULL_TREE, ptr_type_node, endlink);
  soft_getjnienvnewframe_node
    = builtin_function ("_Jv_GetJNIEnvNewFrame",
			build_function_type (ptr_type_node, t),
			0, NOT_BUILT_IN, NULL);
  soft_jnipopsystemframe_node
    = builtin_function ("_Jv_JNI_PopSystemFrame",
			build_function_type (ptr_type_node, t),
			0, NOT_BUILT_IN, NULL);

  t = tree_cons (NULL_TREE, double_type_node,
		 tree_cons (NULL_TREE, double_type_node, endlink));
  soft_fmod_node
    = builtin_function ("__builtin_fmod",
			build_function_type (double_type_node, t),
			BUILT_IN_FMOD, BUILT_IN_NORMAL, "fmod");

#if 0
  t = tree_cons (NULL_TREE, float_type_node,
		 tree_cons (NULL_TREE, float_type_node, endlink));
  soft_fmodf_node
    = builtin_function ("__builtin_fmodf",
			build_function_type (float_type_node, t),
			BUILT_IN_FMOD, BUILT_IN_NORMAL, "fmodf");
#endif
    
  soft_idiv_node
    = builtin_function ("_Jv_divI",
			build_function_type (int_type_node, t),
			0, NOT_BUILT_IN, NULL);

  soft_irem_node
    = builtin_function ("_Jv_remI",
			build_function_type (int_type_node, t),
			0, NOT_BUILT_IN, NULL);

  soft_ldiv_node
    = builtin_function ("_Jv_divJ",
			build_function_type (long_type_node, t),
			0, NOT_BUILT_IN, NULL);

  soft_lrem_node
    = builtin_function ("_Jv_remJ",
			build_function_type (long_type_node, t),
			0, NOT_BUILT_IN, NULL);

  /* Initialize variables for except.c.  */
  eh_personality_libfunc = init_one_libfunc (USING_SJLJ_EXCEPTIONS
                                             ? "__gcj_personality_sj0"
                                             : "__gcj_personality_v0");
  lang_eh_runtime_type = prepare_eh_table_type;

  init_jcf_parse ();

  /* Register nodes with the garbage collector.  */
  ggc_add_tree_root (java_global_trees, ARRAY_SIZE (java_global_trees));
  ggc_add_tree_root (&decl_map, 1);
  ggc_add_tree_root (&pending_local_decls, 1);

  initialize_builtins ();
}


/* Look up NAME in the current binding level and its superiors
   in the namespace of variables, functions and typedefs.
   Return a ..._DECL node of some kind representing its definition,
   or return 0 if it is undefined.  */

tree
lookup_name (name)
     tree name;
{
  register tree val;
  if (current_binding_level != global_binding_level
      && IDENTIFIER_LOCAL_VALUE (name))
    val = IDENTIFIER_LOCAL_VALUE (name);
  else
    val = IDENTIFIER_GLOBAL_VALUE (name);
  return val;
}

/* Similar to `lookup_name' but look only at current binding level and
   the previous one if its the parameter level.  */

static tree
lookup_name_current_level (name)
     tree name;
{
  register tree t;

  if (current_binding_level == global_binding_level)
    return IDENTIFIER_GLOBAL_VALUE (name);

  if (IDENTIFIER_LOCAL_VALUE (name) == 0)
    return 0;

  for (t = current_binding_level->names; t; t = TREE_CHAIN (t))
    if (DECL_NAME (t) == name)
      break;

  return t;
}

/* Use a binding level to record a labeled block declaration */

void
push_labeled_block (lb)
    tree lb;
{
  register tree name = DECL_NAME (LABELED_BLOCK_LABEL (lb));
  register struct binding_level *b = current_binding_level;
  tree oldlocal = IDENTIFIER_LOCAL_VALUE (name);
  if (oldlocal != 0)
      b->shadowed = tree_cons (name, oldlocal, b->shadowed);
  TREE_CHAIN (lb) = b->names;
  b->names = lb;
  IDENTIFIER_LOCAL_VALUE (name) = lb;
}

/* Pop the current binding level, reinstalling values for the previous
   labeled block */

void
pop_labeled_block ()
{
  struct binding_level *b = current_binding_level;
  tree label =  b->names;
  IDENTIFIER_LOCAL_VALUE (DECL_NAME (LABELED_BLOCK_LABEL (label))) = 
    NULL_TREE;
  if (b->shadowed)
    IDENTIFIER_LOCAL_VALUE (TREE_PURPOSE (b->shadowed)) = 
      TREE_VALUE (b->shadowed);

  /* Pop the current level, and free the structure for reuse.  */
  current_binding_level = current_binding_level->level_chain;
  b->level_chain = free_binding_level;
  free_binding_level = b;
}

/* Record a decl-node X as belonging to the current lexical scope.
   Check for errors (such as an incompatible declaration for the same
   name already seen in the same scope).

   Returns either X or an old decl for the same name.
   If an old decl is returned, it may have been smashed
   to agree with what X says.  */

tree
pushdecl (x)
     tree x;
{
  register tree t;
  register tree name = DECL_NAME (x);
  register struct binding_level *b = current_binding_level;
  
  if (TREE_CODE (x) != TYPE_DECL)
    DECL_CONTEXT (x) = current_function_decl;
  if (name)
    {
      const char *file;
      int line;

      t = lookup_name_current_level (name);
      if (t != 0 && t == error_mark_node)
	/* error_mark_node is 0 for a while during initialization!  */
	{
	  t = 0;
	  error_with_decl (x, "`%s' used prior to declaration");
	}

      if (t != 0)
	{
	  file = DECL_SOURCE_FILE (t);
	  line = DECL_SOURCE_LINE (t);
	}

      /* If we're naming a hitherto-unnamed type, set its TYPE_NAME
	 to point to the TYPE_DECL.
	 Since Java does not have typedefs, a type can only have
	 one (true) name, given by a class, interface, or builtin. */
      if (TREE_CODE (x) == TYPE_DECL
	  && TYPE_NAME (TREE_TYPE (x)) == 0
	  && TREE_TYPE (x) != error_mark_node)
	{
	  TYPE_NAME (TREE_TYPE (x)) = x;
	  TYPE_STUB_DECL (TREE_TYPE (x)) = x;
	}

      /* This name is new in its binding level.
	 Install the new declaration and return it.  */
      if (b == global_binding_level)
	{
	  /* Install a global value.  */
	  
	  IDENTIFIER_GLOBAL_VALUE (name) = x;
	}
      else
	{
	  /* Here to install a non-global value.  */
	  tree oldlocal = IDENTIFIER_LOCAL_VALUE (name);
	  IDENTIFIER_LOCAL_VALUE (name) = x;

#if 0
	  /* Warn if shadowing an argument at the top level of the body.  */
	  if (oldlocal != 0 && !DECL_EXTERNAL (x)
	      /* This warning doesn't apply to the parms of a nested fcn.  */
	      && ! current_binding_level->parm_flag
	      /* Check that this is one level down from the parms.  */
	      && current_binding_level->level_chain->parm_flag
	      /* Check that the decl being shadowed
		 comes from the parm level, one level up.  */
	      && chain_member (oldlocal, current_binding_level->level_chain->names))
	    {
	      if (TREE_CODE (oldlocal) == PARM_DECL)
		pedwarn ("declaration of `%s' shadows a parameter",
			 IDENTIFIER_POINTER (name));
	      else
		pedwarn ("declaration of `%s' shadows a symbol from the parameter list",
			 IDENTIFIER_POINTER (name));
	    }

	  /* Maybe warn if shadowing something else.  */
	  else if (warn_shadow && !DECL_EXTERNAL (x)
		   /* No shadow warnings for internally generated vars.  */
		   && DECL_SOURCE_LINE (x) != 0
		   /* No shadow warnings for vars made for inlining.  */
		   && ! DECL_FROM_INLINE (x))
	    {
	      const char *warnstring = 0;

	      if (TREE_CODE (x) == PARM_DECL
		  && current_binding_level->level_chain->parm_flag)
		/* Don't warn about the parm names in function declarator
		   within a function declarator.
		   It would be nice to avoid warning in any function
		   declarator in a declaration, as opposed to a definition,
		   but there is no way to tell it's not a definition.  */
		;
	      else if (oldlocal != 0 && TREE_CODE (oldlocal) == PARM_DECL)
		warnstring = "declaration of `%s' shadows a parameter";
	      else if (oldlocal != 0)
		warnstring = "declaration of `%s' shadows previous local";
	      else if (IDENTIFIER_GLOBAL_VALUE (name) != 0
		       && IDENTIFIER_GLOBAL_VALUE (name) != error_mark_node)
		warnstring = "declaration of `%s' shadows global declaration";

	      if (warnstring)
		warning (warnstring, IDENTIFIER_POINTER (name));
	    }
#endif

	  /* If storing a local value, there may already be one (inherited).
	     If so, record it for restoration when this binding level ends.  */
	  if (oldlocal != 0)
	    b->shadowed = tree_cons (name, oldlocal, b->shadowed);
	}
    }

  /* Put decls on list in reverse order.
     We will reverse them later if necessary.  */
  TREE_CHAIN (x) = b->names;
  b->names = x;

  return x;
}

void
pushdecl_force_head (x)
     tree x;
{
  current_binding_level->names = x;
}

/* Like pushdecl, only it places X in GLOBAL_BINDING_LEVEL, if appropriate.  */

tree
pushdecl_top_level (x)
     tree x;
{
  register tree t;
  register struct binding_level *b = current_binding_level;

  current_binding_level = global_binding_level;
  t = pushdecl (x);
  current_binding_level = b;
  return t;
}

/* Nonzero if we are currently in the global binding level.  */

int
global_bindings_p ()
{
  return current_binding_level == global_binding_level;
}

/* Return the list of declarations of the current level.
   Note that this list is in reverse order unless/until
   you nreverse it; and when you do nreverse it, you must
   store the result back using `storedecls' or you will lose.  */

tree
getdecls ()
{
  return current_binding_level->names;
}

/* Create a new `struct binding_level'.  */

static struct binding_level *
make_binding_level ()
{
  /* NOSTRICT */
  return (struct binding_level *) xmalloc (sizeof (struct binding_level));
}

void
pushlevel (unused)
  int unused ATTRIBUTE_UNUSED;
{
  register struct binding_level *newlevel = NULL_BINDING_LEVEL;

#if 0
  /* If this is the top level of a function,
     just make sure that NAMED_LABELS is 0.  */

  if (current_binding_level == global_binding_level)
    named_labels = 0;
#endif

  /* Reuse or create a struct for this binding level.  */

  if (free_binding_level)
    {
      newlevel = free_binding_level;
      free_binding_level = free_binding_level->level_chain;
    }
  else
    {
      newlevel = make_binding_level ();
    }

  /* Add this level to the front of the chain (stack) of levels that
     are active.  */

  *newlevel = clear_binding_level;
  newlevel->level_chain = current_binding_level;
  current_binding_level = newlevel;
#if defined(DEBUG_JAVA_BINDING_LEVELS)
  newlevel->binding_depth = binding_depth;
  indent ();
  fprintf (stderr, "push %s level 0x%08x pc %d\n",
	   (is_class_level) ? "class" : "block", newlevel, current_pc);
  is_class_level = 0;
  binding_depth++;
#endif /* defined(DEBUG_JAVA_BINDING_LEVELS) */
}

/* Exit a binding level.
   Pop the level off, and restore the state of the identifier-decl mappings
   that were in effect when this level was entered.

   If KEEP is nonzero, this level had explicit declarations, so
   and create a "block" (a BLOCK node) for the level
   to record its declarations and subblocks for symbol table output.

   If FUNCTIONBODY is nonzero, this level is the body of a function,
   so create a block as if KEEP were set and also clear out all
   label names.

   If REVERSE is nonzero, reverse the order of decls before putting
   them into the BLOCK.  */

tree
poplevel (keep, reverse, functionbody)
     int keep;
     int reverse;
     int functionbody;
{
  register tree link;
  /* The chain of decls was accumulated in reverse order.
     Put it into forward order, just for cleanliness.  */
  tree decls;
  tree subblocks = current_binding_level->blocks;
  tree block = 0;
  tree decl;
  int block_previously_created;

#if defined(DEBUG_JAVA_BINDING_LEVELS)
  binding_depth--;
  indent ();
  if (current_binding_level->end_pc != LARGEST_PC)
    fprintf (stderr, "pop  %s level 0x%08x pc %d (end pc %d)\n",
	     (is_class_level) ? "class" : "block", current_binding_level, current_pc,
	     current_binding_level->end_pc);
  else
    fprintf (stderr, "pop  %s level 0x%08x pc %d\n",
	     (is_class_level) ? "class" : "block", current_binding_level, current_pc);
#if 0
  if (is_class_level != (current_binding_level == class_binding_level))
    {
      indent ();
      fprintf (stderr, "XXX is_class_level != (current_binding_level == class_binding_level)\n");
    }
  is_class_level = 0;
#endif
#endif /* defined(DEBUG_JAVA_BINDING_LEVELS) */

  /* Get the decls in the order they were written.
     Usually current_binding_level->names is in reverse order.
     But parameter decls were previously put in forward order.  */

  if (reverse)
    current_binding_level->names
      = decls = nreverse (current_binding_level->names);
  else
    decls = current_binding_level->names;

  /* Output any nested inline functions within this block
     if they weren't already output.  */

  for (decl = decls; decl; decl = TREE_CHAIN (decl))
    if (TREE_CODE (decl) == FUNCTION_DECL
	&& ! TREE_ASM_WRITTEN (decl)
	&& DECL_INITIAL (decl) != 0
	&& TREE_ADDRESSABLE (decl))
      {
	/* If this decl was copied from a file-scope decl
	   on account of a block-scope extern decl,
	   propagate TREE_ADDRESSABLE to the file-scope decl.

	   DECL_ABSTRACT_ORIGIN can be set to itself if warn_return_type is
	   true, since then the decl goes through save_for_inline_copying.  */
	if (DECL_ABSTRACT_ORIGIN (decl) != 0
	    && DECL_ABSTRACT_ORIGIN (decl) != decl)
	  TREE_ADDRESSABLE (DECL_ABSTRACT_ORIGIN (decl)) = 1;
	else
	  {
	    push_function_context ();
	    output_inline_function (decl);
	    pop_function_context ();
	  }
      }

  /* If there were any declarations in that level,
     or if this level is a function body,
     create a BLOCK to record them for the life of this function.  */

  block = 0;
  block_previously_created = (current_binding_level->this_block != 0);
  if (block_previously_created)
    block = current_binding_level->this_block;
  else if (keep || functionbody)
    block = make_node (BLOCK);
  if (block != 0)
    {
      BLOCK_VARS (block) = decls;
      BLOCK_SUBBLOCKS (block) = subblocks;
    }

  /* In each subblock, record that this is its superior.  */

  for (link = subblocks; link; link = TREE_CHAIN (link))
    BLOCK_SUPERCONTEXT (link) = block;

  /* Clear out the meanings of the local variables of this level.  */

  for (link = decls; link; link = TREE_CHAIN (link))
    {
      tree name = DECL_NAME (link);
      if (name != 0 && IDENTIFIER_LOCAL_VALUE (name) == link)
	{
	  /* If the ident. was used or addressed via a local extern decl,
	     don't forget that fact.  */
	  if (DECL_EXTERNAL (link))
	    {
	      if (TREE_USED (link))
		TREE_USED (name) = 1;
	      if (TREE_ADDRESSABLE (link))
		TREE_ADDRESSABLE (DECL_ASSEMBLER_NAME (link)) = 1;
	    }
	  IDENTIFIER_LOCAL_VALUE (name) = 0;
	}
    }

  /* Restore all name-meanings of the outer levels
     that were shadowed by this level.  */

  for (link = current_binding_level->shadowed; link; link = TREE_CHAIN (link))
    IDENTIFIER_LOCAL_VALUE (TREE_PURPOSE (link)) = TREE_VALUE (link);

  /* If the level being exited is the top level of a function,
     check over all the labels, and clear out the current
     (function local) meanings of their names.  */

  if (functionbody)
    {
      /* If this is the top level block of a function,
	 the vars are the function's parameters.
	 Don't leave them in the BLOCK because they are
	 found in the FUNCTION_DECL instead.  */

      BLOCK_VARS (block) = 0;

      /* Clear out the definitions of all label names,
	 since their scopes end here,
	 and add them to BLOCK_VARS.  */

#if 0
      for (link = named_labels; link; link = TREE_CHAIN (link))
	{
	  register tree label = TREE_VALUE (link);

	  if (DECL_INITIAL (label) == 0)
	    {
	      error_with_decl (label, "label `%s' used but not defined");
	      /* Avoid crashing later.  */
	      define_label (input_filename, lineno,
			    DECL_NAME (label));
	    }
	  else if (warn_unused[UNUSED_LABEL] && !TREE_USED (label))
	    warning_with_decl (label, "label `%s' defined but not used");
	  IDENTIFIER_LABEL_VALUE (DECL_NAME (label)) = 0;

	  /* Put the labels into the "variables" of the
	     top-level block, so debugger can see them.  */
	  TREE_CHAIN (label) = BLOCK_VARS (block);
	  BLOCK_VARS (block) = label;
	}
#endif
    }

  /* Pop the current level, and free the structure for reuse.  */

  {
    register struct binding_level *level = current_binding_level;
    current_binding_level = current_binding_level->level_chain;

    level->level_chain = free_binding_level;
    free_binding_level = level;
  }

  /* Dispose of the block that we just made inside some higher level.  */
  if (functionbody)
    DECL_INITIAL (current_function_decl) = block;
  else if (block)
    {
      if (!block_previously_created)
        current_binding_level->blocks
          = chainon (current_binding_level->blocks, block);
    }
  /* If we did not make a block for the level just exited,
     any blocks made for inner levels
     (since they cannot be recorded as subblocks in that level)
     must be carried forward so they will later become subblocks
     of something else.  */
  else if (subblocks)
    current_binding_level->blocks
      = chainon (current_binding_level->blocks, subblocks);

  /* Set the TYPE_CONTEXTs for all of the tagged types belonging to this
     binding contour so that they point to the appropriate construct, i.e.
     either to the current FUNCTION_DECL node, or else to the BLOCK node
     we just constructed.

     Note that for tagged types whose scope is just the formal parameter
     list for some function type specification, we can't properly set
     their TYPE_CONTEXTs here, because we don't have a pointer to the
     appropriate FUNCTION_TYPE node readily available to us.  For those
     cases, the TYPE_CONTEXTs of the relevant tagged type nodes get set
     in `grokdeclarator' as soon as we have created the FUNCTION_TYPE
     node which will represent the "scope" for these "parameter list local"
     tagged types.
  */

  if (block)
    TREE_USED (block) = 1;
  return block;
}

void
maybe_pushlevels (pc)
     int pc;
{
#if defined(DEBUG_JAVA_BINDING_LEVELS)
  current_pc = pc;
#endif

  while (pending_local_decls != NULL_TREE &&
	 DECL_LOCAL_START_PC (pending_local_decls) <= pc)
    {
      tree *ptr = &pending_local_decls;
      tree decl = *ptr;
      int end_pc = DECL_LOCAL_END_PC (decl);

      while (*ptr != NULL_TREE
	     && DECL_LOCAL_START_PC (*ptr) <= pc
	     && DECL_LOCAL_END_PC (*ptr) == end_pc)
	ptr = &TREE_CHAIN (*ptr);
      pending_local_decls = *ptr;
      *ptr = NULL_TREE;

      /* Force non-nested range to be nested in current range. */
      if (end_pc > current_binding_level->end_pc)
	end_pc = current_binding_level->end_pc;

      maybe_start_try (pc, end_pc);
      
      pushlevel (1);
      expand_start_bindings (0);

      current_binding_level->end_pc = end_pc;
      current_binding_level->start_pc = pc;      
      current_binding_level->names = decl;
      for ( ; decl != NULL_TREE;  decl = TREE_CHAIN (decl))
	{
	  push_jvm_slot (DECL_LOCAL_SLOT_NUMBER (decl), decl);
	}
    }      

  maybe_start_try (pc, 0);
}

void
maybe_poplevels (pc)
     int pc;
{
#if defined(DEBUG_JAVA_BINDING_LEVELS)
  current_pc = pc;
#endif

  while (current_binding_level->end_pc <= pc)
    {
      expand_end_bindings (getdecls (), 1, 0);
      maybe_end_try (current_binding_level->start_pc, pc);
      poplevel (1, 0, 0);
    }
  maybe_end_try (0, pc);
}

/* Terminate any binding which began during the range beginning at
   start_pc.  This tidies up improperly nested local variable ranges
   and exception handlers; a variable declared within an exception
   range is forcibly terminated when that exception ends. */

void
force_poplevels (start_pc)
     int start_pc;
{
  while (current_binding_level->start_pc > start_pc)
    {
      if (pedantic && current_binding_level->start_pc > start_pc)
	warning_with_decl (current_function_decl, 
			   "In %s: overlapped variable and exception ranges at %d",
			   current_binding_level->start_pc);
      expand_end_bindings (getdecls (), 1, 0);
      poplevel (1, 0, 0);
    }
}

/* Insert BLOCK at the end of the list of subblocks of the
   current binding level.  This is used when a BIND_EXPR is expanded,
   to handle the BLOCK node inside the BIND_EXPR.  */

void
insert_block (block)
     tree block;
{
  TREE_USED (block) = 1;
  current_binding_level->blocks
    = chainon (current_binding_level->blocks, block);
}

/* Set the BLOCK node for the innermost scope
   (the one we are currently in).  */

void
set_block (block)
     register tree block;
{
  current_binding_level->this_block = block;
  current_binding_level->names = chainon (current_binding_level->names,
					  BLOCK_VARS (block));
  current_binding_level->blocks = chainon (current_binding_level->blocks,
					   BLOCK_SUBBLOCKS (block));
}

/* integrate_decl_tree calls this function. */

void
java_dup_lang_specific_decl (node)
     tree node;
{
  int lang_decl_size;
  struct lang_decl *x;

  if (!DECL_LANG_SPECIFIC (node))
    return;

  if (TREE_CODE (node) == VAR_DECL)
    lang_decl_size = sizeof (struct lang_decl_var);
  else
    lang_decl_size = sizeof (struct lang_decl);
  x = (struct lang_decl *) ggc_alloc (lang_decl_size);
  memcpy (x, DECL_LANG_SPECIFIC (node), lang_decl_size);
  DECL_LANG_SPECIFIC (node) = x;
}

void
give_name_to_locals (jcf)
     JCF *jcf;
{
  int i, n = DECL_LOCALVARIABLES_OFFSET (current_function_decl);
  int code_offset = DECL_CODE_OFFSET (current_function_decl);
  tree parm;
  pending_local_decls = NULL_TREE;
  if (n == 0)
    return;
  JCF_SEEK (jcf, n);
  n = JCF_readu2 (jcf);
  for (i = 0; i < n; i++)
    {
      int start_pc = JCF_readu2 (jcf);
      int length = JCF_readu2 (jcf);
      int name_index = JCF_readu2 (jcf);
      int signature_index = JCF_readu2 (jcf);
      int slot = JCF_readu2 (jcf);
      tree name = get_name_constant (jcf, name_index);
      tree type = parse_signature (jcf, signature_index);
      if (slot < DECL_ARG_SLOT_COUNT (current_function_decl)
	  && start_pc == 0
	  && length == DECL_CODE_LENGTH (current_function_decl))
	{
	  tree decl = TREE_VEC_ELT (decl_map, slot);
	  DECL_NAME (decl) = name;
	  SET_DECL_ASSEMBLER_NAME (decl, name);
	  if (TREE_CODE (decl) != PARM_DECL || TREE_TYPE (decl) != type)
	    warning ("bad type in parameter debug info");
	}
      else
	{
	  tree *ptr;
	  int end_pc = start_pc + length;
	  tree decl = build_decl (VAR_DECL, name, type);
	  if (end_pc > DECL_CODE_LENGTH (current_function_decl))
	    {
	      warning_with_decl (decl,
			 "bad PC range for debug info for local `%s'");
	      end_pc = DECL_CODE_LENGTH (current_function_decl);
	    }

	  /* Adjust start_pc if necessary so that the local's first
	     store operation will use the relevant DECL as a
	     destination. Fore more information, read the leading
	     comments for expr.c:maybe_adjust_start_pc. */
	  start_pc = maybe_adjust_start_pc (jcf, code_offset, start_pc, slot);

	  MAYBE_CREATE_VAR_LANG_DECL_SPECIFIC (decl);
	  DECL_LOCAL_SLOT_NUMBER (decl) = slot;
	  DECL_LOCAL_START_PC (decl) = start_pc;
#if 0
	  /* FIXME: The range used internally for exceptions and local
	     variable ranges, is a half-open interval: 
	     start_pc <= pc < end_pc.  However, the range used in the
	     Java VM spec is inclusive at both ends: 
	     start_pc <= pc <= end_pc. */
	  end_pc++;
#endif
	  DECL_LOCAL_END_PC (decl) = end_pc;

	  /* Now insert the new decl in the proper place in
	     pending_local_decls.  We are essentially doing an insertion sort,
	     which works fine, since the list input will normally already
	     be sorted. */
	  ptr = &pending_local_decls;
	  while (*ptr != NULL_TREE
		 && (DECL_LOCAL_START_PC (*ptr) > start_pc
		     || (DECL_LOCAL_START_PC (*ptr) == start_pc
			 && DECL_LOCAL_END_PC (*ptr) < end_pc)))
	    ptr = &TREE_CHAIN (*ptr);
	  TREE_CHAIN (decl) = *ptr;
	  *ptr = decl;
	}
    }

  pending_local_decls = nreverse (pending_local_decls);

  /* Fill in default names for the parameters. */ 
  for (parm = DECL_ARGUMENTS (current_function_decl), i = 0;
       parm != NULL_TREE;  parm = TREE_CHAIN (parm), i++)
    {
      if (DECL_NAME (parm) == NULL_TREE)
	{
	  int arg_i = METHOD_STATIC (current_function_decl) ? i+1 : i;
	  if (arg_i == 0)
	    DECL_NAME (parm) = get_identifier ("this");
	  else
	    {
	      char buffer[12];
	      sprintf (buffer, "ARG_%d", arg_i);
	      DECL_NAME (parm) = get_identifier (buffer);
	    }
	  SET_DECL_ASSEMBLER_NAME (parm, DECL_NAME (parm));
	}
    }
}

tree
build_result_decl (fndecl)
  tree fndecl;
{
  tree restype = TREE_TYPE (TREE_TYPE (fndecl));
  /* To be compatible with C_PROMOTING_INTEGER_TYPE_P in cc1/cc1plus. */
  if (INTEGRAL_TYPE_P (restype)
      && TYPE_PRECISION (restype) < TYPE_PRECISION (integer_type_node))
    restype = integer_type_node;
  return (DECL_RESULT (fndecl) = build_decl (RESULT_DECL, NULL_TREE, restype));
}

void
complete_start_java_method (fndecl)
  tree fndecl;
{
  if (! flag_emit_class_files)
    {
      /* Initialize the RTL code for the function.  */
      init_function_start (fndecl, input_filename, lineno);

      /* Set up parameters and prepare for return, for the function.  */
      expand_function_start (fndecl, 0);
    }

#if 0
      /* If this fcn was already referenced via a block-scope `extern' decl (or
         an implicit decl), propagate certain information about the usage. */
      if (TREE_ADDRESSABLE (DECL_ASSEMBLER_NAME (current_function_decl)))
        TREE_ADDRESSABLE (current_function_decl) = 1;

#endif

  if (METHOD_STATIC (fndecl) && ! METHOD_PRIVATE (fndecl)
      && ! flag_emit_class_files
      && ! DECL_CLINIT_P (fndecl)
      && ! CLASS_INTERFACE (TYPE_NAME (current_class)))
    {
      tree clas = DECL_CONTEXT (fndecl);
      tree init = build (CALL_EXPR, void_type_node,
			 build_address_of (soft_initclass_node),
			 build_tree_list (NULL_TREE, build_class_ref (clas)),
			 NULL_TREE);
      TREE_SIDE_EFFECTS (init) = 1;
      expand_expr_stmt (init);
    }

  /* Push local variables. Function compiled from source code are
     using a different local variables management, and for them,
     pushlevel shouldn't be called from here.  */
  if (!CLASS_FROM_SOURCE_P (DECL_CONTEXT (fndecl)))
    {
      pushlevel (2);
      if (! flag_emit_class_files)
	expand_start_bindings (1);
    }

  if (METHOD_SYNCHRONIZED (fndecl) && ! flag_emit_class_files)
    {
      /* Wrap function body with a monitorenter plus monitorexit cleanup. */
      tree enter, exit, lock;
      if (METHOD_STATIC (fndecl))
	lock = build_class_ref (DECL_CONTEXT (fndecl));
      else
	lock = DECL_ARGUMENTS (fndecl);
      BUILD_MONITOR_ENTER (enter, lock);
      BUILD_MONITOR_EXIT (exit, lock);
      if (!CLASS_FROM_SOURCE_P (DECL_CONTEXT (fndecl)))
	{
	  expand_expr_stmt (enter);
	  expand_decl_cleanup (NULL_TREE, exit);
	}
      else
	{
	  tree function_body = DECL_FUNCTION_BODY (fndecl);
	  tree body = BLOCK_EXPR_BODY (function_body);
	  lock = build (COMPOUND_EXPR, void_type_node,
			enter,
			build (TRY_FINALLY_EXPR, void_type_node, body, exit));
	  TREE_SIDE_EFFECTS (lock) = 1;
	  BLOCK_EXPR_BODY (function_body) = lock;
	}
    }
}

void
start_java_method (fndecl)
     tree fndecl;
{
  tree tem, *ptr;
  int i;

  current_function_decl = fndecl;
  announce_function (fndecl);

  i = DECL_MAX_LOCALS(fndecl) + DECL_MAX_STACK(fndecl);
  decl_map = make_tree_vec (i);
  type_map = (tree *) xrealloc (type_map, i * sizeof (tree));

#if defined(DEBUG_JAVA_BINDING_LEVELS)
  fprintf (stderr, "%s:\n", lang_printable_name (fndecl, 2));
  current_pc = 0;
#endif /* defined(DEBUG_JAVA_BINDING_LEVELS) */
  pushlevel (1);  /* Push parameters. */

  ptr = &DECL_ARGUMENTS (fndecl);
  for (tem = TYPE_ARG_TYPES (TREE_TYPE (fndecl)), i = 0;
       tem != end_params_node; tem = TREE_CHAIN (tem), i++)
    {
      tree parm_name = NULL_TREE, parm_decl;
      tree parm_type = TREE_VALUE (tem);
      if (i >= DECL_MAX_LOCALS (fndecl))
	abort ();

      parm_decl = build_decl (PARM_DECL, parm_name, parm_type);
      DECL_CONTEXT (parm_decl) = fndecl;
      if (PROMOTE_PROTOTYPES
	  && TYPE_PRECISION (parm_type) < TYPE_PRECISION (integer_type_node)
	  && INTEGRAL_TYPE_P (parm_type))
	parm_type = integer_type_node;
      DECL_ARG_TYPE (parm_decl) = parm_type;

      *ptr = parm_decl;
      ptr = &TREE_CHAIN (parm_decl);

      /* Add parm_decl to the decl_map. */
      push_jvm_slot (i, parm_decl);

      type_map[i] = TREE_TYPE (parm_decl);
      if (TYPE_IS_WIDE (TREE_TYPE (parm_decl)))
	{
	  i++;
	  type_map[i] = void_type_node;
	}
    }
  *ptr = NULL_TREE;
  DECL_ARG_SLOT_COUNT (current_function_decl) = i;

  while (i < DECL_MAX_LOCALS(fndecl))
    type_map[i++] = NULL_TREE;

  build_result_decl (fndecl);
  complete_start_java_method (fndecl);
}

void
end_java_method ()
{
  tree fndecl = current_function_decl;

  expand_end_bindings (getdecls (), 1, 0);
  /* pop out of function */
  poplevel (1, 1, 0);

  /* pop out of its parameters */
  poplevel (1, 0, 1);

  BLOCK_SUPERCONTEXT (DECL_INITIAL (fndecl)) = fndecl;

  /* Generate rtl for function exit.  */
  expand_function_end (input_filename, lineno, 0);

  /* Run the optimizers and output assembler code for this function. */
  rest_of_compilation (fndecl);

  current_function_decl = NULL_TREE;
}

/* Mark language-specific parts of T for garbage-collection.  */

void
java_mark_tree (t)
     tree t;
{
  if (TREE_CODE (t) == IDENTIFIER_NODE)
    {
      struct lang_identifier *li = (struct lang_identifier *) t;
      ggc_mark_tree (li->global_value);
      ggc_mark_tree (li->local_value);
      ggc_mark_tree (li->utf8_ref);
    }
  else if (TREE_CODE (t) == VAR_DECL
	   || TREE_CODE (t) == PARM_DECL
	   || TREE_CODE (t) == FIELD_DECL)
    {
      struct lang_decl_var *ldv = 
	((struct lang_decl_var *) DECL_LANG_SPECIFIC (t));
      if (ldv)
	{
	  ggc_mark (ldv);
	  ggc_mark_tree (ldv->slot_chain);
	  ggc_mark_tree (ldv->am);
	  ggc_mark_tree (ldv->wfl);
	}
    }
  else if (TREE_CODE (t) == FUNCTION_DECL)
    {
      struct lang_decl *ld = DECL_LANG_SPECIFIC (t);
      
      if (ld)
	{
	  ggc_mark (ld);
	  ggc_mark_tree (ld->wfl);
	  ggc_mark_tree (ld->throws_list);
	  ggc_mark_tree (ld->function_decl_body);
	  ggc_mark_tree (ld->called_constructor);
	  ggc_mark_tree (ld->inner_access);
	  ggc_mark_tree_hash_table (&ld->init_test_table);
	  ggc_mark_tree_hash_table (&ld->ict);
	  ggc_mark_tree (ld->smic);
	}
    }
  else if (TYPE_P (t))
    {
      struct lang_type *lt = TYPE_LANG_SPECIFIC (t);
      
      if (lt)
	{
	  ggc_mark (lt);
	  ggc_mark_tree (lt->signature);
	  ggc_mark_tree (lt->cpool_data_ref);
	  ggc_mark_tree (lt->finit_stmt_list);
	  ggc_mark_tree (lt->clinit_stmt_list);
	  ggc_mark_tree (lt->ii_block);
	  ggc_mark_tree (lt->dot_class);
	  ggc_mark_tree (lt->package_list);
	}
    }
}
