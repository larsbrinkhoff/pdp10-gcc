------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             E X P _ A G G R                              --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                                                                          --
--          Copyright (C) 1992-2002 Free Software Foundation, Inc.          --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 2,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT;  see file COPYING.  If not, write --
-- to  the Free Software Foundation,  59 Temple Place - Suite 330,  Boston, --
-- MA 02111-1307, USA.                                                      --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- It is now maintained by Ada Core Technologies Inc (http://www.gnat.com). --
--                                                                          --
------------------------------------------------------------------------------

with Atree;    use Atree;
with Checks;   use Checks;
with Debug;    use Debug;
with Einfo;    use Einfo;
with Elists;   use Elists;
with Expander; use Expander;
with Exp_Util; use Exp_Util;
with Exp_Ch3;  use Exp_Ch3;
with Exp_Ch7;  use Exp_Ch7;
with Freeze;   use Freeze;
with Hostparm; use Hostparm;
with Itypes;   use Itypes;
with Lib;      use Lib;
with Nmake;    use Nmake;
with Nlists;   use Nlists;
with Restrict; use Restrict;
with Rtsfind;  use Rtsfind;
with Ttypes;   use Ttypes;
with Sem;      use Sem;
with Sem_Ch3;  use Sem_Ch3;
with Sem_Eval; use Sem_Eval;
with Sem_Res;  use Sem_Res;
with Sem_Util; use Sem_Util;
with Sinfo;    use Sinfo;
with Snames;   use Snames;
with Stand;    use Stand;
with Tbuild;   use Tbuild;
with Uintp;    use Uintp;

package body Exp_Aggr is

   type Case_Bounds is record
     Choice_Lo   : Node_Id;
     Choice_Hi   : Node_Id;
     Choice_Node : Node_Id;
   end record;

   type Case_Table_Type is array (Nat range <>) of Case_Bounds;
   --  Table type used by Check_Case_Choices procedure

   procedure Sort_Case_Table (Case_Table : in out Case_Table_Type);
   --  Sort the Case Table using the Lower Bound of each Choice as the key.
   --  A simple insertion sort is used since the number of choices in a case
   --  statement of variant part will usually be small and probably in near
   --  sorted order.

   ------------------------------------------------------
   -- Local subprograms for Record Aggregate Expansion --
   ------------------------------------------------------

   procedure Expand_Record_Aggregate
     (N           : Node_Id;
      Orig_Tag    : Node_Id := Empty;
      Parent_Expr : Node_Id := Empty);
   --  This is the top level procedure for record aggregate expansion.
   --  Expansion for record aggregates needs expand aggregates for tagged
   --  record types. Specifically Expand_Record_Aggregate adds the Tag
   --  field in front of the Component_Association list that was created
   --  during resolution by Resolve_Record_Aggregate.
   --
   --    N is the record aggregate node.
   --    Orig_Tag is the value of the Tag that has to be provided for this
   --      specific aggregate. It carries the tag corresponding to the type
   --      of the outermost aggregate during the recursive expansion
   --    Parent_Expr is the ancestor part of the original extension
   --      aggregate

   procedure Convert_To_Assignments (N : Node_Id; Typ : Entity_Id);
   --  N is an N_Aggregate of a N_Extension_Aggregate. Typ is the type of
   --  the aggregate. Transform the given aggregate into a sequence of
   --  assignments component per component.

   function Build_Record_Aggr_Code
     (N      : Node_Id;
      Typ    : Entity_Id;
      Target : Node_Id;
      Flist  : Node_Id   := Empty;
      Obj    : Entity_Id := Empty)
      return   List_Id;
   --  N is an N_Aggregate or a N_Extension_Aggregate. Typ is the type
   --  of the aggregate. Target is an expression containing the
   --  location on which the component by component assignments will
   --  take place. Returns the list of assignments plus all other
   --  adjustments needed for tagged and controlled types. Flist is an
   --  expression representing the finalization list on which to
   --  attach the controlled components if any. Obj is present in the
   --  object declaration and dynamic allocation cases, it contains
   --  an entity that allows to know if the value being created needs to be
   --  attached to the final list in case of pragma finalize_Storage_Only.

   procedure Initialize_Discriminants (N : Node_Id; Typ : Entity_Id);
   --  If the type of the aggregate is a type extension with renamed discrimi-
   --  nants, we must initialize the hidden discriminants of the parent.
   --  Otherwise, the target object must not be initialized. The discriminants
   --  are initialized by calling the initialization procedure for the type.
   --  This is incorrect if the initialization of other components has any
   --  side effects. We restrict this call to the case where the parent type
   --  has a variant part, because this is the only case where the hidden
   --  discriminants are accessed, namely when calling discriminant checking
   --  functions of the parent type, and when applying a stream attribute to
   --  an object of the derived type.

   -----------------------------------------------------
   -- Local Subprograms for Array Aggregate Expansion --
   -----------------------------------------------------

   procedure Convert_To_Positional
     (N                    : Node_Id;
      Max_Others_Replicate : Nat := 5;
      Handle_Bit_Packed    : Boolean := False);
   --  If possible, convert named notation to positional notation. This
   --  conversion is possible only in some static cases. If the conversion
   --  is possible, then N is rewritten with the analyzed converted
   --  aggregate. The parameter Max_Others_Replicate controls the maximum
   --  number of values corresponding to an others choice that will be
   --  converted to positional notation (the default of 5 is the normal
   --  limit, and reflects the fact that normally the loop is better than
   --  a lot of separate assignments). Note that this limit gets overridden
   --  in any case if either of the restrictions No_Elaboration_Code or
   --  No_Implicit_Loops is set. The parameter Handle_Bit_Packed is usually
   --  set False (since we do not expect the back end to handle bit packed
   --  arrays, so the normal case of conversion is pointless), but in the
   --  special case of a call from Packed_Array_Aggregate_Handled, we set
   --  this parameter to True, since these are cases we handle in there.

   procedure Expand_Array_Aggregate (N : Node_Id);
   --  This is the top-level routine to perform array aggregate expansion.
   --  N is the N_Aggregate node to be expanded.

   function Backend_Processing_Possible (N : Node_Id) return Boolean;
   --  This function checks if array aggregate N can be processed directly
   --  by Gigi. If this is the case True is returned.

   function Build_Array_Aggr_Code
     (N           : Node_Id;
      Index       : Node_Id;
      Into        : Node_Id;
      Scalar_Comp : Boolean;
      Indices     : List_Id := No_List;
      Flist       : Node_Id := Empty)
      return        List_Id;
   --  This recursive routine returns a list of statements containing the
   --  loops and assignments that are needed for the expansion of the array
   --  aggregate N.
   --
   --    N is the (sub-)aggregate node to be expanded into code.
   --
   --    Index is the index node corresponding to the array sub-aggregate N.
   --
   --    Into is the target expression into which we are copying the aggregate.
   --
   --    Scalar_Comp is True if the component type of the aggregate is scalar.
   --
   --    Indices is the current list of expressions used to index the
   --    object we are writing into.
   --
   --    Flist is an expression representing the finalization list on which
   --    to attach the controlled components if any.

   function Number_Of_Choices (N : Node_Id) return Nat;
   --  Returns the number of discrete choices (not including the others choice
   --  if present) contained in (sub-)aggregate N.

   function Late_Expansion
     (N      : Node_Id;
      Typ    : Entity_Id;
      Target : Node_Id;
      Flist  : Node_Id := Empty;
      Obj    : Entity_Id := Empty)
      return List_Id;
   --  N is a nested (record or array) aggregate that has been marked
   --  with 'Delay_Expansion'. Typ is the expected type of the
   --  aggregate and Target is a (duplicable) expression that will
   --  hold the result of the aggregate expansion. Flist is the
   --  finalization list to be used to attach controlled
   --  components. 'Obj' when non empty, carries the original object
   --  being initialized in order to know if it needs to be attached
   --  to the previous parameter which may not be the case when
   --  Finalize_Storage_Only is set.  Basically this procedure is used
   --  to implement top-down expansions of nested aggregates. This is
   --  necessary for avoiding temporaries at each level as well as for
   --  propagating the right internal finalization list.

   function Make_OK_Assignment_Statement
     (Sloc       : Source_Ptr;
      Name       : Node_Id;
      Expression : Node_Id)
      return       Node_Id;
   --  This is like Make_Assignment_Statement, except that Assignment_OK
   --  is set in the left operand. All assignments built by this unit
   --  use this routine. This is needed to deal with assignments to
   --  initialized constants that are done in place.

   function Packed_Array_Aggregate_Handled (N : Node_Id) return Boolean;
   --  Given an array aggregate, this function handles the case of a packed
   --  array aggregate with all constant values, where the aggregate can be
   --  evaluated at compile time. If this is possible, then N is rewritten
   --  to be its proper compile time value with all the components properly
   --  assembled. The expression is analyzed and resolved and True is
   --  returned. If this transformation is not possible, N is unchanged
   --  and False is returned

   function Safe_Slice_Assignment (N : Node_Id) return Boolean;
   --  If a slice assignment has an aggregate with a single others_choice,
   --  the assignment can be done in place even if bounds are not static,
   --  by converting it into a loop over the discrete range of the slice.

   ---------------------------------
   -- Backend_Processing_Possible --
   ---------------------------------

   --  Backend processing by Gigi/gcc is possible only if all the following
   --  conditions are met:

   --    1. N is fully positional

   --    2. N is not a bit-packed array aggregate;

   --    3. The size of N's array type must be known at compile time. Note
   --       that this implies that the component size is also known

   --    4. The array type of N does not follow the Fortran layout convention
   --       or if it does it must be 1 dimensional.

   --    5. The array component type is tagged, which may necessitate
   --       reassignment of proper tags.

   function Backend_Processing_Possible (N : Node_Id) return Boolean is
      Typ : constant Entity_Id := Etype (N);
      --  Typ is the correct constrained array subtype of the aggregate.

      function Static_Check (N : Node_Id; Index : Node_Id) return Boolean;
      --  Recursively checks that N is fully positional, returns true if so.

      ------------------
      -- Static_Check --
      ------------------

      function Static_Check (N : Node_Id; Index : Node_Id) return Boolean is
         Expr : Node_Id;

      begin
         --  Check for component associations

         if Present (Component_Associations (N)) then
            return False;
         end if;

         --  Recurse to check subaggregates, which may appear in qualified
         --  expressions. If delayed, the front-end will have to expand.

         Expr := First (Expressions (N));

         while Present (Expr) loop

            if Is_Delayed_Aggregate (Expr) then
               return False;
            end if;

            if Present (Next_Index (Index))
               and then not Static_Check (Expr, Next_Index (Index))
            then
               return False;
            end if;

            Next (Expr);
         end loop;

         return True;
      end Static_Check;

   --  Start of processing for Backend_Processing_Possible

   begin
      --  Checks 2 (array must not be bit packed)

      if Is_Bit_Packed_Array (Typ) then
         return False;
      end if;

      --  Checks 4  (array must not be multi-dimensional Fortran case)

      if Convention (Typ) = Convention_Fortran
        and then Number_Dimensions (Typ) > 1
      then
         return False;
      end if;

      --  Checks 3 (size of array must be known at compile time)

      if not Size_Known_At_Compile_Time (Typ) then
         return False;
      end if;

      --  Checks 1 (aggregate must be fully positional)

      if not Static_Check (N, First_Index (Typ)) then
         return False;
      end if;

      --  Checks 5 (if the component type is tagged, then we may need
      --    to do tag adjustments; perhaps this should be refined to
      --    check for any component associations that actually
      --    need tag adjustment, along the lines of the test that's
      --    done in Has_Delayed_Nested_Aggregate_Or_Tagged_Comps
      --    for record aggregates with tagged components, but not
      --    clear whether it's worthwhile ???; in the case of the
      --    JVM, object tags are handled implicitly)

      if Is_Tagged_Type (Component_Type (Typ)) and then not Java_VM then
         return False;
      end if;

      --  Backend processing is possible

      Set_Compile_Time_Known_Aggregate (N, True);
      Set_Size_Known_At_Compile_Time (Etype (N), True);
      return True;
   end Backend_Processing_Possible;

   ---------------------------
   -- Build_Array_Aggr_Code --
   ---------------------------

   --  The code that we generate from a one dimensional aggregate is

   --  1. If the sub-aggregate contains discrete choices we

   --     (a) Sort the discrete choices

   --     (b) Otherwise for each discrete choice that specifies a range we
   --         emit a loop. If a range specifies a maximum of three values, or
   --         we are dealing with an expression we emit a sequence of
   --         assignments instead of a loop.

   --     (c) Generate the remaining loops to cover the others choice if any.

   --  2. If the aggregate contains positional elements we

   --     (a) translate the positional elements in a series of assignments.

   --     (b) Generate a final loop to cover the others choice if any.
   --         Note that this final loop has to be a while loop since the case

   --             L : Integer := Integer'Last;
   --             H : Integer := Integer'Last;
   --             A : array (L .. H) := (1, others =>0);

   --         cannot be handled by a for loop. Thus for the following

   --             array (L .. H) := (.. positional elements.., others =>E);

   --         we always generate something like:

   --             J : Index_Type := Index_Of_Last_Positional_Element;
   --             while J < H loop
   --                J := Index_Base'Succ (J)
   --                Tmp (J) := E;
   --             end loop;

   function Build_Array_Aggr_Code
     (N           : Node_Id;
      Index       : Node_Id;
      Into        : Node_Id;
      Scalar_Comp : Boolean;
      Indices     : List_Id := No_List;
      Flist       : Node_Id := Empty)
      return        List_Id
   is
      Loc          : constant Source_Ptr := Sloc (N);
      Index_Base   : constant Entity_Id  := Base_Type (Etype (Index));
      Index_Base_L : constant Node_Id := Type_Low_Bound (Index_Base);
      Index_Base_H : constant Node_Id := Type_High_Bound (Index_Base);

      function Add (Val : Int; To : Node_Id) return Node_Id;
      --  Returns an expression where Val is added to expression To,
      --  unless To+Val is provably out of To's base type range.
      --  To must be an already analyzed expression.

      function Empty_Range (L, H : Node_Id) return Boolean;
      --  Returns True if the range defined by L .. H is certainly empty.

      function Equal (L, H : Node_Id) return Boolean;
      --  Returns True if L = H for sure.

      function Index_Base_Name return Node_Id;
      --  Returns a new reference to the index type name.

      function Gen_Assign (Ind : Node_Id; Expr : Node_Id) return List_Id;
      --  Ind must be a side-effect free expression.
      --  If the input aggregate N to Build_Loop contains no sub-aggregates,
      --  This routine returns the assignment statement
      --
      --     Into (Indices, Ind) := Expr;
      --
      --  Otherwise we call Build_Code recursively.

      function Gen_Loop (L, H : Node_Id; Expr : Node_Id) return List_Id;
      --  Nodes L and H must be side-effect free expressions.
      --  If the input aggregate N to Build_Loop contains no sub-aggregates,
      --  This routine returns the for loop statement
      --
      --     for J in Index_Base'(L) .. Index_Base'(H) loop
      --        Into (Indices, J) := Expr;
      --     end loop;
      --
      --  Otherwise we call Build_Code recursively.
      --  As an optimization if the loop covers 3 or less scalar elements we
      --  generate a sequence of assignments.

      function Gen_While (L, H : Node_Id; Expr : Node_Id) return List_Id;
      --  Nodes L and H must be side-effect free expressions.
      --  If the input aggregate N to Build_Loop contains no sub-aggregates,
      --  This routine returns the while loop statement
      --
      --     J : Index_Base := L;
      --     while J < H loop
      --        J := Index_Base'Succ (J);
      --        Into (Indices, J) := Expr;
      --     end loop;
      --
      --  Otherwise we call Build_Code recursively.

      function Local_Compile_Time_Known_Value (E : Node_Id) return Boolean;
      function Local_Expr_Value               (E : Node_Id) return Uint;
      --  These two Local routines are used to replace the corresponding ones
      --  in sem_eval because while processing the bounds of an aggregate with
      --  discrete choices whose index type is an enumeration, we build static
      --  expressions not recognized by Compile_Time_Known_Value as such since
      --  they have not yet been analyzed and resolved. All the expressions in
      --  question are things like Index_Base_Name'Val (Const) which we can
      --  easily recognize as being constant.

      ---------
      -- Add --
      ---------

      function Add (Val : Int; To : Node_Id) return Node_Id is
         Expr_Pos : Node_Id;
         Expr     : Node_Id;
         To_Pos   : Node_Id;

         U_To  : Uint;
         U_Val : Uint := UI_From_Int (Val);

      begin
         --  Note: do not try to optimize the case of Val = 0, because
         --  we need to build a new node with the proper Sloc value anyway.

         --  First test if we can do constant folding

         if Local_Compile_Time_Known_Value (To) then
            U_To := Local_Expr_Value (To) + Val;

            --  Determine if our constant is outside the range of the index.
            --  If so return an Empty node. This empty node will be caught
            --  by Empty_Range below.

            if Compile_Time_Known_Value (Index_Base_L)
              and then U_To < Expr_Value (Index_Base_L)
            then
               return Empty;

            elsif Compile_Time_Known_Value (Index_Base_H)
              and then U_To > Expr_Value (Index_Base_H)
            then
               return Empty;
            end if;

            Expr_Pos := Make_Integer_Literal (Loc, U_To);
            Set_Is_Static_Expression (Expr_Pos);

            if not Is_Enumeration_Type (Index_Base) then
               Expr := Expr_Pos;

            --  If we are dealing with enumeration return
            --     Index_Base'Val (Expr_Pos)

            else
               Expr :=
                 Make_Attribute_Reference
                   (Loc,
                    Prefix         => Index_Base_Name,
                    Attribute_Name => Name_Val,
                    Expressions    => New_List (Expr_Pos));
            end if;

            return Expr;
         end if;

         --  If we are here no constant folding possible

         if not Is_Enumeration_Type (Index_Base) then
            Expr :=
              Make_Op_Add (Loc,
                           Left_Opnd  => Duplicate_Subexpr (To),
                           Right_Opnd => Make_Integer_Literal (Loc, U_Val));

         --  If we are dealing with enumeration return
         --    Index_Base'Val (Index_Base'Pos (To) + Val)

         else
            To_Pos :=
              Make_Attribute_Reference
                (Loc,
                 Prefix         => Index_Base_Name,
                 Attribute_Name => Name_Pos,
                 Expressions    => New_List (Duplicate_Subexpr (To)));

            Expr_Pos :=
              Make_Op_Add (Loc,
                           Left_Opnd  => To_Pos,
                           Right_Opnd => Make_Integer_Literal (Loc, U_Val));

            Expr :=
              Make_Attribute_Reference
                (Loc,
                 Prefix         => Index_Base_Name,
                 Attribute_Name => Name_Val,
                 Expressions    => New_List (Expr_Pos));
         end if;

         return Expr;
      end Add;

      -----------------
      -- Empty_Range --
      -----------------

      function Empty_Range (L, H : Node_Id) return Boolean is
         Is_Empty : Boolean := False;
         Low      : Node_Id;
         High     : Node_Id;

      begin
         --  First check if L or H were already detected as overflowing the
         --  index base range type by function Add above. If this is so Add
         --  returns the empty node.

         if No (L) or else No (H) then
            return True;
         end if;

         for J in 1 .. 3 loop
            case J is

               --  L > H    range is empty

               when 1 =>
                  Low  := L;
                  High := H;

               --  B_L > H  range must be empty

               when 2 =>
                  Low  := Index_Base_L;
                  High := H;

               --  L > B_H  range must be empty

               when 3 =>
                  Low  := L;
                  High := Index_Base_H;
            end case;

            if Local_Compile_Time_Known_Value (Low)
              and then Local_Compile_Time_Known_Value (High)
            then
               Is_Empty :=
                 UI_Gt (Local_Expr_Value (Low), Local_Expr_Value (High));
            end if;

            exit when Is_Empty;
         end loop;

         return Is_Empty;
      end Empty_Range;

      -----------
      -- Equal --
      -----------

      function Equal (L, H : Node_Id) return Boolean is
      begin
         if L = H then
            return True;

         elsif Local_Compile_Time_Known_Value (L)
           and then Local_Compile_Time_Known_Value (H)
         then
            return UI_Eq (Local_Expr_Value (L), Local_Expr_Value (H));
         end if;

         return False;
      end Equal;

      ----------------
      -- Gen_Assign --
      ----------------

      function Gen_Assign (Ind : Node_Id; Expr : Node_Id) return List_Id is
         L : List_Id := New_List;
         F : Entity_Id;
         A : Node_Id;

         New_Indices  : List_Id;
         Indexed_Comp : Node_Id;
         Expr_Q       : Node_Id;
         Comp_Type    : Entity_Id := Empty;

         function Add_Loop_Actions (Lis : List_Id) return List_Id;
         --  Collect insert_actions generated in the construction of a
         --  loop, and prepend them to the sequence of assignments to
         --  complete the eventual body of the loop.

         ----------------------
         -- Add_Loop_Actions --
         ----------------------

         function Add_Loop_Actions (Lis : List_Id) return List_Id is
            Res : List_Id;

         begin
            if Nkind (Parent (Expr)) = N_Component_Association
              and then Present (Loop_Actions (Parent (Expr)))
            then
               Append_List (Lis, Loop_Actions (Parent (Expr)));
               Res := Loop_Actions (Parent (Expr));
               Set_Loop_Actions (Parent (Expr), No_List);
               return Res;

            else
               return Lis;
            end if;
         end Add_Loop_Actions;

      --  Start of processing for Gen_Assign

      begin
         if No (Indices) then
            New_Indices := New_List;
         else
            New_Indices := New_Copy_List_Tree (Indices);
         end if;

         Append_To (New_Indices, Ind);

         if Present (Flist) then
            F := New_Copy_Tree (Flist);

         elsif Present (Etype (N)) and then Controlled_Type (Etype (N)) then
            if Is_Entity_Name (Into)
              and then Present (Scope (Entity (Into)))
            then
               F := Find_Final_List (Scope (Entity (Into)));

            else
               F := Find_Final_List (Current_Scope);
            end if;
         else
            F := 0;
         end if;

         if Present (Next_Index (Index)) then
            return
              Add_Loop_Actions (
                Build_Array_Aggr_Code
                  (Expr, Next_Index (Index),
                    Into, Scalar_Comp, New_Indices, F));
         end if;

         --  If we get here then we are at a bottom-level (sub-)aggregate

         Indexed_Comp :=  Checks_Off (
             Make_Indexed_Component (Loc,
               Prefix      => New_Copy_Tree (Into),
               Expressions => New_Indices));

         Set_Assignment_OK (Indexed_Comp);

         if Nkind (Expr) = N_Qualified_Expression then
            Expr_Q := Expression (Expr);
         else
            Expr_Q := Expr;
         end if;

         if Present (Etype (N))
           and then Etype (N) /= Any_Composite
         then
            Comp_Type := Component_Type (Etype (N));

         elsif Present (Next (First (New_Indices))) then

            --  this is a multidimensional array. Recover the component
            --  type from the outermost aggregate, because subaggregates
            --  do not have an assigned type.

            declare
               P : Node_Id := Parent (Expr);

            begin
               while Present (P) loop

                  if Nkind (P) = N_Aggregate
                    and then Present (Etype (P))
                  then
                     Comp_Type := Component_Type (Etype (P));
                     exit;

                  else
                     P := Parent (P);
                  end if;
               end loop;
            end;
         end if;

         if (Nkind (Expr_Q) = N_Aggregate
           or else Nkind (Expr_Q) = N_Extension_Aggregate)
         then

            --  At this stage the Expression may not have been
            --  analyzed yet because the array aggregate code has not
            --  been updated to use the Expansion_Delayed flag and
            --  avoid analysis altogether to solve the same problem
            --  (see Resolve_Aggr_Expr) so let's do the analysis of
            --  non-array aggregates now in order to get the value of
            --  Expansion_Delayed flag for the inner aggregate ???

            if Present (Comp_Type) and then not Is_Array_Type (Comp_Type) then
               Analyze_And_Resolve (Expr_Q, Comp_Type);
            end if;

            if Is_Delayed_Aggregate (Expr_Q) then
               return
                 Add_Loop_Actions (
                   Late_Expansion (Expr_Q, Etype (Expr_Q), Indexed_Comp, F));
            end if;
         end if;

         --  Now generate the assignment with no associated controlled
         --  actions since the target of the assignment may not have
         --  been initialized, it is not possible to Finalize it as
         --  expected by normal controlled assignment. The rest of the
         --  controlled actions are done manually with the proper
         --  finalization list coming from the context.

         A :=
           Make_OK_Assignment_Statement (Loc,
             Name       => Indexed_Comp,
             Expression => New_Copy_Tree (Expr));

         if Present (Comp_Type) and then Controlled_Type (Comp_Type) then
            Set_No_Ctrl_Actions (A);
         end if;

         Append_To (L, A);

         --  Adjust the tag if tagged (because of possible view
         --  conversions), unless compiling for the Java VM
         --  where tags are implicit.

         if Present (Comp_Type)
           and then Is_Tagged_Type (Comp_Type)
           and then not Java_VM
         then
            A :=
              Make_OK_Assignment_Statement (Loc,
                Name =>
                  Make_Selected_Component (Loc,
                    Prefix =>  New_Copy_Tree (Indexed_Comp),
                    Selector_Name =>
                      New_Reference_To (Tag_Component (Comp_Type), Loc)),

                Expression =>
                  Unchecked_Convert_To (RTE (RE_Tag),
                    New_Reference_To (
                      Access_Disp_Table (Comp_Type), Loc)));

            Append_To (L, A);
         end if;

         --  Adjust and Attach the component to the proper final list
         --  which can be the controller of the outer record object or
         --  the final list associated with the scope

         if Present (Comp_Type)  and then Controlled_Type (Comp_Type) then
            Append_List_To (L,
              Make_Adjust_Call (
                Ref         => New_Copy_Tree (Indexed_Comp),
                Typ         => Comp_Type,
                Flist_Ref   => F,
                With_Attach => Make_Integer_Literal (Loc, 1)));
         end if;

         return Add_Loop_Actions (L);
      end Gen_Assign;

      --------------
      -- Gen_Loop --
      --------------

      function Gen_Loop (L, H : Node_Id; Expr : Node_Id) return List_Id is
         L_J : Node_Id;

         L_Range : Node_Id;
         --  Index_Base'(L) .. Index_Base'(H)

         L_Iteration_Scheme : Node_Id;
         --  L_J in Index_Base'(L) .. Index_Base'(H)

         L_Body : List_Id;
         --  The statements to execute in the loop

         S : List_Id := New_List;
         --  list of statement

         Tcopy : Node_Id;
         --  Copy of expression tree, used for checking purposes

      begin
         --  If loop bounds define an empty range return the null statement

         if Empty_Range (L, H) then
            Append_To (S, Make_Null_Statement (Loc));

            --  The expression must be type-checked even though no component
            --  of the aggregate will have this value. This is done only for
            --  actual components of the array, not for subaggregates. Do the
            --  check on a copy, because the expression may be shared among
            --  several choices, some of which might be non-null.

            if Present (Etype (N))
              and then Is_Array_Type (Etype (N))
              and then No (Next_Index (Index))
            then
               Expander_Mode_Save_And_Set (False);
               Tcopy := New_Copy_Tree (Expr);
               Set_Parent (Tcopy, N);
               Analyze_And_Resolve (Tcopy, Component_Type (Etype (N)));
               Expander_Mode_Restore;
            end if;

            return S;

         --  If loop bounds are the same then generate an assignment

         elsif Equal (L, H) then
            return Gen_Assign (New_Copy_Tree (L), Expr);

         --  If H - L <= 2 then generate a sequence of assignments
         --  when we are processing the bottom most aggregate and it contains
         --  scalar components.

         elsif No (Next_Index (Index))
           and then Scalar_Comp
           and then Local_Compile_Time_Known_Value (L)
           and then Local_Compile_Time_Known_Value (H)
           and then Local_Expr_Value (H) - Local_Expr_Value (L) <= 2
         then
            Append_List_To (S, Gen_Assign (New_Copy_Tree (L), Expr));
            Append_List_To (S, Gen_Assign (Add (1, To => L), Expr));

            if Local_Expr_Value (H) - Local_Expr_Value (L) = 2 then
               Append_List_To (S, Gen_Assign (Add (2, To => L), Expr));
            end if;

            return S;
         end if;

         --  Otherwise construct the loop, starting with the loop index L_J

         L_J := Make_Defining_Identifier (Loc, New_Internal_Name ('J'));

         --  Construct "L .. H"

         L_Range :=
           Make_Range
             (Loc,
              Low_Bound  => Make_Qualified_Expression
                              (Loc,
                               Subtype_Mark => Index_Base_Name,
                               Expression   => L),
              High_Bound => Make_Qualified_Expression
                              (Loc,
                               Subtype_Mark => Index_Base_Name,
                               Expression => H));

         --  Construct "for L_J in Index_Base range L .. H"

         L_Iteration_Scheme :=
           Make_Iteration_Scheme
             (Loc,
              Loop_Parameter_Specification =>
                Make_Loop_Parameter_Specification
                  (Loc,
                   Defining_Identifier         => L_J,
                   Discrete_Subtype_Definition => L_Range));

         --  Construct the statements to execute in the loop body

         L_Body := Gen_Assign (New_Reference_To (L_J, Loc), Expr);

         --  Construct the final loop

         Append_To (S, Make_Implicit_Loop_Statement
                         (Node             => N,
                          Identifier       => Empty,
                          Iteration_Scheme => L_Iteration_Scheme,
                          Statements       => L_Body));

         return S;
      end Gen_Loop;

      ---------------
      -- Gen_While --
      ---------------

      --  The code built is

      --     W_J : Index_Base := L;
      --     while W_J < H loop
      --        W_J := Index_Base'Succ (W);
      --        L_Body;
      --     end loop;

      function Gen_While (L, H : Node_Id; Expr : Node_Id) return List_Id is

         W_J : Node_Id;

         W_Decl : Node_Id;
         --  W_J : Base_Type := L;

         W_Iteration_Scheme : Node_Id;
         --  while W_J < H

         W_Index_Succ : Node_Id;
         --  Index_Base'Succ (J)

         W_Increment  : Node_Id;
         --  W_J := Index_Base'Succ (W)

         W_Body : List_Id := New_List;
         --  The statements to execute in the loop

         S : List_Id := New_List;
         --  list of statement

      begin
         --  If loop bounds define an empty range or are equal return null

         if Empty_Range (L, H) or else Equal (L, H) then
            Append_To (S, Make_Null_Statement (Loc));
            return S;
         end if;

         --  Build the decl of W_J

         W_J    := Make_Defining_Identifier (Loc, New_Internal_Name ('J'));
         W_Decl :=
           Make_Object_Declaration
             (Loc,
              Defining_Identifier => W_J,
              Object_Definition   => Index_Base_Name,
              Expression          => L);

         --  Theoretically we should do a New_Copy_Tree (L) here, but we know
         --  that in this particular case L is a fresh Expr generated by
         --  Add which we are the only ones to use.

         Append_To (S, W_Decl);

         --  construct " while W_J < H"

         W_Iteration_Scheme :=
           Make_Iteration_Scheme
             (Loc,
              Condition => Make_Op_Lt
                             (Loc,
                              Left_Opnd  => New_Reference_To (W_J, Loc),
                              Right_Opnd => New_Copy_Tree (H)));

         --  Construct the statements to execute in the loop body

         W_Index_Succ :=
           Make_Attribute_Reference
             (Loc,
              Prefix         => Index_Base_Name,
              Attribute_Name => Name_Succ,
              Expressions    => New_List (New_Reference_To (W_J, Loc)));

         W_Increment  :=
           Make_OK_Assignment_Statement
             (Loc,
              Name       => New_Reference_To (W_J, Loc),
              Expression => W_Index_Succ);

         Append_To (W_Body, W_Increment);
         Append_List_To (W_Body,
           Gen_Assign (New_Reference_To (W_J, Loc), Expr));

         --  Construct the final loop

         Append_To (S, Make_Implicit_Loop_Statement
                         (Node             => N,
                          Identifier       => Empty,
                          Iteration_Scheme => W_Iteration_Scheme,
                          Statements       => W_Body));

         return S;
      end Gen_While;

      ---------------------
      -- Index_Base_Name --
      ---------------------

      function Index_Base_Name return Node_Id is
      begin
         return New_Reference_To (Index_Base, Sloc (N));
      end Index_Base_Name;

      ------------------------------------
      -- Local_Compile_Time_Known_Value --
      ------------------------------------

      function Local_Compile_Time_Known_Value (E : Node_Id) return Boolean is
      begin
         return Compile_Time_Known_Value (E)
           or else
             (Nkind (E) = N_Attribute_Reference
              and then Attribute_Name (E) = Name_Val
              and then Compile_Time_Known_Value (First (Expressions (E))));
      end Local_Compile_Time_Known_Value;

      ----------------------
      -- Local_Expr_Value --
      ----------------------

      function Local_Expr_Value (E : Node_Id) return Uint is
      begin
         if Compile_Time_Known_Value (E) then
            return Expr_Value (E);
         else
            return Expr_Value (First (Expressions (E)));
         end if;
      end Local_Expr_Value;

      --  Build_Array_Aggr_Code Variables

      Assoc  : Node_Id;
      Choice : Node_Id;
      Expr   : Node_Id;

      Others_Expr : Node_Id   := Empty;

      Aggr_L : constant Node_Id := Low_Bound (Aggregate_Bounds (N));
      Aggr_H : constant Node_Id := High_Bound (Aggregate_Bounds (N));
      --  The aggregate bounds of this specific sub-aggregate. Note that if
      --  the code generated by Build_Array_Aggr_Code is executed then these
      --  bounds are OK. Otherwise a Constraint_Error would have been raised.

      Aggr_Low  : constant Node_Id := Duplicate_Subexpr (Aggr_L);
      Aggr_High : constant Node_Id := Duplicate_Subexpr (Aggr_H);
      --  After Duplicate_Subexpr these are side-effect free.

      Low  : Node_Id;
      High : Node_Id;

      Nb_Choices : Nat := 0;
      Table      : Case_Table_Type (1 .. Number_Of_Choices (N));
      --  Used to sort all the different choice values

      Nb_Elements : Int;
      --  Number of elements in the positional aggregate

      New_Code : List_Id := New_List;

   --  Start of processing for Build_Array_Aggr_Code

   begin
      --  STEP 1: Process component associations

      if No (Expressions (N)) then

         --  STEP 1 (a): Sort the discrete choices

         Assoc := First (Component_Associations (N));
         while Present (Assoc) loop

            Choice := First (Choices (Assoc));
            while Present (Choice) loop

               if Nkind (Choice) = N_Others_Choice then
                  Others_Expr := Expression (Assoc);
                  exit;
               end if;

               Get_Index_Bounds (Choice, Low, High);

               Nb_Choices := Nb_Choices + 1;
               Table (Nb_Choices) := (Choice_Lo   => Low,
                                      Choice_Hi   => High,
                                      Choice_Node => Expression (Assoc));

               Next (Choice);
            end loop;

            Next (Assoc);
         end loop;

         --  If there is more than one set of choices these must be static
         --  and we can therefore sort them. Remember that Nb_Choices does not
         --  account for an others choice.

         if Nb_Choices > 1 then
            Sort_Case_Table (Table);
         end if;

         --  STEP 1 (b):  take care of the whole set of discrete choices.

         for J in 1 .. Nb_Choices loop
            Low  := Table (J).Choice_Lo;
            High := Table (J).Choice_Hi;
            Expr := Table (J).Choice_Node;

            Append_List (Gen_Loop (Low, High, Expr), To => New_Code);
         end loop;

         --  STEP 1 (c): generate the remaining loops to cover others choice
         --  We don't need to generate loops over empty gaps, but if there is
         --  a single empty range we must analyze the expression for semantics

         if Present (Others_Expr) then
            declare
               First : Boolean := True;

            begin
               for J in 0 .. Nb_Choices loop

                  if J = 0 then
                     Low := Aggr_Low;
                  else
                     Low := Add (1, To => Table (J).Choice_Hi);
                  end if;

                  if J = Nb_Choices then
                     High := Aggr_High;
                  else
                     High := Add (-1, To => Table (J + 1).Choice_Lo);
                  end if;

                  --  If this is an expansion within an init_proc, make
                  --  sure that discriminant references are replaced by
                  --  the corresponding discriminal.

                  if Inside_Init_Proc then
                     if Is_Entity_Name (Low)
                       and then Ekind (Entity (Low)) = E_Discriminant
                     then
                        Set_Entity (Low, Discriminal (Entity (Low)));
                     end if;

                     if Is_Entity_Name (High)
                       and then Ekind (Entity (High)) = E_Discriminant
                     then
                        Set_Entity (High, Discriminal (Entity (High)));
                     end if;
                  end if;

                  if First
                    or else not Empty_Range (Low, High)
                  then
                     First := False;
                     Append_List
                       (Gen_Loop (Low, High, Others_Expr), To => New_Code);
                  end if;
               end loop;
            end;
         end if;

      --  STEP 2: Process positional components

      else
         --  STEP 2 (a): Generate the assignments for each positional element
         --  Note that here we have to use Aggr_L rather than Aggr_Low because
         --  Aggr_L is analyzed and Add wants an analyzed expression.

         Expr        := First (Expressions (N));
         Nb_Elements := -1;

         while Present (Expr) loop
            Nb_Elements := Nb_Elements + 1;
            Append_List (Gen_Assign (Add (Nb_Elements, To => Aggr_L), Expr),
                         To => New_Code);
            Next (Expr);
         end loop;

         --  STEP 2 (b): Generate final loop if an others choice is present
         --  Here Nb_Elements gives the offset of the last positional element.

         if Present (Component_Associations (N)) then
            Assoc := Last (Component_Associations (N));
            Expr  := Expression (Assoc);

            Append_List (Gen_While (Add (Nb_Elements, To => Aggr_L),
                                    Aggr_High,
                                    Expr),
                         To => New_Code);
         end if;
      end if;

      return New_Code;
   end Build_Array_Aggr_Code;

   ----------------------------
   -- Build_Record_Aggr_Code --
   ----------------------------

   function Build_Record_Aggr_Code
     (N      : Node_Id;
      Typ    : Entity_Id;
      Target : Node_Id;
      Flist  : Node_Id   := Empty;
      Obj    : Entity_Id := Empty)
      return   List_Id
   is
      Loc     : constant Source_Ptr := Sloc (N);
      L       : constant List_Id    := New_List;
      Start_L : constant List_Id    := New_List;
      N_Typ   : constant Entity_Id  := Etype (N);

      Comp      : Node_Id;
      Instr     : Node_Id;
      Ref       : Node_Id;
      F         : Node_Id;
      Comp_Type : Entity_Id;
      Selector  : Entity_Id;
      Comp_Expr : Node_Id;
      Comp_Kind : Node_Kind;
      Expr_Q    : Node_Id;

      Internal_Final_List : Node_Id;

      --  If this is an internal aggregate, the External_Final_List is an
      --  expression for the controller record of the enclosing type.
      --  If the current aggregate has several controlled components, this
      --  expression will appear in several calls to attach to the finali-
      --  zation list, and it must not be shared.

      External_Final_List      : Node_Id;
      Ancestor_Is_Expression   : Boolean := False;
      Ancestor_Is_Subtype_Mark : Boolean := False;

      Init_Typ : Entity_Id := Empty;
      Attach   : Node_Id;

      function Get_Constraint_Association (T : Entity_Id) return Node_Id;
      --  Returns the first discriminant association in the constraint
      --  associated with T, if any, otherwise returns Empty.

      function Ancestor_Discriminant_Value (Disc : Entity_Id) return Node_Id;
      --  Returns the value that the given discriminant of an ancestor
      --  type should receive (in the absence of a conflict with the
      --  value provided by an ancestor part of an extension aggregate).

      procedure Check_Ancestor_Discriminants (Anc_Typ : Entity_Id);
      --  Check that each of the discriminant values defined by the
      --  ancestor part of an extension aggregate match the corresponding
      --  values provided by either an association of the aggregate or
      --  by the constraint imposed by a parent type (RM95-4.3.2(8)).

      function Init_Controller
        (Target  : Node_Id;
         Typ     : Entity_Id;
         F       : Node_Id;
         Attach  : Node_Id;
         Init_Pr : Boolean)
        return List_Id;
      --  returns the list of statements necessary to initialize the internal
      --  controller of the (possible) ancestor typ into target and attach
      --  it to finalization list F. Init_Pr conditions the call to the
      --  init_proc since it may already be done due to ancestor initialization

      ---------------------------------
      -- Ancestor_Discriminant_Value --
      ---------------------------------

      function Ancestor_Discriminant_Value (Disc : Entity_Id) return Node_Id is
         Assoc        : Node_Id;
         Assoc_Elmt   : Elmt_Id;
         Aggr_Comp    : Entity_Id;
         Corresp_Disc : Entity_Id;
         Current_Typ  : Entity_Id := Base_Type (Typ);
         Parent_Typ   : Entity_Id;
         Parent_Disc  : Entity_Id;
         Save_Assoc   : Node_Id := Empty;

      begin
         --  First check any discriminant associations to see if
         --  any of them provide a value for the discriminant.

         if Present (Discriminant_Specifications (Parent (Current_Typ))) then
            Assoc := First (Component_Associations (N));
            while Present (Assoc) loop
               Aggr_Comp := Entity (First (Choices (Assoc)));

               if Ekind (Aggr_Comp) = E_Discriminant then
                  Save_Assoc := Expression (Assoc);

                  Corresp_Disc := Corresponding_Discriminant (Aggr_Comp);
                  while Present (Corresp_Disc) loop
                     --  If found a corresponding discriminant then return
                     --  the value given in the aggregate. (Note: this is
                     --  not correct in the presence of side effects. ???)

                     if Disc = Corresp_Disc then
                        return Duplicate_Subexpr (Expression (Assoc));
                     end if;
                     Corresp_Disc :=
                       Corresponding_Discriminant (Corresp_Disc);
                  end loop;
               end if;

               Next (Assoc);
            end loop;
         end if;

         --  No match found in aggregate, so chain up parent types to find
         --  a constraint that defines the value of the discriminant.

         Parent_Typ := Etype (Current_Typ);
         while Current_Typ /= Parent_Typ loop
            if Has_Discriminants (Parent_Typ) then
               Parent_Disc := First_Discriminant (Parent_Typ);

               --  We either get the association from the subtype indication
               --  of the type definition itself, or from the discriminant
               --  constraint associated with the type entity (which is
               --  preferable, but it's not always present ???)

               if Is_Empty_Elmt_List (
                 Discriminant_Constraint (Current_Typ))
               then
                  Assoc := Get_Constraint_Association (Current_Typ);
                  Assoc_Elmt := No_Elmt;
               else
                  Assoc_Elmt :=
                    First_Elmt (Discriminant_Constraint (Current_Typ));
                  Assoc := Node (Assoc_Elmt);
               end if;

               --  Traverse the discriminants of the parent type looking
               --  for one that corresponds.

               while Present (Parent_Disc) and then Present (Assoc) loop
                  Corresp_Disc := Parent_Disc;
                  while Present (Corresp_Disc)
                    and then Disc /= Corresp_Disc
                  loop
                     Corresp_Disc :=
                       Corresponding_Discriminant (Corresp_Disc);
                  end loop;

                  if Disc = Corresp_Disc then
                     if Nkind (Assoc) = N_Discriminant_Association then
                        Assoc := Expression (Assoc);
                     end if;

                     --  If the located association directly denotes
                     --  a discriminant, then use the value of a saved
                     --  association of the aggregate. This is a kludge
                     --  to handle certain cases involving multiple
                     --  discriminants mapped to a single discriminant
                     --  of a descendant. It's not clear how to locate the
                     --  appropriate discriminant value for such cases. ???

                     if Is_Entity_Name (Assoc)
                       and then Ekind (Entity (Assoc)) = E_Discriminant
                     then
                        Assoc := Save_Assoc;
                     end if;

                     return Duplicate_Subexpr (Assoc);
                  end if;

                  Next_Discriminant (Parent_Disc);

                  if No (Assoc_Elmt) then
                     Next (Assoc);
                  else
                     Next_Elmt (Assoc_Elmt);
                     if Present (Assoc_Elmt) then
                        Assoc := Node (Assoc_Elmt);
                     else
                        Assoc := Empty;
                     end if;
                  end if;
               end loop;
            end if;

            Current_Typ := Parent_Typ;
            Parent_Typ := Etype (Current_Typ);
         end loop;

         --  In some cases there's no ancestor value to locate (such as
         --  when an ancestor part given by an expression defines the
         --  discriminant value).

         return Empty;
      end Ancestor_Discriminant_Value;

      ----------------------------------
      -- Check_Ancestor_Discriminants --
      ----------------------------------

      procedure Check_Ancestor_Discriminants (Anc_Typ : Entity_Id) is
         Discr      : Entity_Id := First_Discriminant (Base_Type (Anc_Typ));
         Disc_Value : Node_Id;
         Cond       : Node_Id;

      begin
         while Present (Discr) loop
            Disc_Value := Ancestor_Discriminant_Value (Discr);

            if Present (Disc_Value) then
               Cond := Make_Op_Ne (Loc,
                 Left_Opnd =>
                   Make_Selected_Component (Loc,
                     Prefix        => New_Copy_Tree (Target),
                     Selector_Name => New_Occurrence_Of (Discr, Loc)),
                 Right_Opnd => Disc_Value);

               Append_To (L,
                 Make_Raise_Constraint_Error (Loc,
                   Condition => Cond,
                   Reason    => CE_Discriminant_Check_Failed));
            end if;

            Next_Discriminant (Discr);
         end loop;
      end Check_Ancestor_Discriminants;

      --------------------------------
      -- Get_Constraint_Association --
      --------------------------------

      function Get_Constraint_Association (T : Entity_Id) return Node_Id is
         Typ_Def : constant Node_Id := Type_Definition (Parent (T));
         Indic   : constant Node_Id := Subtype_Indication (Typ_Def);

      begin
         --  ??? Also need to cover case of a type mark denoting a subtype
         --  with constraint.

         if Nkind (Indic) = N_Subtype_Indication
           and then Present (Constraint (Indic))
         then
            return First (Constraints (Constraint (Indic)));
         end if;

         return Empty;
      end Get_Constraint_Association;

      ---------------------
      -- Init_controller --
      ---------------------

      function Init_Controller
        (Target  : Node_Id;
         Typ     : Entity_Id;
         F       : Node_Id;
         Attach  : Node_Id;
         Init_Pr : Boolean)
        return List_Id
      is
         Ref : Node_Id;
         L   : List_Id := New_List;

      begin
         --     _init_proc (target._controller);
         --     initialize (target._controller);
         --     Attach_to_Final_List (target._controller, F);

         Ref := Make_Selected_Component (Loc,
                  Prefix        => Convert_To (Typ, New_Copy_Tree (Target)),
                  Selector_Name => Make_Identifier (Loc, Name_uController));
         Set_Assignment_OK (Ref);

         if Init_Pr then
            Append_List_To (L,
              Build_Initialization_Call (Loc,
                Id_Ref       => Ref,
                Typ          => RTE (RE_Record_Controller),
                In_Init_Proc => Within_Init_Proc));
         end if;

         Append_To (L,
           Make_Procedure_Call_Statement (Loc,
             Name =>
               New_Reference_To (Find_Prim_Op (RTE (RE_Record_Controller),
                 Name_Initialize), Loc),
             Parameter_Associations => New_List (New_Copy_Tree (Ref))));

         Append_To (L,
           Make_Attach_Call (
             Obj_Ref     => New_Copy_Tree (Ref),
             Flist_Ref   => F,
             With_Attach => Attach));
         return L;
      end Init_Controller;

   --  Start of processing for Build_Record_Aggr_Code

   begin

      --  Deal with the ancestor part of extension aggregates
      --  or with the discriminants of the root type

      if Nkind (N) = N_Extension_Aggregate then
         declare
            A : constant Node_Id := Ancestor_Part (N);

         begin

            --  If the ancestor part is a subtype mark "T", we generate
            --     _init_proc (T(tmp));  if T is constrained and
            --     _init_proc (S(tmp));  where S applies an appropriate
            --                           constraint if T is unconstrained

            if Is_Entity_Name (A) and then Is_Type (Entity (A)) then

               Ancestor_Is_Subtype_Mark := True;

               if Is_Constrained (Entity (A)) then
                  Init_Typ := Entity (A);

               --  For an ancestor part given by an unconstrained type
               --  mark, create a subtype constrained by appropriate
               --  corresponding discriminant values coming from either
               --  associations of the aggregate or a constraint on
               --  a parent type. The subtype will be used to generate
               --  the correct default value for the ancestor part.

               elsif Has_Discriminants (Entity (A)) then
                  declare
                     Anc_Typ    : Entity_Id := Entity (A);
                     Discrim    : Entity_Id := First_Discriminant (Anc_Typ);
                     Anc_Constr : List_Id := New_List;
                     Disc_Value : Node_Id;
                     New_Indic  : Node_Id;
                     Subt_Decl  : Node_Id;
                  begin
                     while Present (Discrim) loop
                        Disc_Value := Ancestor_Discriminant_Value (Discrim);
                        Append_To (Anc_Constr, Disc_Value);
                        Next_Discriminant (Discrim);
                     end loop;

                     New_Indic :=
                       Make_Subtype_Indication (Loc,
                         Subtype_Mark => New_Occurrence_Of (Anc_Typ, Loc),
                         Constraint   =>
                           Make_Index_Or_Discriminant_Constraint (Loc,
                             Constraints => Anc_Constr));

                     Init_Typ := Create_Itype (Ekind (Anc_Typ), N);

                     Subt_Decl :=
                       Make_Subtype_Declaration (Loc,
                         Defining_Identifier => Init_Typ,
                         Subtype_Indication  => New_Indic);

                     --  Itypes must be analyzed with checks off
                     --  Declaration must have a parent for proper
                     --  handling of subsidiary actions.

                     Set_Parent (Subt_Decl, N);
                     Analyze (Subt_Decl, Suppress => All_Checks);
                  end;
               end if;

               Ref := Convert_To (Init_Typ, New_Copy_Tree (Target));
               Set_Assignment_OK (Ref);

               Append_List_To (Start_L,
                 Build_Initialization_Call (Loc,
                   Id_Ref => Ref,
                   Typ    => Init_Typ,
                   In_Init_Proc => Within_Init_Proc));

               if Is_Constrained (Entity (A))
                 and then Has_Discriminants (Entity (A))
               then
                  Check_Ancestor_Discriminants (Entity (A));
               end if;

            --  If the ancestor part is an expression "E", we generate
            --     T(tmp) := E;

            else
               Ancestor_Is_Expression := True;
               Init_Typ := Etype (A);

               --  Assign the tag before doing the assignment to make sure
               --  that the dispatching call in the subsequent deep_adjust
               --  works properly (unless Java_VM, where tags are implicit).

               if not Java_VM then
                  Instr :=
                    Make_OK_Assignment_Statement (Loc,
                      Name =>
                        Make_Selected_Component (Loc,
                          Prefix => New_Copy_Tree (Target),
                          Selector_Name => New_Reference_To (
                            Tag_Component (Base_Type (Typ)), Loc)),

                      Expression =>
                        Unchecked_Convert_To (RTE (RE_Tag),
                          New_Reference_To (
                            Access_Disp_Table (Base_Type (Typ)), Loc)));

                  Set_Assignment_OK (Name (Instr));
                  Append_To (L, Instr);
               end if;

               --  If the ancestor part is an aggregate, force its full
               --  expansion, which was delayed.

               if Nkind (A) = N_Qualified_Expression
                 and then (Nkind (Expression (A)) = N_Aggregate
                             or else
                           Nkind (Expression (A)) = N_Extension_Aggregate)
               then
                  Set_Analyzed (A, False);
                  Set_Analyzed (Expression (A), False);
               end if;

               Ref := Convert_To (Init_Typ, New_Copy_Tree (Target));
               Set_Assignment_OK (Ref);
               Append_To (L,
                 Make_Unsuppress_Block (Loc,
                   Name_Discriminant_Check,
                   New_List (
                     Make_OK_Assignment_Statement (Loc,
                       Name       => Ref,
                       Expression => A))));

               if Has_Discriminants (Init_Typ) then
                  Check_Ancestor_Discriminants (Init_Typ);
               end if;
            end if;
         end;

      else
         --  Generate the discriminant expressions, component by component.
         --  If the base type is an unchecked union, the discriminants are
         --  unknown to the back-end and absent from a value of the type, so
         --  assignments for them are not emitted.

         if Has_Discriminants (Typ)
           and then not Is_Unchecked_Union (Base_Type (Typ))
         then

            --  ??? The discriminants of the object not inherited in the type
            --  of the object should be initialized here

            null;

            --  Generate discriminant init values

            declare
               Discriminant : Entity_Id;
               Discriminant_Value : Node_Id;

            begin
               Discriminant := First_Girder_Discriminant (Typ);

               while Present (Discriminant) loop

                  Comp_Expr :=
                    Make_Selected_Component (Loc,
                      Prefix        => New_Copy_Tree (Target),
                      Selector_Name => New_Occurrence_Of (Discriminant, Loc));

                  Discriminant_Value :=
                    Get_Discriminant_Value (
                      Discriminant,
                      N_Typ,
                      Discriminant_Constraint (N_Typ));

                  Instr :=
                    Make_OK_Assignment_Statement (Loc,
                      Name       => Comp_Expr,
                      Expression => New_Copy_Tree (Discriminant_Value));

                  Set_No_Ctrl_Actions (Instr);
                  Append_To (L, Instr);

                  Next_Girder_Discriminant (Discriminant);
               end loop;
            end;
         end if;
      end if;

      --  Generate the assignments, component by component

      --    tmp.comp1 := Expr1_From_Aggr;
      --    tmp.comp2 := Expr2_From_Aggr;
      --    ....

      Comp := First (Component_Associations (N));
      while Present (Comp) loop
         Selector  := Entity (First (Choices (Comp)));

         if Ekind (Selector) /= E_Discriminant
           or else Nkind (N) = N_Extension_Aggregate
         then
            Comp_Type := Etype (Selector);
            Comp_Kind := Nkind (Expression (Comp));
            Comp_Expr :=
              Make_Selected_Component (Loc,
                Prefix        => New_Copy_Tree (Target),
                Selector_Name => New_Occurrence_Of (Selector, Loc));

            if Nkind (Expression (Comp)) = N_Qualified_Expression then
               Expr_Q := Expression (Expression (Comp));
            else
               Expr_Q := Expression (Comp);
            end if;

            --  The controller is the one of the parent type defining
            --  the component (in case of inherited components).

            if Controlled_Type (Comp_Type) then
               Internal_Final_List :=
                 Make_Selected_Component (Loc,
                   Prefix => Convert_To (
                     Scope (Original_Record_Component (Selector)),
                     New_Copy_Tree (Target)),
                   Selector_Name =>
                     Make_Identifier (Loc, Name_uController));
               Internal_Final_List :=
                 Make_Selected_Component (Loc,
                   Prefix => Internal_Final_List,
                   Selector_Name => Make_Identifier (Loc, Name_F));

               --  The internal final list can be part of a constant object

               Set_Assignment_OK (Internal_Final_List);
            else
               Internal_Final_List := Empty;
            end if;

            if Is_Delayed_Aggregate (Expr_Q) then
               Append_List_To (L,
                 Late_Expansion (Expr_Q, Comp_Type, Comp_Expr,
                   Internal_Final_List));
            else
               Instr :=
                 Make_OK_Assignment_Statement (Loc,
                   Name       => Comp_Expr,
                   Expression => Expression (Comp));

               Set_No_Ctrl_Actions (Instr);
               Append_To (L, Instr);

               --  Adjust the tag if tagged (because of possible view
               --  conversions), unless compiling for the Java VM
               --  where tags are implicit.

               --    tmp.comp._tag := comp_typ'tag;

               if Is_Tagged_Type (Comp_Type) and then not Java_VM then
                  Instr :=
                    Make_OK_Assignment_Statement (Loc,
                      Name =>
                        Make_Selected_Component (Loc,
                          Prefix =>  New_Copy_Tree (Comp_Expr),
                          Selector_Name =>
                            New_Reference_To (Tag_Component (Comp_Type), Loc)),

                      Expression =>
                        Unchecked_Convert_To (RTE (RE_Tag),
                          New_Reference_To (
                            Access_Disp_Table (Comp_Type), Loc)));

                  Append_To (L, Instr);
               end if;

               --  Adjust and Attach the component to the proper controller
               --     Adjust (tmp.comp);
               --     Attach_To_Final_List (tmp.comp,
               --       comp_typ (tmp)._record_controller.f)

               if Controlled_Type (Comp_Type) then
                  Append_List_To (L,
                    Make_Adjust_Call (
                      Ref         => New_Copy_Tree (Comp_Expr),
                      Typ         => Comp_Type,
                      Flist_Ref   => Internal_Final_List,
                      With_Attach => Make_Integer_Literal (Loc, 1)));
               end if;
            end if;
         end if;

         Next (Comp);
      end loop;

      --  If the type is tagged, the tag needs to be initialized (unless
      --  compiling for the Java VM where tags are implicit). It is done
      --  late in the initialization process because in some cases, we call
      --  the init_proc of an ancestor which will not leave out the right tag

      if Ancestor_Is_Expression then
         null;

      elsif Is_Tagged_Type (Typ) and then not Java_VM then
         Instr :=
           Make_OK_Assignment_Statement (Loc,
             Name =>
               Make_Selected_Component (Loc,
                  Prefix => New_Copy_Tree (Target),
                 Selector_Name =>
                   New_Reference_To (Tag_Component (Base_Type (Typ)), Loc)),

             Expression =>
               Unchecked_Convert_To (RTE (RE_Tag),
                 New_Reference_To (Access_Disp_Table (Base_Type (Typ)), Loc)));

         Append_To (L, Instr);
      end if;

      --  Now deal with the various controlled type data structure
      --  initializations

      if Present (Obj)
        and then Finalize_Storage_Only (Typ)
        and then (Is_Library_Level_Entity (Obj)
        or else Entity (Constant_Value (RTE (RE_Garbage_Collected)))
                  = Standard_True)
      then
         Attach := Make_Integer_Literal (Loc, 0);

      elsif Nkind (Parent (N)) = N_Qualified_Expression
        and then Nkind (Parent (Parent (N))) = N_Allocator
      then
         Attach := Make_Integer_Literal (Loc, 2);

      else
         Attach := Make_Integer_Literal (Loc, 1);
      end if;

      --  Determine the external finalization list. It is either the
      --  finalization list of the outer-scope or the one coming from
      --  an outer aggregate.  When the target is not a temporary, the
      --  proper scope is the scope of the target rather than the
      --  potentially transient current scope.

      if Controlled_Type (Typ) then
         if Present (Flist) then
            External_Final_List := New_Copy_Tree (Flist);

         elsif Is_Entity_Name (Target)
           and then Present (Scope (Entity (Target)))
         then
            External_Final_List := Find_Final_List (Scope (Entity (Target)));

         else
            External_Final_List := Find_Final_List (Current_Scope);
         end if;

      else
         External_Final_List := Empty;
      end if;

      --  initialize and attach the outer object in the is_controlled
      --  case

      if Is_Controlled (Typ) then
         if Ancestor_Is_Subtype_Mark then
            Ref := Convert_To (Init_Typ, New_Copy_Tree (Target));
            Set_Assignment_OK (Ref);
            Append_To (L,
              Make_Procedure_Call_Statement (Loc,
                Name => New_Reference_To (
                  Find_Prim_Op (Init_Typ, Name_Initialize), Loc),
                Parameter_Associations => New_List (New_Copy_Tree (Ref))));
         end if;

         --  ??? when the ancestor part is an expression, the global
         --  object is already attached at the wrong level. It should
         --  be detached and re-attached. We have a design problem here.

         if Ancestor_Is_Expression
           and then Has_Controlled_Component (Init_Typ)
         then
            null;

         elsif Has_Controlled_Component (Typ) then
            F := Make_Selected_Component (Loc,
                   Prefix        => New_Copy_Tree (Target),
                   Selector_Name => Make_Identifier (Loc, Name_uController));
            F := Make_Selected_Component (Loc,
                   Prefix        => F,
                   Selector_Name => Make_Identifier (Loc, Name_F));

            Ref := New_Copy_Tree (Target);
            Set_Assignment_OK (Ref);

            Append_To (L,
              Make_Attach_Call (
                Obj_Ref     => Ref,
                Flist_Ref   => F,
                With_Attach => Make_Integer_Literal (Loc, 1)));

         else --  is_Controlled (Typ) and not Has_Controlled_Component (Typ)
            Ref := New_Copy_Tree (Target);
            Set_Assignment_OK (Ref);
            Append_To (Start_L,
              Make_Attach_Call (
                Obj_Ref     => Ref,
                Flist_Ref   => New_Copy_Tree (External_Final_List),
                With_Attach => Attach));
         end if;
      end if;

      --  in the Has_Controlled component case, all the intermediate
      --  controllers must be initialized

      if Has_Controlled_Component (Typ) then
         declare
            Inner_Typ : Entity_Id;
            Outer_Typ : Entity_Id;
            At_Root   : Boolean;

         begin

            Outer_Typ := Base_Type (Typ);

            --  find outer type with a controller

            while Outer_Typ /= Init_Typ
              and then not Has_New_Controlled_Component (Outer_Typ)
            loop
               Outer_Typ := Etype (Outer_Typ);
            end loop;

            --  attach it to the outer record controller to the
            --  external final list

            if Outer_Typ = Init_Typ then
               Append_List_To (Start_L,
                 Init_Controller (
                   Target  => Target,
                   Typ     => Outer_Typ,
                   F       => External_Final_List,
                   Attach  => Attach,
                   Init_Pr => Ancestor_Is_Expression));
               At_Root := True;
               Inner_Typ := Init_Typ;

            else
               Append_List_To (Start_L,
                 Init_Controller (
                   Target  => Target,
                   Typ     => Outer_Typ,
                   F       => External_Final_List,
                   Attach  => Attach,
                   Init_Pr => True));

               Inner_Typ := Etype (Outer_Typ);
               At_Root   :=
                 not Is_Tagged_Type (Typ) or else Inner_Typ = Outer_Typ;
            end if;

            --  Initialize the internal controllers for tagged types with
            --  more than one controller.

            while not At_Root and then Inner_Typ /= Init_Typ loop
               if Has_New_Controlled_Component (Inner_Typ) then
                  F :=
                    Make_Selected_Component (Loc,
                      Prefix => Convert_To (Outer_Typ, New_Copy_Tree (Target)),
                      Selector_Name =>
                        Make_Identifier (Loc, Name_uController));
                  F := Make_Selected_Component (Loc,
                         Prefix => F,
                         Selector_Name => Make_Identifier (Loc, Name_F));
                  Append_List_To (Start_L,
                    Init_Controller (
                      Target  => Target,
                      Typ     => Inner_Typ,
                      F       => F,
                      Attach  => Make_Integer_Literal (Loc, 1),
                      Init_Pr => True));
                  Outer_Typ := Inner_Typ;
               end if;

               --  Stop at the root

               At_Root := Inner_Typ = Etype (Inner_Typ);
               Inner_Typ := Etype (Inner_Typ);
            end loop;

            --  if not done yet attach the controller of the ancestor part

            if Outer_Typ /= Init_Typ
              and then Inner_Typ = Init_Typ
              and then Has_Controlled_Component (Init_Typ)
            then
               F :=
                  Make_Selected_Component (Loc,
                    Prefix => Convert_To (Outer_Typ, New_Copy_Tree (Target)),
                    Selector_Name => Make_Identifier (Loc, Name_uController));
               F := Make_Selected_Component (Loc,
                       Prefix => F,
                       Selector_Name => Make_Identifier (Loc, Name_F));

               Attach := Make_Integer_Literal (Loc, 1);
               Append_List_To (Start_L,
                 Init_Controller (
                   Target  => Target,
                   Typ     => Init_Typ,
                   F       => F,
                   Attach  => Attach,
                   Init_Pr => Ancestor_Is_Expression));
            end if;
         end;
      end if;

      Append_List_To (Start_L, L);
      return Start_L;
   end Build_Record_Aggr_Code;

   -------------------------------
   -- Convert_Aggr_In_Allocator --
   -------------------------------

   procedure Convert_Aggr_In_Allocator (Decl, Aggr : Node_Id) is
      Loc  : constant Source_Ptr := Sloc (Aggr);
      Typ  : constant Entity_Id  := Etype (Aggr);
      Temp : constant Entity_Id  := Defining_Identifier (Decl);
      Occ  : constant Node_Id    := Unchecked_Convert_To (Typ,
        Make_Explicit_Dereference (Loc, New_Reference_To (Temp, Loc)));

      Access_Type : constant Entity_Id := Etype (Temp);

   begin
      Insert_Actions_After (Decl,
        Late_Expansion (Aggr, Typ, Occ,
          Find_Final_List (Access_Type),
          Associated_Final_Chain (Base_Type (Access_Type))));
   end Convert_Aggr_In_Allocator;

   --------------------------------
   -- Convert_Aggr_In_Assignment --
   --------------------------------

   procedure Convert_Aggr_In_Assignment (N : Node_Id) is
      Aggr :          Node_Id    := Expression (N);
      Typ  : constant Entity_Id  := Etype (Aggr);
      Occ  : constant Node_Id    := New_Copy_Tree (Name (N));

   begin
      if Nkind (Aggr) = N_Qualified_Expression then
         Aggr := Expression (Aggr);
      end if;

      Insert_Actions_After (N,
        Late_Expansion (Aggr, Typ, Occ,
          Find_Final_List (Typ, New_Copy_Tree (Occ))));
   end Convert_Aggr_In_Assignment;

   ---------------------------------
   -- Convert_Aggr_In_Object_Decl --
   ---------------------------------

   procedure Convert_Aggr_In_Object_Decl (N : Node_Id) is
      Obj  : constant Entity_Id  := Defining_Identifier (N);
      Aggr :          Node_Id    := Expression (N);
      Loc  : constant Source_Ptr := Sloc (Aggr);
      Typ  : constant Entity_Id  := Etype (Aggr);
      Occ  : constant Node_Id    := New_Occurrence_Of (Obj, Loc);

   begin
      Set_Assignment_OK (Occ);

      if Nkind (Aggr) = N_Qualified_Expression then
         Aggr := Expression (Aggr);
      end if;

      Insert_Actions_After (N, Late_Expansion (Aggr, Typ, Occ, Obj => Obj));
      Set_No_Initialization (N);
      Initialize_Discriminants (N, Typ);
   end Convert_Aggr_In_Object_Decl;

   ----------------------------
   -- Convert_To_Assignments --
   ----------------------------

   procedure Convert_To_Assignments (N : Node_Id; Typ : Entity_Id) is
      Loc  : constant Source_Ptr := Sloc (N);
      Temp : Entity_Id;

      Instr         : Node_Id;
      Target_Expr   : Node_Id;
      Parent_Kind   : Node_Kind;
      Unc_Decl      : Boolean := False;
      Parent_Node   : Node_Id;

   begin

      Parent_Node := Parent (N);
      Parent_Kind := Nkind (Parent_Node);

      if Parent_Kind = N_Qualified_Expression then

         --  Check if we are in a unconstrained declaration because in this
         --  case the current delayed expansion mechanism doesn't work when
         --  the declared object size depend on the initializing expr.

         begin
            Parent_Node := Parent (Parent_Node);
            Parent_Kind := Nkind (Parent_Node);
            if Parent_Kind = N_Object_Declaration then
               Unc_Decl :=
                 not Is_Entity_Name (Object_Definition (Parent_Node))
                 or else Has_Discriminants (
                   Entity (Object_Definition (Parent_Node)))
                 or else Is_Class_Wide_Type (
                   Entity (Object_Definition (Parent_Node)));
            end if;
         end;
      end if;

      --  Just set the Delay flag in the following cases where the
      --  transformation will be done top down from above
      --    - internal aggregate (transformed when expanding the parent)
      --    - allocators  (see Convert_Aggr_In_Allocator)
      --    - object decl (see Convert_Aggr_In_Object_Decl)
      --    - safe assignments (see Convert_Aggr_Assignments)
      --      so far only the assignments in the init_procs are taken
      --      into account

      if Parent_Kind = N_Aggregate
        or else Parent_Kind = N_Extension_Aggregate
        or else Parent_Kind = N_Component_Association
        or else Parent_Kind = N_Allocator
        or else (Parent_Kind = N_Object_Declaration and then not Unc_Decl)
        or else (Parent_Kind = N_Assignment_Statement
                  and then Inside_Init_Proc)
      then
         Set_Expansion_Delayed (N);
         return;
      end if;

      if Requires_Transient_Scope (Typ) then
         Establish_Transient_Scope (N, Sec_Stack =>
              Is_Controlled (Typ) or else Has_Controlled_Component (Typ));
      end if;

      --  Create the temporary

      Temp := Make_Defining_Identifier (Loc, New_Internal_Name ('A'));

      Instr :=
        Make_Object_Declaration (Loc,
          Defining_Identifier => Temp,
          Object_Definition => New_Occurrence_Of (Typ, Loc));

      Set_No_Initialization (Instr);
      Insert_Action (N, Instr);
      Initialize_Discriminants (Instr, Typ);
      Target_Expr := New_Occurrence_Of (Temp, Loc);

      Insert_Actions (N, Build_Record_Aggr_Code (N, Typ, Target_Expr));
      Rewrite (N, New_Occurrence_Of (Temp, Loc));
      Analyze_And_Resolve (N, Typ);
   end Convert_To_Assignments;

   ---------------------------
   -- Convert_To_Positional --
   ---------------------------

   procedure Convert_To_Positional
     (N                    : Node_Id;
      Max_Others_Replicate : Nat := 5;
      Handle_Bit_Packed    : Boolean := False)
   is
      Loc  : constant Source_Ptr := Sloc (N);
      Typ  : constant Entity_Id  := Etype (N);
      Ndim : constant Pos        := Number_Dimensions (Typ);
      Xtyp : constant Entity_Id  := Etype (First_Index (Typ));
      Indx : constant Node_Id    := First_Index (Base_Type (Typ));
      Blo  : constant Node_Id    := Type_Low_Bound (Etype (Indx));
      Lo   : constant Node_Id    := Type_Low_Bound (Xtyp);
      Hi   : constant Node_Id    := Type_High_Bound (Xtyp);
      Lov  : Uint;
      Hiv  : Uint;

      --  The following constant determines the maximum size of an
      --  aggregate produced by converting named to positional
      --  notation (e.g. from others clauses). This avoids running
      --  away with attempts to convert huge aggregates.

      --  The normal limit is 5000, but we increase this limit to
      --  2**24 (about 16 million) if Restrictions (No_Elaboration_Code)
      --  or Restrictions (No_Implicit_Loops) is specified, since in
      --  either case, we are at risk of declaring the program illegal
      --  because of this limit.

      Max_Aggr_Size : constant Nat :=
         5000 + (2 ** 24 - 5000) * Boolean'Pos
                           (Restrictions (No_Elaboration_Code)
                              or else
                            Restrictions (No_Implicit_Loops));

   begin
      --  For now, we only handle the one dimensional case and aggregates
      --  that are not part of a component_association

      if Ndim > 1 or else Nkind (Parent (N)) = N_Aggregate
        or else Nkind (Parent (N)) = N_Component_Association
      then
         return;
      end if;

      --  If already positional, nothing to do!

      if No (Component_Associations (N)) then
         return;
      end if;

      --  Bounds need to be known at compile time

      if not Compile_Time_Known_Value (Lo)
        or else not Compile_Time_Known_Value (Hi)
      then
         return;
      end if;

      --  Normally we do not attempt to convert bit packed arrays. The
      --  exception is when we are explicitly asked to do so (this call
      --  is from the Packed_Array_Aggregate_Handled procedure).

      if Is_Bit_Packed_Array (Typ)
        and then not Handle_Bit_Packed
      then
         return;
      end if;

      --  Do not convert to positional if controlled components are
      --  involved since these require special processing

      if Has_Controlled_Component (Typ) then
         return;
      end if;

      --  Get bounds and check reasonable size (positive, not too large)
      --  Also only handle bounds starting at the base type low bound for now
      --  since the compiler isn't able to handle different low bounds yet.

      Lov := Expr_Value (Lo);
      Hiv := Expr_Value (Hi);

      if Hiv < Lov
        or else (Hiv - Lov > Max_Aggr_Size)
        or else not Compile_Time_Known_Value (Blo)
        or else (Lov /= Expr_Value (Blo))
      then
         return;
      end if;

      --  Bounds must be in integer range (for array Vals below)

      if not UI_Is_In_Int_Range (Lov)
          or else
         not UI_Is_In_Int_Range (Hiv)
      then
         return;
      end if;

      --  Determine if set of alternatives is suitable for conversion
      --  and build an array containing the values in sequence.

      declare
         Vals : array (UI_To_Int (Lov) .. UI_To_Int (Hiv))
                  of Node_Id := (others => Empty);
         --  The values in the aggregate sorted appropriately

         Vlist : List_Id;
         --  Same data as Vals in list form

         Rep_Count : Nat;
         --  Used to validate Max_Others_Replicate limit

         Elmt   : Node_Id;
         Num    : Int := UI_To_Int (Lov);
         Choice : Node_Id;
         Lo, Hi : Node_Id;

      begin
         if Present (Expressions (N)) then
            Elmt := First (Expressions (N));
            while Present (Elmt) loop
               Vals (Num) := Relocate_Node (Elmt);
               Num := Num + 1;
               Next (Elmt);
            end loop;
         end if;

         Elmt := First (Component_Associations (N));
         Component_Loop : while Present (Elmt) loop

            Choice := First (Choices (Elmt));
            Choice_Loop : while Present (Choice) loop

               --  If we have an others choice, fill in the missing elements
               --  subject to the limit established by Max_Others_Replicate.

               if Nkind (Choice) = N_Others_Choice then
                  Rep_Count := 0;

                  for J in Vals'Range loop
                     if No (Vals (J)) then
                        Vals (J) := New_Copy_Tree (Expression (Elmt));
                        Rep_Count := Rep_Count + 1;

                        --  Check for maximum others replication. Note that
                        --  we skip this test if either of the restrictions
                        --  No_Elaboration_Code or No_Implicit_Loops is
                        --  active, or if this is a preelaborable unit.

                        if Rep_Count > Max_Others_Replicate
                          and then not Restrictions (No_Elaboration_Code)
                          and then not Restrictions (No_Implicit_Loops)
                          and then not
                            Is_Preelaborated (Cunit_Entity (Current_Sem_Unit))
                        then
                           return;
                        end if;
                     end if;
                  end loop;

                  exit Component_Loop;

               --  Case of a subtype mark

               elsif (Nkind (Choice) = N_Identifier
                       and then Is_Type (Entity (Choice)))
               then
                  Lo := Type_Low_Bound  (Etype (Choice));
                  Hi := Type_High_Bound (Etype (Choice));

               --  Case of subtype indication

               elsif Nkind (Choice) = N_Subtype_Indication then
                  Lo := Low_Bound  (Range_Expression (Constraint (Choice)));
                  Hi := High_Bound (Range_Expression (Constraint (Choice)));

               --  Case of a range

               elsif Nkind (Choice) = N_Range then
                  Lo := Low_Bound (Choice);
                  Hi := High_Bound (Choice);

               --  Normal subexpression case

               else pragma Assert (Nkind (Choice) in N_Subexpr);
                  if not Compile_Time_Known_Value (Choice) then
                     return;

                  else
                     Vals (UI_To_Int (Expr_Value (Choice))) :=
                       New_Copy_Tree (Expression (Elmt));
                     goto Continue;
                  end if;
               end if;

               --  Range cases merge with Lo,Hi said

               if not Compile_Time_Known_Value (Lo)
                    or else
                  not Compile_Time_Known_Value (Hi)
               then
                  return;
               else
                  for J in UI_To_Int (Expr_Value (Lo)) ..
                           UI_To_Int (Expr_Value (Hi))
                  loop
                     Vals (J) := New_Copy_Tree (Expression (Elmt));
                  end loop;
               end if;

            <<Continue>>
               Next (Choice);
            end loop Choice_Loop;

            Next (Elmt);
         end loop Component_Loop;

         --  If we get here the conversion is possible

         Vlist := New_List;
         for J in Vals'Range loop
            Append (Vals (J), Vlist);
         end loop;

         Rewrite (N, Make_Aggregate (Loc, Expressions => Vlist));
         Analyze_And_Resolve (N, Typ);
      end;
   end Convert_To_Positional;

   ----------------------------
   -- Expand_Array_Aggregate --
   ----------------------------

   --  Array aggregate expansion proceeds as follows:

   --  1. If requested we generate code to perform all the array aggregate
   --     bound checks, specifically

   --         (a) Check that the index range defined by aggregate bounds is
   --             compatible with corresponding index subtype.

   --         (b) If an others choice is present check that no aggregate
   --             index is outside the bounds of the index constraint.

   --         (c) For multidimensional arrays make sure that all subaggregates
   --             corresponding to the same dimension have the same bounds.

   --  2. Check if the aggregate can be statically processed. If this is the
   --     case pass it as is to Gigi. Note that a necessary condition for
   --     static processing is that the aggregate be fully positional.

   --  3. If in place aggregate expansion is possible (i.e. no need to create
   --     a temporary) then mark the aggregate as such and return. Otherwise
   --     create a new temporary and generate the appropriate initialization
   --     code.

   procedure Expand_Array_Aggregate (N : Node_Id) is
      Loc : constant Source_Ptr := Sloc (N);

      Typ  : constant Entity_Id := Etype (N);
      Ctyp : constant Entity_Id := Component_Type (Typ);
      --  Typ is the correct constrained array subtype of the aggregate
      --  Ctyp is the corresponding component type.

      Aggr_Dimension : constant Pos := Number_Dimensions (Typ);
      --  Number of aggregate index dimensions.

      Aggr_Low  : array (1 .. Aggr_Dimension) of Node_Id;
      Aggr_High : array (1 .. Aggr_Dimension) of Node_Id;
      --  Low and High bounds of the constraint for each aggregate index.

      Aggr_Index_Typ : array (1 .. Aggr_Dimension) of Entity_Id;
      --  The type of each index.

      Maybe_In_Place_OK : Boolean;
      --  If the type is neither controlled nor packed and the aggregate
      --  is the expression in an assignment, assignment in place may be
      --  possible, provided other conditions are met on the LHS.

      Others_Present : array (1 .. Aggr_Dimension) of Boolean :=
                         (others => False);
      --  If Others_Present (J) is True, then there is an others choice
      --  in one of the sub-aggregates of N at dimension J.

      procedure Build_Constrained_Type (Positional : Boolean);
      --  If the subtype is not static or unconstrained, build a constrained
      --  type using the computable sizes of the aggregate and its sub-
      --  aggregates.

      procedure Check_Bounds (Aggr_Bounds : Node_Id; Index_Bounds : Node_Id);
      --  Checks that the bounds of Aggr_Bounds are within the bounds defined
      --  by Index_Bounds.

      procedure Check_Same_Aggr_Bounds (Sub_Aggr : Node_Id; Dim : Pos);
      --  Checks that in a multi-dimensional array aggregate all subaggregates
      --  corresponding to the same dimension have the same bounds.
      --  Sub_Aggr is an array sub-aggregate. Dim is the dimension
      --  corresponding to the sub-aggregate.

      procedure Compute_Others_Present (Sub_Aggr : Node_Id; Dim : Pos);
      --  Computes the values of array Others_Present. Sub_Aggr is the
      --  array sub-aggregate we start the computation from. Dim is the
      --  dimension corresponding to the sub-aggregate.

      function Has_Address_Clause (D : Node_Id) return Boolean;
      --  If the aggregate is the expression in an object declaration, it
      --  cannot be expanded in place. This function does a lookahead in the
      --  current declarative part to find an address clause for the object
      --  being declared.

      function In_Place_Assign_OK return Boolean;
      --  Simple predicate to determine whether an aggregate assignment can
      --  be done in place, because none of the new values can depend on the
      --  components of the target of the assignment.

      procedure Others_Check (Sub_Aggr : Node_Id; Dim : Pos);
      --  Checks that if an others choice is present in any sub-aggregate no
      --  aggregate index is outside the bounds of the index constraint.
      --  Sub_Aggr is an array sub-aggregate. Dim is the dimension
      --  corresponding to the sub-aggregate.

      ----------------------------
      -- Build_Constrained_Type --
      ----------------------------

      procedure Build_Constrained_Type (Positional : Boolean) is
         Loc        : constant Source_Ptr := Sloc (N);
         Agg_Type   : Entity_Id;
         Comp       : Node_Id;
         Decl       : Node_Id;
         Typ        : constant Entity_Id := Etype (N);
         Indices    : List_Id := New_List;
         Num        : Int;
         Sub_Agg    : Node_Id;

      begin
         Agg_Type :=
           Make_Defining_Identifier (
             Loc, New_Internal_Name ('A'));

         --  If the aggregate is purely positional, all its subaggregates
         --  have the same size. We collect the dimensions from the first
         --  subaggregate at each level.

         if Positional then
            Sub_Agg := N;

            for D in 1 .. Number_Dimensions (Typ) loop
               Comp := First (Expressions (Sub_Agg));

               Sub_Agg := Comp;
               Num := 0;

               while Present (Comp) loop
                  Num := Num + 1;
                  Next (Comp);
               end loop;

               Append (
                 Make_Range (Loc,
                   Low_Bound => Make_Integer_Literal (Loc, 1),
                   High_Bound =>
                          Make_Integer_Literal (Loc, Num)),
                 Indices);
            end loop;

         else

            --  We know the aggregate type is unconstrained and the
            --  aggregate is not processable by the back end, therefore
            --  not necessarily positional. Retrieve the bounds of each
            --  dimension as computed earlier.

            for D in 1 .. Number_Dimensions (Typ) loop
               Append (
                 Make_Range (Loc,
                    Low_Bound  => Aggr_Low  (D),
                    High_Bound => Aggr_High (D)),
                 Indices);
            end loop;
         end if;

         Decl :=
           Make_Full_Type_Declaration (Loc,
               Defining_Identifier => Agg_Type,
               Type_Definition =>
                 Make_Constrained_Array_Definition (Loc,
                   Discrete_Subtype_Definitions => Indices,
                   Subtype_Indication =>
                     New_Occurrence_Of (Component_Type (Typ), Loc)));

         Insert_Action (N, Decl);
         Analyze (Decl);
         Set_Etype (N, Agg_Type);
         Set_Is_Itype (Agg_Type);
         Freeze_Itype (Agg_Type, N);
      end Build_Constrained_Type;

      ------------------
      -- Check_Bounds --
      ------------------

      procedure Check_Bounds (Aggr_Bounds : Node_Id; Index_Bounds : Node_Id) is
         Aggr_Lo : Node_Id;
         Aggr_Hi : Node_Id;

         Ind_Lo  : Node_Id;
         Ind_Hi  : Node_Id;

         Cond    : Node_Id := Empty;

      begin
         Get_Index_Bounds (Aggr_Bounds, Aggr_Lo, Aggr_Hi);
         Get_Index_Bounds (Index_Bounds, Ind_Lo, Ind_Hi);

         --  Generate the following test:
         --
         --    [constraint_error when
         --      Aggr_Lo <= Aggr_Hi and then
         --        (Aggr_Lo < Ind_Lo or else Aggr_Hi > Ind_Hi)]
         --
         --  As an optimization try to see if some tests are trivially vacuos
         --  because we are comparing an expression against itself.

         if Aggr_Lo = Ind_Lo and then Aggr_Hi = Ind_Hi then
            Cond := Empty;

         elsif Aggr_Hi = Ind_Hi then
            Cond :=
              Make_Op_Lt (Loc,
                Left_Opnd  => Duplicate_Subexpr (Aggr_Lo),
                Right_Opnd => Duplicate_Subexpr (Ind_Lo));

         elsif Aggr_Lo = Ind_Lo then
            Cond :=
              Make_Op_Gt (Loc,
                Left_Opnd  => Duplicate_Subexpr (Aggr_Hi),
                Right_Opnd => Duplicate_Subexpr (Ind_Hi));

         else
            Cond :=
              Make_Or_Else (Loc,
                Left_Opnd =>
                  Make_Op_Lt (Loc,
                    Left_Opnd  => Duplicate_Subexpr (Aggr_Lo),
                    Right_Opnd => Duplicate_Subexpr (Ind_Lo)),

                Right_Opnd =>
                  Make_Op_Gt (Loc,
                    Left_Opnd  => Duplicate_Subexpr (Aggr_Hi),
                    Right_Opnd => Duplicate_Subexpr (Ind_Hi)));
         end if;

         if Present (Cond) then
            Cond :=
              Make_And_Then (Loc,
                Left_Opnd =>
                  Make_Op_Le (Loc,
                    Left_Opnd  => Duplicate_Subexpr (Aggr_Lo),
                    Right_Opnd => Duplicate_Subexpr (Aggr_Hi)),

                Right_Opnd => Cond);

            Set_Analyzed (Left_Opnd  (Left_Opnd (Cond)), False);
            Set_Analyzed (Right_Opnd (Left_Opnd (Cond)), False);
            Insert_Action (N,
              Make_Raise_Constraint_Error (Loc,
                Condition => Cond,
                Reason    => CE_Length_Check_Failed));
         end if;
      end Check_Bounds;

      ----------------------------
      -- Check_Same_Aggr_Bounds --
      ----------------------------

      procedure Check_Same_Aggr_Bounds (Sub_Aggr : Node_Id; Dim : Pos) is
         Sub_Lo : constant Node_Id := Low_Bound (Aggregate_Bounds (Sub_Aggr));
         Sub_Hi : constant Node_Id := High_Bound (Aggregate_Bounds (Sub_Aggr));
         --  The bounds of this specific sub-aggregate.

         Aggr_Lo : constant Node_Id := Aggr_Low (Dim);
         Aggr_Hi : constant Node_Id := Aggr_High (Dim);
         --  The bounds of the aggregate for this dimension

         Ind_Typ : constant Entity_Id := Aggr_Index_Typ (Dim);
         --  The index type for this dimension.

         Cond : Node_Id := Empty;

         Assoc  : Node_Id;
         Expr   : Node_Id;

      begin
         --  If index checks are on generate the test
         --
         --    [constraint_error when
         --      Aggr_Lo /= Sub_Lo or else Aggr_Hi /= Sub_Hi]
         --
         --  As an optimization try to see if some tests are trivially vacuos
         --  because we are comparing an expression against itself. Also for
         --  the first dimension the test is trivially vacuous because there
         --  is just one aggregate for dimension 1.

         if Index_Checks_Suppressed (Ind_Typ) then
            Cond := Empty;

         elsif Dim = 1
           or else (Aggr_Lo = Sub_Lo and then Aggr_Hi = Sub_Hi)
         then
            Cond := Empty;

         elsif Aggr_Hi = Sub_Hi then
            Cond :=
              Make_Op_Ne (Loc,
                Left_Opnd  => Duplicate_Subexpr (Aggr_Lo),
                Right_Opnd => Duplicate_Subexpr (Sub_Lo));

         elsif Aggr_Lo = Sub_Lo then
            Cond :=
              Make_Op_Ne (Loc,
                Left_Opnd  => Duplicate_Subexpr (Aggr_Hi),
                Right_Opnd => Duplicate_Subexpr (Sub_Hi));

         else
            Cond :=
              Make_Or_Else (Loc,
                Left_Opnd =>
                  Make_Op_Ne (Loc,
                    Left_Opnd  => Duplicate_Subexpr (Aggr_Lo),
                    Right_Opnd => Duplicate_Subexpr (Sub_Lo)),

                Right_Opnd =>
                  Make_Op_Ne (Loc,
                    Left_Opnd  => Duplicate_Subexpr (Aggr_Hi),
                    Right_Opnd => Duplicate_Subexpr (Sub_Hi)));
         end if;

         if Present (Cond) then
            Insert_Action (N,
              Make_Raise_Constraint_Error (Loc,
                Condition => Cond,
                Reason    => CE_Length_Check_Failed));
         end if;

         --  Now look inside the sub-aggregate to see if there is more work

         if Dim < Aggr_Dimension then

            --  Process positional components

            if Present (Expressions (Sub_Aggr)) then
               Expr := First (Expressions (Sub_Aggr));
               while Present (Expr) loop
                  Check_Same_Aggr_Bounds (Expr, Dim + 1);
                  Next (Expr);
               end loop;
            end if;

            --  Process component associations

            if Present (Component_Associations (Sub_Aggr)) then
               Assoc := First (Component_Associations (Sub_Aggr));
               while Present (Assoc) loop
                  Expr := Expression (Assoc);
                  Check_Same_Aggr_Bounds (Expr, Dim + 1);
                  Next (Assoc);
               end loop;
            end if;
         end if;
      end Check_Same_Aggr_Bounds;

      ----------------------------
      -- Compute_Others_Present --
      ----------------------------

      procedure Compute_Others_Present (Sub_Aggr : Node_Id; Dim : Pos) is
         Assoc  : Node_Id;
         Expr   : Node_Id;

      begin
         if Present (Component_Associations (Sub_Aggr)) then
            Assoc := Last (Component_Associations (Sub_Aggr));

            if Nkind (First (Choices (Assoc))) = N_Others_Choice then
               Others_Present (Dim) := True;
            end if;
         end if;

         --  Now look inside the sub-aggregate to see if there is more work

         if Dim < Aggr_Dimension then

            --  Process positional components

            if Present (Expressions (Sub_Aggr)) then
               Expr := First (Expressions (Sub_Aggr));
               while Present (Expr) loop
                  Compute_Others_Present (Expr, Dim + 1);
                  Next (Expr);
               end loop;
            end if;

            --  Process component associations

            if Present (Component_Associations (Sub_Aggr)) then
               Assoc := First (Component_Associations (Sub_Aggr));
               while Present (Assoc) loop
                  Expr := Expression (Assoc);
                  Compute_Others_Present (Expr, Dim + 1);
                  Next (Assoc);
               end loop;
            end if;
         end if;
      end Compute_Others_Present;

      -------------------------
      --  Has_Address_Clause --
      -------------------------

      function Has_Address_Clause (D : Node_Id) return Boolean is
         Id   : Entity_Id := Defining_Identifier (D);
         Decl : Node_Id := Next (D);

      begin
         while Present (Decl) loop

            if Nkind (Decl) = N_At_Clause
               and then Chars (Identifier (Decl)) = Chars (Id)
            then
               return True;

            elsif Nkind (Decl) = N_Attribute_Definition_Clause
               and then Chars (Decl) = Name_Address
               and then Chars (Name (Decl)) = Chars (Id)
            then
               return True;
            end if;

            Next (Decl);
         end loop;

         return False;
      end Has_Address_Clause;

      ------------------------
      -- In_Place_Assign_OK --
      ------------------------

      function In_Place_Assign_OK return Boolean is
         Aggr_In : Node_Id;
         Aggr_Lo : Node_Id;
         Aggr_Hi : Node_Id;
         Obj_In  : Node_Id;
         Obj_Lo  : Node_Id;
         Obj_Hi  : Node_Id;

         function Is_Others_Aggregate (Aggr : Node_Id) return Boolean;
         --   Aggregates that consist of a single Others choice are safe
         --  if the single expression is.

         function Safe_Aggregate (Aggr : Node_Id) return Boolean;
         --  Check recursively that each component of a (sub)aggregate does
         --  not depend on the variable being assigned to.

         function Safe_Component (Expr : Node_Id) return Boolean;
         --  Verify that an expression cannot depend on the variable being
         --  assigned to. Room for improvement here (but less than before).

         -------------------------
         -- Is_Others_Aggregate --
         -------------------------

         function Is_Others_Aggregate (Aggr : Node_Id) return Boolean is
         begin
            return No (Expressions (Aggr))
              and then Nkind
                (First (Choices (First (Component_Associations (Aggr)))))
                  = N_Others_Choice;
         end Is_Others_Aggregate;

         --------------------
         -- Safe_Aggregate --
         --------------------

         function Safe_Aggregate (Aggr : Node_Id) return Boolean is
            Expr : Node_Id;

         begin
            if Present (Expressions (Aggr)) then
               Expr := First (Expressions (Aggr));

               while Present (Expr) loop
                  if Nkind (Expr) = N_Aggregate then
                     if not Safe_Aggregate (Expr) then
                        return False;
                     end if;

                  elsif not Safe_Component (Expr) then
                     return False;
                  end if;

                  Next (Expr);
               end loop;
            end if;

            if Present (Component_Associations (Aggr)) then
               Expr := First (Component_Associations (Aggr));

               while Present (Expr) loop
                  if Nkind (Expression (Expr)) = N_Aggregate then
                     if not Safe_Aggregate (Expression (Expr)) then
                        return False;
                     end if;

                  elsif not Safe_Component (Expression (Expr)) then
                     return False;
                  end if;

                  Next (Expr);
               end loop;
            end if;

            return True;
         end Safe_Aggregate;

         --------------------
         -- Safe_Component --
         --------------------

         function Safe_Component (Expr : Node_Id) return Boolean is
            Comp : Node_Id := Expr;

            function Check_Component (Comp : Node_Id) return Boolean;
            --  Do the recursive traversal, after copy.

            function Check_Component (Comp : Node_Id) return Boolean is
            begin
               if Is_Overloaded (Comp) then
                  return False;
               end if;

               return Compile_Time_Known_Value (Comp)

                 or else (Is_Entity_Name (Comp)
                           and then  Present (Entity (Comp))
                           and then No (Renamed_Object (Entity (Comp))))

                 or else (Nkind (Comp) = N_Attribute_Reference
                           and then Check_Component (Prefix (Comp)))

                 or else (Nkind (Comp) in N_Binary_Op
                           and then Check_Component (Left_Opnd  (Comp))
                           and then Check_Component (Right_Opnd (Comp)))

                 or else (Nkind (Comp) in N_Unary_Op
                           and then Check_Component (Right_Opnd (Comp)))

                 or else (Nkind (Comp) = N_Selected_Component
                           and then Check_Component (Prefix (Comp)));
            end Check_Component;

            --  Start of processing for Safe_Component

         begin
            --  If the component appears in an association that may
            --  correspond to more than one element, it is not analyzed
            --  before the expansion into assignments, to avoid side effects.
            --  We analyze, but do not resolve the copy, to obtain sufficient
            --  entity information for the checks that follow. If component is
            --  overloaded we assume an unsafe function call.

            if not Analyzed (Comp) then
               if Is_Overloaded (Expr) then
                  return False;

               elsif Nkind (Expr) = N_Aggregate
                  and then not Is_Others_Aggregate (Expr)
               then
                  return False;

               elsif Nkind (Expr) = N_Allocator then
                  --  For now, too complex to analyze.

                  return False;
               end if;

               Comp := New_Copy_Tree (Expr);
               Set_Parent (Comp, Parent (Expr));
               Analyze (Comp);
            end if;

            if Nkind (Comp) = N_Aggregate then
               return Safe_Aggregate (Comp);
            else
               return Check_Component (Comp);
            end if;
         end Safe_Component;

      --  Start of processing for In_Place_Assign_OK

      begin
         if Present (Component_Associations (N)) then

            --  On assignment, sliding can take place, so we cannot do the
            --  assignment in place unless the bounds of the aggregate are
            --  statically equal to those of the target.

            --  If the aggregate is given by an others choice, the bounds
            --  are derived from the left-hand side, and the assignment is
            --  safe if the expression is.

            if Is_Others_Aggregate (N) then
               return
                 Safe_Component
                  (Expression (First (Component_Associations (N))));
            end if;

            Aggr_In := First_Index (Etype (N));
            Obj_In  := First_Index (Etype (Name (Parent (N))));

            while Present (Aggr_In) loop
               Get_Index_Bounds (Aggr_In, Aggr_Lo, Aggr_Hi);
               Get_Index_Bounds (Obj_In, Obj_Lo, Obj_Hi);

               if not Compile_Time_Known_Value (Aggr_Lo)
                 or else not Compile_Time_Known_Value (Aggr_Hi)
                 or else not Compile_Time_Known_Value (Obj_Lo)
                 or else not Compile_Time_Known_Value (Obj_Hi)
                 or else Expr_Value (Aggr_Lo) /= Expr_Value (Obj_Lo)
                 or else Expr_Value (Aggr_Hi) /= Expr_Value (Obj_Hi)
               then
                  return False;
               end if;

               Next_Index (Aggr_In);
               Next_Index (Obj_In);
            end loop;
         end if;

         --  Now check the component values themselves.

         return Safe_Aggregate (N);
      end In_Place_Assign_OK;

      ------------------
      -- Others_Check --
      ------------------

      procedure Others_Check (Sub_Aggr : Node_Id; Dim : Pos) is
         Aggr_Lo : constant Node_Id := Aggr_Low (Dim);
         Aggr_Hi : constant Node_Id := Aggr_High (Dim);
         --  The bounds of the aggregate for this dimension.

         Ind_Typ : constant Entity_Id := Aggr_Index_Typ (Dim);
         --  The index type for this dimension.

         Need_To_Check : Boolean := False;

         Choices_Lo : Node_Id := Empty;
         Choices_Hi : Node_Id := Empty;
         --  The lowest and highest discrete choices for a named sub-aggregate

         Nb_Choices : Int := -1;
         --  The number of discrete non-others choices in this sub-aggregate

         Nb_Elements : Uint := Uint_0;
         --  The number of elements in a positional aggregate

         Cond : Node_Id := Empty;

         Assoc  : Node_Id;
         Choice : Node_Id;
         Expr   : Node_Id;

      begin
         --  Check if we have an others choice. If we do make sure that this
         --  sub-aggregate contains at least one element in addition to the
         --  others choice.

         if Range_Checks_Suppressed (Ind_Typ) then
            Need_To_Check := False;

         elsif Present (Expressions (Sub_Aggr))
           and then Present (Component_Associations (Sub_Aggr))
         then
            Need_To_Check := True;

         elsif Present (Component_Associations (Sub_Aggr)) then
            Assoc := Last (Component_Associations (Sub_Aggr));

            if Nkind (First (Choices (Assoc))) /= N_Others_Choice then
               Need_To_Check := False;

            else
               --  Count the number of discrete choices. Start with -1
               --  because the others choice does not count.

               Nb_Choices := -1;
               Assoc := First (Component_Associations (Sub_Aggr));
               while Present (Assoc) loop
                  Choice := First (Choices (Assoc));
                  while Present (Choice) loop
                     Nb_Choices := Nb_Choices + 1;
                     Next (Choice);
                  end loop;

                  Next (Assoc);
               end loop;

               --  If there is only an others choice nothing to do

               Need_To_Check := (Nb_Choices > 0);
            end if;

         else
            Need_To_Check := False;
         end if;

         --  If we are dealing with a positional sub-aggregate with an
         --  others choice then compute the number or positional elements.

         if Need_To_Check and then Present (Expressions (Sub_Aggr)) then
            Expr := First (Expressions (Sub_Aggr));
            Nb_Elements := Uint_0;
            while Present (Expr) loop
               Nb_Elements := Nb_Elements + 1;
               Next (Expr);
            end loop;

         --  If the aggregate contains discrete choices and an others choice
         --  compute the smallest and largest discrete choice values.

         elsif Need_To_Check then
            Compute_Choices_Lo_And_Choices_Hi : declare

               Table : Case_Table_Type (1 .. Nb_Choices);
               --  Used to sort all the different choice values

               J    : Pos := 1;
               Low  : Node_Id;
               High : Node_Id;

            begin
               Assoc := First (Component_Associations (Sub_Aggr));
               while Present (Assoc) loop
                  Choice := First (Choices (Assoc));
                  while Present (Choice) loop
                     if Nkind (Choice) = N_Others_Choice then
                        exit;
                     end if;

                     Get_Index_Bounds (Choice, Low, High);
                     Table (J).Choice_Lo := Low;
                     Table (J).Choice_Hi := High;

                     J := J + 1;
                     Next (Choice);
                  end loop;

                  Next (Assoc);
               end loop;

               --  Sort the discrete choices

               Sort_Case_Table (Table);

               Choices_Lo := Table (1).Choice_Lo;
               Choices_Hi := Table (Nb_Choices).Choice_Hi;
            end Compute_Choices_Lo_And_Choices_Hi;
         end if;

         --  If no others choice in this sub-aggregate, or the aggregate
         --  comprises only an others choice, nothing to do.

         if not Need_To_Check then
            Cond := Empty;

         --  If we are dealing with an aggregate containing an others
         --  choice and positional components, we generate the following test:
         --
         --    if Ind_Typ'Pos (Aggr_Lo) + (Nb_Elements - 1) >
         --            Ind_Typ'Pos (Aggr_Hi)
         --    then
         --       raise Constraint_Error;
         --    end if;

         elsif Nb_Elements > Uint_0 then
            Cond :=
              Make_Op_Gt (Loc,
                Left_Opnd  =>
                  Make_Op_Add (Loc,
                    Left_Opnd  =>
                      Make_Attribute_Reference (Loc,
                        Prefix         => New_Reference_To (Ind_Typ, Loc),
                        Attribute_Name => Name_Pos,
                        Expressions    =>
                          New_List (Duplicate_Subexpr (Aggr_Lo))),
                    Right_Opnd => Make_Integer_Literal (Loc, Nb_Elements - 1)),

                Right_Opnd =>
                  Make_Attribute_Reference (Loc,
                    Prefix         => New_Reference_To (Ind_Typ, Loc),
                    Attribute_Name => Name_Pos,
                    Expressions    => New_List (Duplicate_Subexpr (Aggr_Hi))));

         --  If we are dealing with an aggregate containing an others
         --  choice and discrete choices we generate the following test:
         --
         --    [constraint_error when
         --      Choices_Lo < Aggr_Lo or else Choices_Hi > Aggr_Hi];

         else
            Cond :=
              Make_Or_Else (Loc,
                Left_Opnd =>
                  Make_Op_Lt (Loc,
                    Left_Opnd  => Duplicate_Subexpr (Choices_Lo),
                    Right_Opnd => Duplicate_Subexpr (Aggr_Lo)),

                Right_Opnd =>
                  Make_Op_Gt (Loc,
                    Left_Opnd  => Duplicate_Subexpr (Choices_Hi),
                    Right_Opnd => Duplicate_Subexpr (Aggr_Hi)));
         end if;

         if Present (Cond) then
            Insert_Action (N,
              Make_Raise_Constraint_Error (Loc,
                Condition => Cond,
                Reason    => CE_Length_Check_Failed));
         end if;

         --  Now look inside the sub-aggregate to see if there is more work

         if Dim < Aggr_Dimension then

            --  Process positional components

            if Present (Expressions (Sub_Aggr)) then
               Expr := First (Expressions (Sub_Aggr));
               while Present (Expr) loop
                  Others_Check (Expr, Dim + 1);
                  Next (Expr);
               end loop;
            end if;

            --  Process component associations

            if Present (Component_Associations (Sub_Aggr)) then
               Assoc := First (Component_Associations (Sub_Aggr));
               while Present (Assoc) loop
                  Expr := Expression (Assoc);
                  Others_Check (Expr, Dim + 1);
                  Next (Assoc);
               end loop;
            end if;
         end if;
      end Others_Check;

      --  Remaining Expand_Array_Aggregate variables

      Tmp : Entity_Id;
      --  Holds the temporary aggregate value.

      Tmp_Decl : Node_Id;
      --  Holds the declaration of Tmp.

      Aggr_Code   : List_Id;
      Parent_Node : Node_Id;
      Parent_Kind : Node_Kind;

   --  Start of processing for Expand_Array_Aggregate

   begin
      --  Do not touch the special aggregates of attributes used for Asm calls

      if Is_RTE (Ctyp, RE_Asm_Input_Operand)
        or else Is_RTE (Ctyp, RE_Asm_Output_Operand)
      then
         return;
      end if;

      --  If the semantic analyzer has determined that aggregate N will raise
      --  Constraint_Error at run-time, then the aggregate node has been
      --  replaced with an N_Raise_Constraint_Error node and we should
      --  never get here.

      pragma Assert (not Raises_Constraint_Error (N));

      --  STEP 1: Check (a)

      Index_Compatibility_Check : declare
         Aggr_Index_Range : Node_Id := First_Index (Typ);
         --  The current aggregate index range

         Index_Constraint : Node_Id := First_Index (Etype (Typ));
         --  The corresponding index constraint against which we have to
         --  check the above aggregate index range.

      begin
         Compute_Others_Present (N, 1);

         for J in 1 .. Aggr_Dimension loop
            --  There is no need to emit a check if an others choice is
            --  present for this array aggregate dimension since in this
            --  case one of N's sub-aggregates has taken its bounds from the
            --  context and these bounds must have been checked already. In
            --  addition all sub-aggregates corresponding to the same
            --  dimension must all have the same bounds (checked in (c) below).

            if not Range_Checks_Suppressed (Etype (Index_Constraint))
              and then not Others_Present (J)
            then
               --  We don't use Checks.Apply_Range_Check here because it
               --  emits a spurious check. Namely it checks that the range
               --  defined by the aggregate bounds is non empty. But we know
               --  this already if we get here.

               Check_Bounds (Aggr_Index_Range, Index_Constraint);
            end if;

            --  Save the low and high bounds of the aggregate index as well
            --  as the index type for later use in checks (b) and (c) below.

            Aggr_Low  (J) := Low_Bound (Aggr_Index_Range);
            Aggr_High (J) := High_Bound (Aggr_Index_Range);

            Aggr_Index_Typ (J) := Etype (Index_Constraint);

            Next_Index (Aggr_Index_Range);
            Next_Index (Index_Constraint);
         end loop;
      end Index_Compatibility_Check;

      --  STEP 1: Check (b)

      Others_Check (N, 1);

      --  STEP 1: Check (c)

      if Aggr_Dimension > 1 then
         Check_Same_Aggr_Bounds (N, 1);
      end if;

      --  STEP 2.

      --  First try to convert to positional form. If the result is not
      --  an aggregate any more, then we are done with the analysis (it
      --  it could be a string literal or an identifier for a temporary
      --  variable following this call). If result is an analyzed aggregate
      --  the transformation was also successful and we are done as well.

      Convert_To_Positional (N);

      if Nkind (N) /= N_Aggregate then
         return;

      elsif Analyzed (N)
        and then N /= Original_Node (N)
      then
         return;
      end if;

      if Backend_Processing_Possible (N) then

         --  If the aggregate is static but the constraints are not, build
         --  a static subtype for the aggregate, so that Gigi can place it
         --  in static memory. Perform an unchecked_conversion to the non-
         --  static type imposed by the context.

         declare
            Itype      : constant Entity_Id := Etype (N);
            Index      : Node_Id;
            Needs_Type : Boolean := False;

         begin
            Index := First_Index (Itype);

            while Present (Index) loop
               if not Is_Static_Subtype (Etype (Index)) then
                  Needs_Type := True;
                  exit;
               else
                  Next_Index (Index);
               end if;
            end loop;

            if Needs_Type then
               Build_Constrained_Type (Positional => True);
               Rewrite (N, Unchecked_Convert_To (Itype, N));
               Analyze (N);
            end if;
         end;

         return;
      end if;

      --  Delay expansion for nested aggregates it will be taken care of
      --  when the parent aggregate is expanded

      Parent_Node := Parent (N);
      Parent_Kind := Nkind (Parent_Node);

      if Parent_Kind = N_Qualified_Expression then
         Parent_Node := Parent (Parent_Node);
         Parent_Kind := Nkind (Parent_Node);
      end if;

      if Parent_Kind = N_Aggregate
        or else Parent_Kind = N_Extension_Aggregate
        or else Parent_Kind = N_Component_Association
        or else (Parent_Kind = N_Object_Declaration
                  and then Controlled_Type (Typ))
        or else (Parent_Kind = N_Assignment_Statement
                  and then Inside_Init_Proc)
      then
         Set_Expansion_Delayed (N);
         return;
      end if;

      --  STEP 3.

      --  Look if in place aggregate expansion is possible

      --  First case to test for is packed array aggregate that we can
      --  handle at compile time. If so, return with transformation done.

      if Packed_Array_Aggregate_Handled (N) then
         return;
      end if;

      --  For object declarations we build the aggregate in place, unless
      --  the array is bit-packed or the component is controlled.

      --  For assignments we do the assignment in place if all the component
      --  associations have compile-time known values. For other cases we
      --  create a temporary. The analysis for safety of on-line assignment
      --  is delicate, i.e. we don't know how to do it fully yet ???

      if Requires_Transient_Scope (Typ) then
         Establish_Transient_Scope
           (N, Sec_Stack => Has_Controlled_Component (Typ));
      end if;

      Maybe_In_Place_OK :=
        Comes_From_Source (N)
          and then Nkind (Parent (N)) = N_Assignment_Statement
          and then not Is_Bit_Packed_Array (Typ)
          and then not Has_Controlled_Component (Typ)
          and then In_Place_Assign_OK;

      if Comes_From_Source (Parent (N))
         and then Nkind (Parent (N)) = N_Object_Declaration
         and then N = Expression (Parent (N))
         and then not Is_Bit_Packed_Array (Typ)
         and then not Has_Controlled_Component (Typ)
         and then not Has_Address_Clause (Parent (N))
      then
         Tmp := Defining_Identifier (Parent (N));
         Set_No_Initialization (Parent (N));
         Set_Expression (Parent (N), Empty);

         --  Set the type of the entity, for use in the analysis of the
         --  subsequent indexed assignments. If the nominal type is not
         --  constrained, build a subtype from the known bounds of the
         --  aggregate. If the declaration has a subtype mark, use it,
         --  otherwise use the itype of the aggregate.

         if not Is_Constrained (Typ) then
            Build_Constrained_Type (Positional => False);
         elsif Is_Entity_Name (Object_Definition (Parent (N)))
           and then Is_Constrained (Entity (Object_Definition (Parent (N))))
         then
            Set_Etype (Tmp, Entity (Object_Definition (Parent (N))));
         else
            Set_Size_Known_At_Compile_Time (Typ, False);
            Set_Etype (Tmp, Typ);
         end if;

      elsif Maybe_In_Place_OK
        and then Is_Entity_Name (Name (Parent (N)))
      then
         Tmp := Entity (Name (Parent (N)));

         if Etype (Tmp) /= Etype (N) then
            Apply_Length_Check (N, Etype (Tmp));
         end if;

      elsif Maybe_In_Place_OK
        and then Nkind (Name (Parent (N))) = N_Explicit_Dereference
        and then Is_Entity_Name (Prefix (Name (Parent (N))))
      then
         Tmp := Name (Parent (N));

         if Etype (Tmp) /= Etype (N) then
            Apply_Length_Check (N, Etype (Tmp));
         end if;

      elsif Maybe_In_Place_OK
        and then Nkind (Name (Parent (N))) = N_Slice
        and then Safe_Slice_Assignment (N)
      then
         --  Safe_Slice_Assignment rewrites assignment as a loop

         return;

      else
         Maybe_In_Place_OK := False;
         Tmp := Make_Defining_Identifier (Loc, New_Internal_Name ('A'));
         Tmp_Decl :=
           Make_Object_Declaration
             (Loc,
              Defining_Identifier => Tmp,
              Object_Definition   => New_Occurrence_Of (Typ, Loc));
         Set_No_Initialization (Tmp_Decl, True);

         --  If we are within a loop, the temporary will be pushed on the
         --  stack at each iteration. If the aggregate is the expression for
         --  an allocator, it will be immediately copied to the heap and can
         --  be reclaimed at once. We create a transient scope around the
         --  aggregate for this purpose.

         if Ekind (Current_Scope) = E_Loop
           and then Nkind (Parent (Parent (N))) = N_Allocator
         then
            Establish_Transient_Scope (N, False);
         end if;

         Insert_Action (N, Tmp_Decl);
      end if;

      --  Construct and insert the aggregate code. We can safely suppress
      --  index checks because this code is guaranteed not to raise CE
      --  on index checks. However we should *not* suppress all checks.

      declare
         Target : Node_Id;

      begin
         if Nkind (Tmp) = N_Defining_Identifier then
            Target := New_Reference_To (Tmp, Loc);

         else
            --  Name in assignment is explicit dereference.

            Target := New_Copy (Tmp);
         end if;

         Aggr_Code :=
           Build_Array_Aggr_Code (N,
             Index       => First_Index (Typ),
             Into        => Target,
             Scalar_Comp => Is_Scalar_Type (Ctyp));
      end;

      if Comes_From_Source (Tmp) then
         Insert_Actions_After (Parent (N), Aggr_Code);

      else
         Insert_Actions (N, Aggr_Code);
      end if;

      --  If the aggregate has been assigned in place, remove the original
      --  assignment.

      if Nkind (Parent (N)) = N_Assignment_Statement
        and then Maybe_In_Place_OK
      then
         Rewrite (Parent (N), Make_Null_Statement (Loc));

      elsif Nkind (Parent (N)) /= N_Object_Declaration
        or else Tmp /= Defining_Identifier (Parent (N))
      then
         Rewrite (N, New_Occurrence_Of (Tmp, Loc));
         Analyze_And_Resolve (N, Typ);
      end if;
   end Expand_Array_Aggregate;

   ------------------------
   -- Expand_N_Aggregate --
   ------------------------

   procedure Expand_N_Aggregate (N : Node_Id) is
   begin
      if Is_Record_Type (Etype (N)) then
         Expand_Record_Aggregate (N);
      else
         Expand_Array_Aggregate (N);
      end if;
   end Expand_N_Aggregate;

   ----------------------------------
   -- Expand_N_Extension_Aggregate --
   ----------------------------------

   --  If the ancestor part is an expression, add a component association for
   --  the parent field. If the type of the ancestor part is not the direct
   --  parent of the expected type,  build recursively the needed ancestors.
   --  If the ancestor part is a subtype_mark, replace aggregate with a decla-
   --  ration for a temporary of the expected type, followed by individual
   --  assignments to the given components.

   procedure Expand_N_Extension_Aggregate (N : Node_Id) is
      Loc : constant Source_Ptr := Sloc  (N);
      A   : constant Node_Id    := Ancestor_Part (N);
      Typ : constant Entity_Id  := Etype (N);

   begin
      --  If the ancestor is a subtype mark, an init_proc must be called
      --  on the resulting object which thus has to be materialized in
      --  the front-end

      if Is_Entity_Name (A) and then Is_Type (Entity (A)) then
         Convert_To_Assignments (N, Typ);

      --  The extension aggregate is transformed into a record aggregate
      --  of the following form (c1 and c2 are inherited components)

      --   (Exp with c3 => a, c4 => b)
      --      ==> (c1 => Exp.c1, c2 => Exp.c2, c1 => a, c2 => b)

      else
         Set_Etype (N, Typ);

         --  No tag is needed in the case of Java_VM

         if Java_VM then
            Expand_Record_Aggregate (N,
              Parent_Expr => A);
         else
            Expand_Record_Aggregate (N,
              Orig_Tag    => New_Occurrence_Of (Access_Disp_Table (Typ), Loc),
              Parent_Expr => A);
         end if;
      end if;
   end Expand_N_Extension_Aggregate;

   -----------------------------
   -- Expand_Record_Aggregate --
   -----------------------------

   procedure Expand_Record_Aggregate
     (N           : Node_Id;
      Orig_Tag    : Node_Id := Empty;
      Parent_Expr : Node_Id := Empty)
   is
      Loc   : constant Source_Ptr   := Sloc  (N);
      Comps : constant List_Id      := Component_Associations (N);
      Typ   : constant Entity_Id    := Etype (N);
      Base_Typ : constant Entity_Id := Base_Type (Typ);

      function Has_Delayed_Nested_Aggregate_Or_Tagged_Comps return Boolean;
      --  Checks the presence of a nested aggregate which needs Late_Expansion
      --  or the presence of tagged components which may need tag adjustment.

      --------------------------------------------------
      -- Has_Delayed_Nested_Aggregate_Or_Tagged_Comps --
      --------------------------------------------------

      function Has_Delayed_Nested_Aggregate_Or_Tagged_Comps return Boolean is
         C     : Node_Id;
         Expr_Q : Node_Id;

      begin
         if No (Comps) then
            return False;
         end if;

         C := First (Comps);
         while Present (C) loop

            if Nkind (Expression (C)) = N_Qualified_Expression then
               Expr_Q := Expression (Expression (C));
            else
               Expr_Q := Expression (C);
            end if;

            --  Return true if the aggregate has any associations for
            --  tagged components that may require tag adjustment.
            --  These are cases where the source expression may have
            --  a tag that could differ from the component tag (e.g.,
            --  can occur for type conversions and formal parameters).
            --  (Tag adjustment is not needed if Java_VM because object
            --  tags are implicit in the JVM.)

            if Is_Tagged_Type (Etype (Expr_Q))
              and then (Nkind (Expr_Q) = N_Type_Conversion
                or else (Is_Entity_Name (Expr_Q)
                          and then Ekind (Entity (Expr_Q)) in Formal_Kind))
              and then not Java_VM
            then
               return True;
            end if;

            if Is_Delayed_Aggregate (Expr_Q) then
               return True;
            end if;

            Next (C);
         end loop;

         return False;
      end  Has_Delayed_Nested_Aggregate_Or_Tagged_Comps;

      --  Remaining Expand_Record_Aggregate variables

      Tag_Value : Node_Id;
      Comp      : Entity_Id;
      New_Comp  : Node_Id;

   --  Start of processing for Expand_Record_Aggregate

   begin
      --  Gigi doesn't handle properly temporaries of variable size
      --  so we generate it in the front-end

      if not Size_Known_At_Compile_Time (Typ) then
         Convert_To_Assignments (N, Typ);

      --  Temporaries for controlled aggregates need to be attached to a
      --  final chain in order to be properly finalized, so it has to
      --  be created in the front-end

      elsif Is_Controlled (Typ)
        or else Has_Controlled_Component (Base_Type (Typ))
      then
         Convert_To_Assignments (N, Typ);

      elsif Has_Delayed_Nested_Aggregate_Or_Tagged_Comps then
         Convert_To_Assignments (N, Typ);

      --  If an ancestor is private, some components are not inherited and
      --  we cannot expand into a record aggregate

      elsif Has_Private_Ancestor (Typ) then
         Convert_To_Assignments (N, Typ);

      --  ??? The following was done to compile fxacc00.ads in the ACVCs. Gigi
      --  is not able to handle the aggregate for Late_Request.

      elsif Is_Tagged_Type (Typ) and then Has_Discriminants (Typ) then
         Convert_To_Assignments (N, Typ);

      --  In all other cases we generate a proper aggregate that
      --  can be handled by gigi.

      else
         --  If no discriminants, nothing special to do

         if not Has_Discriminants (Typ) then
            null;

         --  Case of discriminants present

         elsif Is_Derived_Type (Typ) then

            --  For untagged types,  non-girder discriminants are replaced
            --  with girder discriminants, which are the ones that gigi uses
            --  to describe the type and its components.

            Generate_Aggregate_For_Derived_Type : declare
               First_Comp   : Node_Id;
               Discriminant : Entity_Id;
               Constraints  : List_Id := New_List;
               Decl         : Node_Id;
               Num_Disc     : Int := 0;
               Num_Gird     : Int := 0;

               procedure Prepend_Girder_Values (T : Entity_Id);
               --  Scan the list of girder discriminants of the type, and
               --  add their values to the aggregate being built.

               ---------------------------
               -- Prepend_Girder_Values --
               ---------------------------

               procedure Prepend_Girder_Values (T : Entity_Id) is
               begin
                  Discriminant := First_Girder_Discriminant (T);

                  while Present (Discriminant) loop
                     New_Comp :=
                       Make_Component_Association (Loc,
                         Choices    =>
                           New_List (New_Occurrence_Of (Discriminant, Loc)),

                         Expression =>
                           New_Copy_Tree (
                             Get_Discriminant_Value (
                                 Discriminant,
                                 Typ,
                                 Discriminant_Constraint (Typ))));

                     if No (First_Comp) then
                        Prepend_To (Component_Associations (N), New_Comp);
                     else
                        Insert_After (First_Comp, New_Comp);
                     end if;

                     First_Comp := New_Comp;
                     Next_Girder_Discriminant (Discriminant);
                  end loop;
               end Prepend_Girder_Values;

            --  Start of processing for Generate_Aggregate_For_Derived_Type

            begin
               --  Remove the associations for the  discriminant of
               --  the derived type.

               First_Comp := First (Component_Associations (N));

               while Present (First_Comp) loop
                  Comp := First_Comp;
                  Next (First_Comp);

                  if Ekind (Entity (First (Choices (Comp)))) =
                    E_Discriminant
                  then
                     Remove (Comp);
                     Num_Disc := Num_Disc + 1;
                  end if;
               end loop;

               --  Insert girder discriminant associations in the correct
               --  order. If there are more girder discriminants than new
               --  discriminants, there is at least one new discriminant
               --  that constrains more than one of the girders. In this
               --  case we need to construct a proper subtype of the parent
               --  type, in order to supply values to all the components.
               --  Otherwise there is one-one correspondence between the
               --  constraints and the girder discriminants.

               First_Comp := Empty;

               Discriminant := First_Girder_Discriminant (Base_Type (Typ));

               while Present (Discriminant) loop
                  Num_Gird := Num_Gird + 1;
                  Next_Girder_Discriminant (Discriminant);
               end loop;

               --  Case of more girder discriminants than new discriminants

               if Num_Gird > Num_Disc then

                  --  Create a proper subtype of the parent type, which is
                  --  the proper implementation type for the aggregate, and
                  --  convert it to the intended target type.

                  Discriminant := First_Girder_Discriminant (Base_Type (Typ));

                  while Present (Discriminant) loop
                     New_Comp :=
                       New_Copy_Tree (
                         Get_Discriminant_Value (
                             Discriminant,
                             Typ,
                             Discriminant_Constraint (Typ)));
                     Append (New_Comp, Constraints);
                     Next_Girder_Discriminant (Discriminant);
                  end loop;

                  Decl :=
                    Make_Subtype_Declaration (Loc,
                      Defining_Identifier =>
                         Make_Defining_Identifier (Loc,
                            New_Internal_Name ('T')),
                      Subtype_Indication =>
                        Make_Subtype_Indication (Loc,
                          Subtype_Mark =>
                            New_Occurrence_Of (Etype (Base_Type (Typ)), Loc),
                          Constraint =>
                            Make_Index_Or_Discriminant_Constraint
                              (Loc, Constraints)));

                  Insert_Action (N, Decl);
                  Prepend_Girder_Values (Base_Type (Typ));

                  Set_Etype (N, Defining_Identifier (Decl));
                  Set_Analyzed (N);

                  Rewrite (N, Unchecked_Convert_To (Typ, N));
                  Analyze (N);

               --  Case where we do not have fewer new discriminants than
               --  girder discriminants, so in this case we can simply
               --  use the girder discriminants of the subtype.

               else
                  Prepend_Girder_Values (Typ);
               end if;
            end Generate_Aggregate_For_Derived_Type;
         end if;

         if Is_Tagged_Type (Typ) then

            --  The tagged case, _parent and _tag component must be created.

            --  Reset null_present unconditionally. tagged records always have
            --  at least one field (the tag or the parent)

            Set_Null_Record_Present (N, False);

            --  When the current aggregate comes from the expansion of an
            --  extension aggregate, the parent expr is replaced by an
            --  aggregate formed by selected components of this expr

            if Present (Parent_Expr)
              and then Is_Empty_List (Comps)
            then
               Comp := First_Entity (Typ);
               while Present (Comp) loop

                  --  Skip all entities that aren't discriminants or components

                  if Ekind (Comp) /= E_Discriminant
                    and then Ekind (Comp) /= E_Component
                  then
                     null;

                  --  Skip all expander-generated components

                  elsif
                    not Comes_From_Source (Original_Record_Component (Comp))
                  then
                     null;

                  else
                     New_Comp :=
                       Make_Selected_Component (Loc,
                         Prefix =>
                           Unchecked_Convert_To (Typ,
                             Duplicate_Subexpr (Parent_Expr, True)),

                         Selector_Name => New_Occurrence_Of (Comp, Loc));

                     Append_To (Comps,
                       Make_Component_Association (Loc,
                         Choices    =>
                           New_List (New_Occurrence_Of (Comp, Loc)),
                         Expression =>
                           New_Comp));

                     Analyze_And_Resolve (New_Comp, Etype (Comp));
                  end if;

                  Next_Entity (Comp);
               end loop;
            end if;

            --  Compute the value for the Tag now, if the type is a root it
            --  will be included in the aggregate right away, otherwise it will
            --  be propagated to the parent aggregate

            if Present (Orig_Tag) then
               Tag_Value := Orig_Tag;
            elsif Java_VM then
               Tag_Value := Empty;
            else
               Tag_Value := New_Occurrence_Of (Access_Disp_Table (Typ), Loc);
            end if;

            --  For a derived type, an aggregate for the parent is formed with
            --  all the inherited components.

            if Is_Derived_Type (Typ) then

               declare
                  First_Comp   : Node_Id;
                  Parent_Comps : List_Id;
                  Parent_Aggr  : Node_Id;
                  Parent_Name  : Node_Id;

               begin
                  --  Remove the inherited component association from the
                  --  aggregate and store them in the parent aggregate

                  First_Comp := First (Component_Associations (N));
                  Parent_Comps := New_List;

                  while Present (First_Comp)
                    and then Scope (Original_Record_Component (
                            Entity (First (Choices (First_Comp))))) /= Base_Typ
                  loop
                     Comp := First_Comp;
                     Next (First_Comp);
                     Remove (Comp);
                     Append (Comp, Parent_Comps);
                  end loop;

                  Parent_Aggr := Make_Aggregate (Loc,
                    Component_Associations => Parent_Comps);
                  Set_Etype (Parent_Aggr, Etype (Base_Type (Typ)));

                  --  Find the _parent component

                  Comp := First_Component (Typ);
                  while Chars (Comp) /= Name_uParent loop
                     Comp := Next_Component (Comp);
                  end loop;

                  Parent_Name := New_Occurrence_Of (Comp, Loc);

                  --  Insert the parent aggregate

                  Prepend_To (Component_Associations (N),
                    Make_Component_Association (Loc,
                      Choices    => New_List (Parent_Name),
                      Expression => Parent_Aggr));

                  --  Expand recursively the parent propagating the right Tag

                  Expand_Record_Aggregate (
                    Parent_Aggr, Tag_Value, Parent_Expr);
               end;

            --  For a root type, the tag component is added (unless compiling
            --  for the Java VM, where tags are implicit).

            elsif not Java_VM then
               declare
                  Tag_Name  : constant Node_Id :=
                                New_Occurrence_Of (Tag_Component (Typ), Loc);
                  Typ_Tag   : constant Entity_Id := RTE (RE_Tag);
                  Conv_Node : constant Node_Id :=
                                Unchecked_Convert_To (Typ_Tag, Tag_Value);

               begin
                  Set_Etype (Conv_Node, Typ_Tag);
                  Prepend_To (Component_Associations (N),
                    Make_Component_Association (Loc,
                      Choices    => New_List (Tag_Name),
                      Expression => Conv_Node));
               end;
            end if;
         end if;
      end if;
   end Expand_Record_Aggregate;

   --------------------------
   -- Is_Delayed_Aggregate --
   --------------------------

   function Is_Delayed_Aggregate (N : Node_Id) return Boolean is
      Node : Node_Id := N;
      Kind : Node_Kind := Nkind (Node);
   begin
      if Kind = N_Qualified_Expression then
         Node := Expression (Node);
         Kind := Nkind (Node);
      end if;

      if Kind /= N_Aggregate and then Kind /= N_Extension_Aggregate then
         return False;
      else
         return Expansion_Delayed (Node);
      end if;
   end Is_Delayed_Aggregate;

   --------------------
   -- Late_Expansion --
   --------------------

   function Late_Expansion
     (N      : Node_Id;
      Typ    : Entity_Id;
      Target : Node_Id;
      Flist  : Node_Id := Empty;
      Obj    : Entity_Id := Empty)

      return   List_Id is

   begin
      if Is_Record_Type (Etype (N)) then
         return Build_Record_Aggr_Code (N, Typ, Target, Flist, Obj);
      else
         return
           Build_Array_Aggr_Code
             (N,
              First_Index (Typ),
              Target,
              Is_Scalar_Type (Component_Type (Typ)),
              No_List,
              Flist);
      end if;
   end Late_Expansion;

   ----------------------------------
   -- Make_OK_Assignment_Statement --
   ----------------------------------

   function Make_OK_Assignment_Statement
     (Sloc       : Source_Ptr;
      Name       : Node_Id;
      Expression : Node_Id)
      return       Node_Id
   is
   begin
      Set_Assignment_OK (Name);
      return Make_Assignment_Statement (Sloc, Name, Expression);
   end Make_OK_Assignment_Statement;

   -----------------------
   -- Number_Of_Choices --
   -----------------------

   function Number_Of_Choices (N : Node_Id) return Nat is
      Assoc  : Node_Id;
      Choice : Node_Id;

      Nb_Choices : Nat := 0;

   begin
      if Present (Expressions (N)) then
         return 0;
      end if;

      Assoc := First (Component_Associations (N));
      while Present (Assoc) loop

         Choice := First (Choices (Assoc));
         while Present (Choice) loop

            if Nkind (Choice) /= N_Others_Choice then
               Nb_Choices := Nb_Choices + 1;
            end if;

            Next (Choice);
         end loop;

         Next (Assoc);
      end loop;

      return Nb_Choices;
   end Number_Of_Choices;

   ------------------------------------
   -- Packed_Array_Aggregate_Handled --
   ------------------------------------

   --  The current version of this procedure will handle at compile time
   --  any array aggregate that meets these conditions:

   --    One dimensional, bit packed
   --    Underlying packed type is modular type
   --    Bounds are within 32-bit Int range
   --    All bounds and values are static

   function Packed_Array_Aggregate_Handled (N : Node_Id) return Boolean is
      Loc  : constant Source_Ptr := Sloc (N);
      Typ  : constant Entity_Id  := Etype (N);
      Ctyp : constant Entity_Id  := Component_Type (Typ);

      Not_Handled : exception;
      --  Exception raised if this aggregate cannot be handled

   begin
      --  For now, handle only one dimensional bit packed arrays

      if not Is_Bit_Packed_Array (Typ)
        or else Number_Dimensions (Typ) > 1
        or else not Is_Modular_Integer_Type (Packed_Array_Type (Typ))
      then
         return False;
      end if;

      declare
         Csiz  : constant Nat := UI_To_Int (Component_Size (Typ));

         Lo : Node_Id;
         Hi : Node_Id;
         --  Bounds of index type

         Lob : Uint;
         Hib : Uint;
         --  Values of bounds if compile time known

         function Get_Component_Val (N : Node_Id) return Uint;
         --  Given a expression value N of the component type Ctyp, returns
         --  A value of Csiz (component size) bits representing this value.
         --  If the value is non-static or any other reason exists why the
         --  value cannot be returned, then Not_Handled is raised.

         -----------------------
         -- Get_Component_Val --
         -----------------------

         function Get_Component_Val (N : Node_Id) return Uint is
            Val  : Uint;

         begin
            --  We have to analyze the expression here before doing any further
            --  processing here. The analysis of such expressions is deferred
            --  till expansion to prevent some problems of premature analysis.

            Analyze_And_Resolve (N, Ctyp);

            --  Must have a compile time value

            if not Compile_Time_Known_Value (N) then
               raise Not_Handled;
            end if;

            Val := Expr_Rep_Value (N);

            --  Adjust for bias, and strip proper number of bits

            if Has_Biased_Representation (Ctyp) then
               Val := Val - Expr_Value (Type_Low_Bound (Ctyp));
            end if;

            return Val mod Uint_2 ** Csiz;
         end Get_Component_Val;

      --  Here we know we have a one dimensional bit packed array

      begin
         Get_Index_Bounds (First_Index (Typ), Lo, Hi);

         --  Cannot do anything if bounds are dynamic

         if not Compile_Time_Known_Value (Lo)
              or else
            not Compile_Time_Known_Value (Hi)
         then
            return False;
         end if;

         --  Or are silly out of range of int bounds

         Lob := Expr_Value (Lo);
         Hib := Expr_Value (Hi);

         if not UI_Is_In_Int_Range (Lob)
              or else
            not UI_Is_In_Int_Range (Hib)
         then
            return False;
         end if;

         --  At this stage we have a suitable aggregate for handling
         --  at compile time (the only remaining checks, are that the
         --  values of expressions in the aggregate are compile time
         --  known (check performed by Get_Component_Val), and that
         --  any subtypes or ranges are statically known.

         --  If the aggregate is not fully positional at this stage,
         --  then convert it to positional form. Either this will fail,
         --  in which case we can do nothing, or it will succeed, in
         --  which case we have succeeded in handling the aggregate,
         --  or it will stay an aggregate, in which case we have failed
         --  to handle this case.

         if Present (Component_Associations (N)) then
            Convert_To_Positional
             (N, Max_Others_Replicate => 64, Handle_Bit_Packed => True);
            return Nkind (N) /= N_Aggregate;
         end if;

         --  Otherwise we are all positional, so convert to proper value

         declare
            Lov : constant Nat := UI_To_Int (Lob);
            Hiv : constant Nat := UI_To_Int (Hib);

            Len : constant Nat := Int'Max (0, Hiv - Lov + 1);
            --  The length of the array (number of elements)

            Aggregate_Val : Uint;
            --  Value of aggregate. The value is set in the low order
            --  bits of this value. For the little-endian case, the
            --  values are stored from low-order to high-order and
            --  for the big-endian case the values are stored from
            --  high-order to low-order. Note that gigi will take care
            --  of the conversions to left justify the value in the big
            --  endian case (because of left justified modular type
            --  processing), so we do not have to worry about that here.

            Lit : Node_Id;
            --  Integer literal for resulting constructed value

            Shift : Nat;
            --  Shift count from low order for next value

            Incr : Int;
            --  Shift increment for loop

            Expr : Node_Id;
            --  Next expression from positional parameters of aggregate

         begin
            --  For little endian, we fill up the low order bits of the
            --  target value. For big endian we fill up the high order
            --  bits of the target value (which is a left justified
            --  modular value).

            if Bytes_Big_Endian xor Debug_Flag_8 then
               Shift := Csiz * (Len - 1);
               Incr  := -Csiz;
            else
               Shift := 0;
               Incr  := +Csiz;
            end if;

            --  Loop to set the values

            Aggregate_Val := Uint_0;
            Expr := First (Expressions (N));
            for J in 1 .. Len loop
               Aggregate_Val :=
                 Aggregate_Val + Get_Component_Val (Expr) * Uint_2 ** Shift;
               Shift := Shift + Incr;
               Next (Expr);
            end loop;

            --  Now we can rewrite with the proper value

            Lit :=
              Make_Integer_Literal (Loc,
                Intval => Aggregate_Val);
            Set_Print_In_Hex (Lit);

            --  Construct the expression using this literal. Note that it is
            --  important to qualify the literal with its proper modular type
            --  since universal integer does not have the required range and
            --  also this is a left justified modular type, which is important
            --  in the big-endian case.

            Rewrite (N,
              Unchecked_Convert_To (Typ,
                Make_Qualified_Expression (Loc,
                  Subtype_Mark =>
                    New_Occurrence_Of (Packed_Array_Type (Typ), Loc),
                  Expression   => Lit)));

            Analyze_And_Resolve (N, Typ);
            return True;
         end;
      end;

   exception
      when Not_Handled =>
         return False;
   end Packed_Array_Aggregate_Handled;

   ------------------------------
   -- Initialize_Discriminants --
   ------------------------------

   procedure Initialize_Discriminants (N : Node_Id; Typ : Entity_Id) is
      Loc  : constant Source_Ptr := Sloc (N);
      Bas  : constant Entity_Id  := Base_Type (Typ);
      Par  : constant Entity_Id  := Etype (Bas);
      Decl : constant Node_Id    := Parent (Par);
      Ref  : Node_Id;

   begin
      if Is_Tagged_Type (Bas)
        and then Is_Derived_Type (Bas)
        and then Has_Discriminants (Par)
        and then Has_Discriminants (Bas)
        and then Number_Discriminants (Bas) /= Number_Discriminants (Par)
        and then Nkind (Decl) = N_Full_Type_Declaration
        and then Nkind (Type_Definition (Decl)) = N_Record_Definition
        and then Present
          (Variant_Part (Component_List (Type_Definition (Decl))))
        and then Nkind (N) /= N_Extension_Aggregate
      then

         --   Call init_proc to set discriminants.
         --   There should eventually be a special procedure for this ???

         Ref := New_Reference_To (Defining_Identifier (N), Loc);
         Insert_Actions_After (N,
           Build_Initialization_Call (Sloc (N), Ref, Typ));
      end if;
   end Initialize_Discriminants;

   ---------------------------
   -- Safe_Slice_Assignment --
   ---------------------------

   function Safe_Slice_Assignment (N : Node_Id) return Boolean is
      Loc        : constant Source_Ptr := Sloc (Parent (N));
      Pref       : constant Node_Id    := Prefix (Name (Parent (N)));
      Range_Node : constant Node_Id    := Discrete_Range (Name (Parent (N)));
      Expr       : Node_Id;
      L_J        : Entity_Id;
      L_Iter     : Node_Id;
      L_Body     : Node_Id;
      Stat       : Node_Id;

   begin
      --  Generate: for J in Range loop Pref (J) := Expr; end loop;

      if Comes_From_Source (N)
        and then No (Expressions (N))
        and then Nkind (First (Choices (First (Component_Associations (N)))))
                   = N_Others_Choice
      then
         Expr :=
           Expression (First (Component_Associations (N)));
         L_J := Make_Defining_Identifier (Loc, New_Internal_Name ('J'));

         L_Iter :=
           Make_Iteration_Scheme (Loc,
             Loop_Parameter_Specification =>
               Make_Loop_Parameter_Specification
                 (Loc,
                  Defining_Identifier         => L_J,
                  Discrete_Subtype_Definition => Relocate_Node (Range_Node)));

         L_Body :=
           Make_Assignment_Statement (Loc,
              Name =>
                Make_Indexed_Component (Loc,
                  Prefix      => Relocate_Node (Pref),
                  Expressions => New_List (New_Occurrence_Of (L_J, Loc))),
               Expression => Relocate_Node (Expr));

         --  Construct the final loop

         Stat :=
           Make_Implicit_Loop_Statement
             (Node             => Parent (N),
              Identifier       => Empty,
              Iteration_Scheme => L_Iter,
              Statements       => New_List (L_Body));

         Rewrite (Parent (N), Stat);
         Analyze (Parent (N));
         return True;

      else
         return False;
      end if;
   end Safe_Slice_Assignment;

   ---------------------
   -- Sort_Case_Table --
   ---------------------

   procedure Sort_Case_Table (Case_Table : in out Case_Table_Type) is
      L : Int := Case_Table'First;
      U : Int := Case_Table'Last;
      K : Int;
      J : Int;
      T : Case_Bounds;

   begin
      K := L;

      while K /= U loop
         T := Case_Table (K + 1);
         J := K + 1;

         while J /= L
           and then Expr_Value (Case_Table (J - 1).Choice_Lo) >
                    Expr_Value (T.Choice_Lo)
         loop
            Case_Table (J) := Case_Table (J - 1);
            J := J - 1;
         end loop;

         Case_Table (J) := T;
         K := K + 1;
      end loop;
   end Sort_Case_Table;

end Exp_Aggr;
