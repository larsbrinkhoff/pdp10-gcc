------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             S E M _ C H 1 0                              --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                                                                          --
--          Copyright (C) 1992-2001 Free Software Foundation, Inc.          --
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
with Debug;    use Debug;
with Einfo;    use Einfo;
with Errout;   use Errout;
with Exp_Util; use Exp_Util;
with Fname;    use Fname;
with Fname.UF; use Fname.UF;
with Freeze;   use Freeze;
with Impunit;  use Impunit;
with Inline;   use Inline;
with Lib;      use Lib;
with Lib.Load; use Lib.Load;
with Lib.Xref; use Lib.Xref;
with Namet;    use Namet;
with Nlists;   use Nlists;
with Nmake;    use Nmake;
with Opt;      use Opt;
with Output;   use Output;
with Restrict; use Restrict;
with Sem;      use Sem;
with Sem_Ch6;  use Sem_Ch6;
with Sem_Ch7;  use Sem_Ch7;
with Sem_Ch8;  use Sem_Ch8;
with Sem_Dist; use Sem_Dist;
with Sem_Prag; use Sem_Prag;
with Sem_Util; use Sem_Util;
with Sem_Warn; use Sem_Warn;
with Stand;    use Stand;
with Sinfo;    use Sinfo;
with Sinfo.CN; use Sinfo.CN;
with Sinput;   use Sinput;
with Snames;   use Snames;
with Style;    use Style;
with Tbuild;   use Tbuild;
with Ttypes;   use Ttypes;
with Uname;    use Uname;

package body Sem_Ch10 is

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Analyze_Context (N : Node_Id);
   --  Analyzes items in the context clause of compilation unit

   procedure Check_With_Type_Clauses (N : Node_Id);
   --  If N is a body, verify that any with_type clauses on the spec, or
   --  on the spec of any parent, have a matching with_clause.

   procedure Check_Private_Child_Unit (N : Node_Id);
   --  If a with_clause mentions a private child unit, the compilation
   --  unit must be a member of the same family, as described in 10.1.2 (8).

   procedure Check_Stub_Level (N : Node_Id);
   --  Verify that a stub is declared immediately within a compilation unit,
   --  and not in an inner frame.

   procedure Expand_With_Clause (Nam : Node_Id; N : Node_Id);
   --  When a child unit appears in a context clause, the implicit withs on
   --  parents are made explicit, and with clauses are inserted in the context
   --  clause before the one for the child. If a parent in the with_clause
   --  is a renaming, the implicit with_clause is on the renaming whose name
   --  is mentioned in the with_clause, and not on the package it renames.
   --  N is the compilation unit whose list of context items receives the
   --  implicit with_clauses.

   function Get_Parent_Entity (Unit : Node_Id) return Entity_Id;
   --  Get defining entity of parent unit of a child unit. In most cases this
   --  is the defining entity of the unit, but for a child instance whose
   --  parent needs a body for inlining, the instantiation node of the parent
   --  has not yet been rewritten as a package declaration, and the entity has
   --  to be retrieved from the Instance_Spec of the unit.

   procedure Implicit_With_On_Parent (Child_Unit : Node_Id; N : Node_Id);
   --  If the main unit is a child unit, implicit withs are also added for
   --  all its ancestors.

   procedure Install_Context_Clauses (N : Node_Id);
   --  Subsidiary to previous one. Process only with_ and use_clauses for
   --  current unit and its library unit if any.

   procedure Install_Withed_Unit (With_Clause : Node_Id);
   --  If the unit is not a child unit, make unit immediately visible.
   --  The caller ensures that the unit is not already currently installed.

   procedure Install_Parents (Lib_Unit : Node_Id; Is_Private : Boolean);
   --  This procedure establishes the context for the compilation of a child
   --  unit. If Lib_Unit is a child library spec then the context of the parent
   --  is installed, and the parent itself made immediately visible, so that
   --  the child unit is processed in the declarative region of the parent.
   --  Install_Parents makes a recursive call to itself to ensure that all
   --  parents are loaded in the nested case. If Lib_Unit is a library body,
   --  the only effect of Install_Parents is to install the private decls of
   --  the parents, because the visible parent declarations will have been
   --  installed as part of the context of the corresponding spec.

   procedure Install_Siblings (U_Name : Entity_Id; N : Node_Id);
   --  In the compilation of a child unit, a child of any of the  ancestor
   --  units is directly visible if it is visible, because the parent is in
   --  an enclosing scope. Iterate over context to find child units of U_Name
   --  or of some ancestor of it.

   function Is_Child_Spec (Lib_Unit : Node_Id) return Boolean;
   --  Lib_Unit is a library unit which may be a spec or a body. Is_Child_Spec
   --  returns True if Lib_Unit is a library spec which is a child spec, i.e.
   --  a library spec that has a parent. If the call to Is_Child_Spec returns
   --  True, then Parent_Spec (Lib_Unit) is non-Empty and points to the
   --  compilation unit for the parent spec.
   --
   --  Lib_Unit can also be a subprogram body that acts as its own spec. If
   --  the Parent_Spec is  non-empty, this is also a child unit.

   procedure Remove_With_Type_Clause (Name : Node_Id);
   --  Remove imported type and its enclosing package from visibility, and
   --  remove attributes of imported type so they don't interfere with its
   --  analysis (should it appear otherwise in the context).

   procedure Remove_Context_Clauses (N : Node_Id);
   --  Subsidiary of previous one. Remove use_ and with_clauses.

   procedure Remove_Parents (Lib_Unit : Node_Id);
   --  Remove_Parents checks if Lib_Unit is a child spec. If so then the parent
   --  contexts established by the corresponding call to Install_Parents are
   --  removed. Remove_Parents contains a recursive call to itself to ensure
   --  that all parents are removed in the nested case.

   procedure Remove_Unit_From_Visibility (Unit_Name : Entity_Id);
   --  Reset all visibility flags on unit after compiling it, either as a
   --  main unit or as a unit in the context.

   procedure Analyze_Proper_Body (N : Node_Id; Nam : Entity_Id);
   --  Common processing for all stubs (subprograms, tasks, packages, and
   --  protected cases). N is the stub to be analyzed. Once the subunit
   --  name is established, load and analyze. Nam is the non-overloadable
   --  entity for which the proper body provides a completion. Subprogram
   --  stubs are handled differently because they can be declarations.

   ------------------------------
   -- Analyze_Compilation_Unit --
   ------------------------------

   procedure Analyze_Compilation_Unit (N : Node_Id) is
      Unit_Node     : constant Node_Id := Unit (N);
      Lib_Unit      : Node_Id          := Library_Unit (N);
      Spec_Id       : Node_Id;
      Main_Cunit    : constant Node_Id := Cunit (Main_Unit);
      Par_Spec_Name : Unit_Name_Type;
      Unum          : Unit_Number_Type;

      procedure Generate_Parent_References (N : Node_Id; P_Id : Entity_Id);
      --  Generate cross-reference information for the parents of child units.
      --  N is a defining_program_unit_name, and P_Id is the immediate parent.

      --------------------------------
      -- Generate_Parent_References --
      --------------------------------

      procedure Generate_Parent_References (N : Node_Id; P_Id : Entity_Id) is
         Pref   : Node_Id;
         P_Name : Entity_Id := P_Id;

      begin
         Pref   := Name (Parent (Defining_Entity (N)));

         if Nkind (Pref) = N_Expanded_Name then

            --  Done already, if the unit has been compiled indirectly as
            --  part of the closure of its context because of inlining.

            return;
         end if;

         while Nkind (Pref) = N_Selected_Component loop
            Change_Selected_Component_To_Expanded_Name (Pref);
            Set_Entity (Pref, P_Name);
            Set_Etype (Pref, Etype (P_Name));
            Generate_Reference (P_Name, Pref, 'r');
            Pref   := Prefix (Pref);
            P_Name := Scope (P_Name);
         end loop;

         --  The guard here on P_Name is to handle the error condition where
         --  the parent unit is missing because the file was not found.

         if Present (P_Name) then
            Set_Entity (Pref, P_Name);
            Set_Etype (Pref, Etype (P_Name));
            Generate_Reference (P_Name, Pref, 'r');
            Style.Check_Identifier (Pref, P_Name);
         end if;
      end Generate_Parent_References;

   --  Start of processing for Analyze_Compilation_Unit

   begin
      Process_Compilation_Unit_Pragmas (N);

      --  If the unit is a subunit whose parent has not been analyzed (which
      --  indicates that the main unit is a subunit, either the current one or
      --  one of its descendents) then the subunit is compiled as part of the
      --  analysis of the parent, which we proceed to do. Basically this gets
      --  handled from the top down and we don't want to do anything at this
      --  level (i.e. this subunit will be handled on the way down from the
      --  parent), so at this level we immediately return. If the subunit
      --  ends up not analyzed, it means that the parent did not contain a
      --  stub for it, or that there errors were dectected in some ancestor.

      if Nkind (Unit_Node) = N_Subunit
        and then not Analyzed (Lib_Unit)
      then
         Semantics (Lib_Unit);

         if not Analyzed (Proper_Body (Unit_Node)) then
            if Serious_Errors_Detected > 0 then
               Error_Msg_N ("subunit not analyzed (errors in parent unit)", N);
            else
               Error_Msg_N ("missing stub for subunit", N);
            end if;
         end if;

         return;
      end if;

      --  Analyze context (this will call Sem recursively for with'ed units)

      Analyze_Context (N);

      --  If the unit is a package body, the spec is already loaded and must
      --  be analyzed first, before we analyze the body.

      if Nkind (Unit_Node) = N_Package_Body then

         --  If no Lib_Unit, then there was a serious previous error, so
         --  just ignore the entire analysis effort

         if No (Lib_Unit) then
            return;

         else
            Semantics (Lib_Unit);
            Check_Unused_Withs (Get_Cunit_Unit_Number (Lib_Unit));

            --  Verify that the library unit is a package declaration.

            if Nkind (Unit (Lib_Unit)) /= N_Package_Declaration
                 and then
               Nkind (Unit (Lib_Unit)) /= N_Generic_Package_Declaration
            then
               Error_Msg_N
                 ("no legal package declaration for package body", N);
               return;

            --  Otherwise, the entity in the declaration is visible. Update
            --  the version to reflect dependence of this body on the spec.

            else
               Spec_Id := Defining_Entity (Unit (Lib_Unit));
               Set_Is_Immediately_Visible (Spec_Id, True);
               Version_Update (N, Lib_Unit);

               if Nkind (Defining_Unit_Name (Unit_Node))
                 = N_Defining_Program_Unit_Name
               then
                  Generate_Parent_References (Unit_Node, Scope (Spec_Id));
               end if;
            end if;
         end if;

      --  If the unit is a subprogram body, then we similarly need to analyze
      --  its spec. However, things are a little simpler in this case, because
      --  here, this analysis is done only for error checking and consistency
      --  purposes, so there's nothing else to be done.

      elsif Nkind (Unit_Node) = N_Subprogram_Body then
         if Acts_As_Spec (N) then

            --  If the subprogram body is a child unit, we must create a
            --  declaration for it, in order to properly load the parent(s).
            --  After this, the original unit does not acts as a spec, because
            --  there is an explicit one. If this  unit appears in a context
            --  clause, then an implicit with on the parent will be added when
            --  installing the context. If this is the main unit, there is no
            --  Unit_Table entry for the declaration, (It has the unit number
            --  of the main unit) and code generation is unaffected.

            Unum := Get_Cunit_Unit_Number (N);
            Par_Spec_Name := Get_Parent_Spec_Name (Unit_Name (Unum));

            if Par_Spec_Name /= No_Name then
               Unum :=
                 Load_Unit
                   (Load_Name  => Par_Spec_Name,
                    Required   => True,
                    Subunit    => False,
                    Error_Node => N);

               if Unum /= No_Unit then

                  --  Build subprogram declaration and attach parent unit to it
                  --  This subprogram declaration does not come from source!

                  declare
                     Loc : constant Source_Ptr := Sloc (N);
                     SCS : constant Boolean :=
                             Get_Comes_From_Source_Default;

                  begin
                     Set_Comes_From_Source_Default (False);
                     Lib_Unit :=
                       Make_Compilation_Unit (Loc,
                         Context_Items => New_Copy_List (Context_Items (N)),
                         Unit =>
                           Make_Subprogram_Declaration (Sloc (N),
                             Specification =>
                               Copy_Separate_Tree
                                 (Specification (Unit_Node))),
                         Aux_Decls_Node =>
                           Make_Compilation_Unit_Aux (Loc));

                     Set_Library_Unit (N, Lib_Unit);
                     Set_Parent_Spec (Unit (Lib_Unit), Cunit (Unum));
                     Semantics (Lib_Unit);
                     Set_Acts_As_Spec (N, False);
                     Set_Comes_From_Source_Default (SCS);
                  end;
               end if;
            end if;

         --  Here for subprogram with separate declaration

         else
            Semantics (Lib_Unit);
            Check_Unused_Withs (Get_Cunit_Unit_Number (Lib_Unit));
            Version_Update (N, Lib_Unit);
         end if;

         if Nkind (Defining_Unit_Name (Specification (Unit_Node))) =
                                             N_Defining_Program_Unit_Name
         then
            Generate_Parent_References (
              Specification (Unit_Node),
                Scope (Defining_Entity (Unit (Lib_Unit))));
         end if;
      end if;

      --  If it is a child unit, the parent must be elaborated first
      --  and we update version, since we are dependent on our parent.

      if Is_Child_Spec (Unit_Node) then

         --  The analysis of the parent is done with style checks off

         declare
            Save_Style_Check : constant Boolean := Opt.Style_Check;
            Save_C_Restrict  : constant Save_Compilation_Unit_Restrictions :=
                                 Compilation_Unit_Restrictions_Save;

         begin
            if not GNAT_Mode then
               Style_Check := False;
            end if;

            Semantics (Parent_Spec (Unit_Node));
            Version_Update (N, Parent_Spec (Unit_Node));
            Style_Check := Save_Style_Check;
            Compilation_Unit_Restrictions_Restore (Save_C_Restrict);
         end;
      end if;

      --  With the analysis done, install the context. Note that we can't
      --  install the context from the with clauses as we analyze them,
      --  because each with clause must be analyzed in a clean visibility
      --  context, so we have to wait and install them all at once.

      Install_Context (N);

      if Is_Child_Spec (Unit_Node) then

         --  Set the entities of all parents in the program_unit_name.

         Generate_Parent_References (
           Unit_Node, Get_Parent_Entity (Unit (Parent_Spec (Unit_Node))));
      end if;

      --  All components of the context: with-clauses, library unit, ancestors
      --  if any, (and their context)  are analyzed and installed. Now analyze
      --  the unit itself, which is either a package, subprogram spec or body.

      Analyze (Unit_Node);

      --  The above call might have made Unit_Node an N_Subprogram_Body
      --  from something else, so propagate any Acts_As_Spec flag.

      if Nkind (Unit_Node) = N_Subprogram_Body
        and then Acts_As_Spec (Unit_Node)
      then
         Set_Acts_As_Spec (N);
      end if;

      --  Treat compilation unit pragmas that appear after the library unit

      if Present (Pragmas_After (Aux_Decls_Node (N))) then
         declare
            Prag_Node : Node_Id := First (Pragmas_After (Aux_Decls_Node (N)));

         begin
            while Present (Prag_Node) loop
               Analyze (Prag_Node);
               Next (Prag_Node);
            end loop;
         end;
      end if;

      --  Generate distribution stub files if requested and no error

      if N = Main_Cunit
        and then (Distribution_Stub_Mode = Generate_Receiver_Stub_Body
                    or else
                  Distribution_Stub_Mode = Generate_Caller_Stub_Body)
        and then not Fatal_Error (Main_Unit)
      then
         if Is_RCI_Pkg_Spec_Or_Body (N) then

            --  Regular RCI package

            Add_Stub_Constructs (N);

         elsif (Nkind (Unit_Node) = N_Package_Declaration
                 and then Is_Shared_Passive (Defining_Entity
                                              (Specification (Unit_Node))))
           or else (Nkind (Unit_Node) = N_Package_Body
                     and then
                       Is_Shared_Passive (Corresponding_Spec (Unit_Node)))
         then
            --  Shared passive package

            Add_Stub_Constructs (N);

         elsif Nkind (Unit_Node) = N_Package_Instantiation
           and then
             Is_Remote_Call_Interface
               (Defining_Entity (Specification (Instance_Spec (Unit_Node))))
         then
            --  Instantiation of a RCI generic package

            Add_Stub_Constructs (N);
         end if;

         --  Reanalyze the unit with the new constructs

         Analyze (Unit_Node);
      end if;

      if Nkind (Unit_Node) = N_Package_Declaration
        or else Nkind (Unit_Node) in N_Generic_Declaration
        or else Nkind (Unit_Node) = N_Package_Renaming_Declaration
        or else Nkind (Unit_Node) = N_Subprogram_Declaration
      then
         Remove_Unit_From_Visibility (Defining_Entity (Unit_Node));

      elsif Nkind (Unit_Node) = N_Package_Body
        or else (Nkind (Unit_Node) = N_Subprogram_Body
                  and then not Acts_As_Spec (Unit_Node))
      then
         --  Bodies that are not the main unit are compiled if they
         --  are generic or contain generic or inlined units. Their
         --  analysis brings in the context of the corresponding spec
         --  (unit declaration) which must be removed as well, to
         --  return the compilation environment to its proper state.

         Remove_Context (Lib_Unit);
         Set_Is_Immediately_Visible (Defining_Entity (Unit (Lib_Unit)), False);
      end if;

      --  Last step is to deinstall the context we just installed
      --  as well as the unit just compiled.

      Remove_Context (N);

      --  If this is the main unit and we are generating code, we must
      --  check that all generic units in the context have a body if they
      --  need it, even if they have not been instantiated. In the absence
      --  of .ali files for generic units, we must force the load of the body,
      --  just to produce the proper error if the body is absent. We skip this
      --  verification if the main unit itself is generic.

      if Get_Cunit_Unit_Number (N) = Main_Unit
        and then Operating_Mode = Generate_Code
        and then Expander_Active
      then
         --  Indicate that the main unit is now analyzed, to catch possible
         --  circularities between it and generic bodies. Remove main unit
         --  from visibility. This might seem superfluous, but the main unit
         --  must not be visible in the generic body expansions that follow.

         Set_Analyzed (N, True);
         Set_Is_Immediately_Visible (Cunit_Entity (Main_Unit), False);

         declare
            Item  : Node_Id;
            Nam   : Entity_Id;
            Un    : Unit_Number_Type;

            Save_Style_Check : constant Boolean := Opt.Style_Check;
            Save_C_Restrict  : constant Save_Compilation_Unit_Restrictions :=
                                 Compilation_Unit_Restrictions_Save;

         begin
            Item := First (Context_Items (N));

            while Present (Item) loop

               if Nkind (Item) = N_With_Clause
                  and then not Implicit_With (Item)
               then
                  Nam := Entity (Name (Item));

                  if (Ekind (Nam) = E_Generic_Procedure
                       and then not Is_Intrinsic_Subprogram (Nam))
                    or else (Ekind (Nam) = E_Generic_Function
                              and then not Is_Intrinsic_Subprogram (Nam))
                    or else (Ekind (Nam) = E_Generic_Package
                              and then Unit_Requires_Body (Nam))
                  then
                     Opt.Style_Check := False;

                     if Present (Renamed_Object (Nam)) then
                        Un :=
                           Load_Unit
                             (Load_Name  => Get_Body_Name
                                              (Get_Unit_Name
                                                (Unit_Declaration_Node
                                                  (Renamed_Object (Nam)))),
                              Required   => False,
                              Subunit    => False,
                              Error_Node => N,
                              Renamings  => True);
                     else
                        Un :=
                          Load_Unit
                            (Load_Name  => Get_Body_Name
                                             (Get_Unit_Name (Item)),
                             Required   => False,
                             Subunit    => False,
                             Error_Node => N,
                             Renamings  => True);
                     end if;

                     if Un = No_Unit then
                        Error_Msg_NE
                          ("body of generic unit& not found", Item, Nam);
                        exit;

                     elsif not Analyzed (Cunit (Un))
                       and then Un /= Main_Unit
                     then
                        Opt.Style_Check := False;
                        Semantics (Cunit (Un));
                     end if;
                  end if;
               end if;

               Next (Item);
            end loop;

            Style_Check := Save_Style_Check;
            Compilation_Unit_Restrictions_Restore (Save_C_Restrict);
         end;
      end if;

      --  Deal with creating elaboration Boolean if needed. We create an
      --  elaboration boolean only for units that come from source since
      --  units manufactured by the compiler never need elab checks.

      if Comes_From_Source (N)
        and then
          (Nkind (Unit (N)) =  N_Package_Declaration         or else
           Nkind (Unit (N)) =  N_Generic_Package_Declaration or else
           Nkind (Unit (N)) =  N_Subprogram_Declaration      or else
           Nkind (Unit (N)) =  N_Generic_Subprogram_Declaration)
      then
         declare
            Loc  : constant Source_Ptr := Sloc (N);
            Unum : constant Unit_Number_Type := Get_Source_Unit (Loc);

         begin
            Spec_Id := Defining_Entity (Unit (N));
            Generate_Definition (Spec_Id);

            --  See if an elaboration entity is required for possible
            --  access before elaboration checking. Note that we must
            --  allow for this even if -gnatE is not set, since a client
            --  may be compiled in -gnatE mode and reference the entity.

            --  Case of units which do not require elaboration checks

            if
               --  Pure units do not need checks

                 Is_Pure (Spec_Id)

               --  Preelaborated units do not need checks

                 or else Is_Preelaborated (Spec_Id)

               --  No checks needed if pagma Elaborate_Body present

                 or else Has_Pragma_Elaborate_Body (Spec_Id)

               --  No checks needed if unit does not require a body

                 or else not Unit_Requires_Body (Spec_Id)

               --  No checks needed for predefined files

                 or else Is_Predefined_File_Name (Unit_File_Name (Unum))

               --  No checks required if no separate spec

                 or else Acts_As_Spec (N)
            then
               --  This is a case where we only need the entity for
               --  checking to prevent multiple elaboration checks.

               Set_Elaboration_Entity_Required (Spec_Id, False);

            --  Case of elaboration entity is required for access before
            --  elaboration checking (so certainly we must build it!)

            else
               Set_Elaboration_Entity_Required (Spec_Id, True);
            end if;

            Build_Elaboration_Entity (N, Spec_Id);
         end;
      end if;

      --  Finally, freeze the compilation unit entity. This for sure is needed
      --  because of some warnings that can be output (see Freeze_Subprogram),
      --  but may in general be required. If freezing actions result, place
      --  them in the compilation unit actions list, and analyze them.

      declare
         Loc : constant Source_Ptr := Sloc (N);
         L   : constant List_Id :=
                 Freeze_Entity (Cunit_Entity (Current_Sem_Unit), Loc);

      begin
         while Is_Non_Empty_List (L) loop
            Insert_Library_Level_Action (Remove_Head (L));
         end loop;
      end;

      Set_Analyzed (N);

      if Nkind (Unit_Node) = N_Package_Declaration
        and then Get_Cunit_Unit_Number (N) /= Main_Unit
        and then Front_End_Inlining
        and then Expander_Active
      then
         Check_Body_For_Inlining (N, Defining_Entity (Unit_Node));
      end if;
   end Analyze_Compilation_Unit;

   ---------------------
   -- Analyze_Context --
   ---------------------

   procedure Analyze_Context (N : Node_Id) is
      Item  : Node_Id;

   begin
      --  Loop through context items

      Item := First (Context_Items (N));
      while Present (Item) loop

         --  For with clause, analyze the with clause, and then update
         --  the version, since we are dependent on a unit that we with.

         if Nkind (Item) = N_With_Clause then

            --  Skip analyzing with clause if no unit, nothing to do (this
            --  happens for a with that references a non-existent unit)

            if Present (Library_Unit (Item)) then
               Analyze (Item);
            end if;

            if not Implicit_With (Item) then
               Version_Update (N, Library_Unit (Item));
            end if;

         --  But skip use clauses at this stage, since we don't want to do
         --  any installing of potentially use visible entities until we
         --  we actually install the complete context (in Install_Context).
         --  Otherwise things can get installed in the wrong context.
         --  Similarly, pragmas are analyzed in Install_Context, after all
         --  the implicit with's on parent units are generated.

         else
            null;
         end if;

         Next (Item);
      end loop;
   end Analyze_Context;

   -------------------------------
   -- Analyze_Package_Body_Stub --
   -------------------------------

   procedure Analyze_Package_Body_Stub (N : Node_Id) is
      Id   : constant Entity_Id := Defining_Identifier (N);
      Nam  : Entity_Id;

   begin
      --  The package declaration must be in the current declarative part.

      Check_Stub_Level (N);
      Nam := Current_Entity_In_Scope (Id);

      if No (Nam) or else not Is_Package (Nam) then
         Error_Msg_N ("missing specification for package stub", N);

      elsif Has_Completion (Nam)
        and then Present (Corresponding_Body (Unit_Declaration_Node (Nam)))
      then
         Error_Msg_N ("duplicate or redundant stub for package", N);

      else
         --  Indicate that the body of the package exists. If we are doing
         --  only semantic analysis, the stub stands for the body. If we are
         --  generating code, the existence of the body will be confirmed
         --  when we load the proper body.

         Set_Has_Completion (Nam);
         Set_Scope (Defining_Entity (N), Current_Scope);
         Analyze_Proper_Body (N, Nam);
      end if;
   end Analyze_Package_Body_Stub;

   -------------------------
   -- Analyze_Proper_Body --
   -------------------------

   procedure Analyze_Proper_Body (N : Node_Id; Nam : Entity_Id) is
      Subunit_Name      : constant Unit_Name_Type := Get_Unit_Name (N);
      Unum              : Unit_Number_Type;
      Subunit_Not_Found : Boolean := False;

      procedure Optional_Subunit;
      --  This procedure is called when the main unit is a stub, or when we
      --  are not generating code. In such a case, we analyze the subunit if
      --  present, which is user-friendly and in fact required for ASIS, but
      --  we don't complain if the subunit is missing.

      ----------------------
      -- Optional_Subunit --
      ----------------------

      procedure Optional_Subunit is
         Comp_Unit : Node_Id;

      begin
         --  Try to load subunit, but ignore any errors that occur during
         --  the loading of the subunit, by using the special feature in
         --  Errout to ignore all errors. Note that Fatal_Error will still
         --  be set, so we will be able to check for this case below.

         Ignore_Errors_Enable := Ignore_Errors_Enable + 1;
         Unum :=
           Load_Unit
             (Load_Name  => Subunit_Name,
              Required   => False,
              Subunit    => True,
              Error_Node => N);
         Ignore_Errors_Enable := Ignore_Errors_Enable - 1;

         --  All done if we successfully loaded the subunit

         if Unum /= No_Unit and then not Fatal_Error (Unum) then
            Comp_Unit := Cunit (Unum);

            Set_Corresponding_Stub (Unit (Comp_Unit), N);
            Analyze_Subunit (Comp_Unit);
            Set_Library_Unit (N, Comp_Unit);

         elsif Unum = No_Unit
           and then Present (Nam)
         then
            if Is_Protected_Type (Nam) then
               Set_Corresponding_Body (Parent (Nam), Defining_Identifier (N));
            else
               Set_Corresponding_Body (
                 Unit_Declaration_Node (Nam), Defining_Identifier (N));
            end if;
         end if;
      end Optional_Subunit;

   --  Start of processing for Analyze_Proper_Body

   begin
      --  If the subunit is already loaded, it means that the main unit
      --  is a subunit, and that the current unit is one of its parents
      --  which was being analyzed to provide the needed context for the
      --  analysis of the subunit. In this case we analyze the subunit and
      --  continue with the parent, without looking a subsequent subunits.

      if Is_Loaded (Subunit_Name) then

         --  If the proper body is already linked to the stub node,
         --  the stub is in a generic unit and just needs analyzing.

         if Present (Library_Unit (N)) then
            Set_Corresponding_Stub (Unit (Library_Unit (N)), N);
            Analyze_Subunit (Library_Unit (N));

         --  Otherwise we must load the subunit and link to it

         else
            --  Load the subunit, this must work, since we originally
            --  loaded the subunit earlier on. So this will not really
            --  load it, just give access to it.

            Unum :=
              Load_Unit
                (Load_Name  => Subunit_Name,
                 Required   => True,
                 Subunit    => False,
                 Error_Node => N);

            --  And analyze the subunit in the parent context (note that we
            --  do not call Semantics, since that would remove the parent
            --  context). Because of this, we have to manually reset the
            --  compiler state to Analyzing since it got destroyed by Load.

            if Unum /= No_Unit then
               Compiler_State := Analyzing;
               Set_Corresponding_Stub (Unit (Cunit (Unum)), N);
               Analyze_Subunit (Cunit (Unum));
               Set_Library_Unit (N, Cunit (Unum));
            end if;
         end if;

      --  If the main unit is a subunit, then we are just performing semantic
      --  analysis on that subunit, and any other subunits of any parent unit
      --  should be ignored, except that if we are building trees for ASIS
      --  usage we want to annotate the stub properly.

      elsif Nkind (Unit (Cunit (Main_Unit))) = N_Subunit
        and then Subunit_Name /= Unit_Name (Main_Unit)
      then
         if Tree_Output then
            Optional_Subunit;
         end if;

         --  But before we return, set the flag for unloaded subunits. This
         --  will suppress junk warnings of variables in the same declarative
         --  part (or a higher level one) that are in danger of looking unused
         --  when in fact there might be a declaration in the subunit that we
         --  do not intend to load.

         Unloaded_Subunits := True;
         return;

      --  If the subunit is not already loaded, and we are generating code,
      --  then this is the case where compilation started from the parent,
      --  and we are generating code for an entire subunit tree. In that
      --  case we definitely need to load the subunit.

      --  In order to continue the analysis with the rest of the parent,
      --  and other subunits, we load the unit without requiring its
      --  presence, and emit a warning if not found, rather than terminating
      --  the compilation abruptly, as for other missing file problems.

      elsif Operating_Mode = Generate_Code then

         --  If the proper body is already linked to the stub node,
         --  the stub is in a generic unit and just needs analyzing.

         --  We update the version. Although we are not technically
         --  semantically dependent on the subunit, given our approach
         --  of macro substitution of subunits, it makes sense to
         --  include it in the version identification.

         if Present (Library_Unit (N)) then
            Set_Corresponding_Stub (Unit (Library_Unit (N)), N);
            Analyze_Subunit (Library_Unit (N));
            Version_Update (Cunit (Main_Unit), Library_Unit (N));

         --  Otherwise we must load the subunit and link to it

         else
            Unum :=
              Load_Unit
                (Load_Name  => Subunit_Name,
                 Required   => False,
                 Subunit    => True,
                 Error_Node => N);

            if Operating_Mode = Generate_Code
              and then Unum = No_Unit
            then
               Error_Msg_Name_1 := Subunit_Name;
               Error_Msg_Name_2 :=
                 Get_File_Name (Subunit_Name, Subunit => True);
               Error_Msg_N
                 ("subunit% in file{ not found!?", N);
               Subunits_Missing := True;
               Subunit_Not_Found := True;
            end if;

            --  Load_Unit may reset Compiler_State, since it may have been
            --  necessary to parse an additional units, so we make sure
            --  that we reset it to the Analyzing state.

            Compiler_State := Analyzing;

            if Unum /= No_Unit and then not Fatal_Error (Unum) then

               if Debug_Flag_L then
                  Write_Str ("*** Loaded subunit from stub. Analyze");
                  Write_Eol;
               end if;

               declare
                  Comp_Unit : constant Node_Id := Cunit (Unum);

               begin
                  --  Check for child unit instead of subunit

                  if Nkind (Unit (Comp_Unit)) /= N_Subunit then
                     Error_Msg_N
                       ("expected SEPARATE subunit, found child unit",
                        Cunit_Entity (Unum));

                  --  OK, we have a subunit, so go ahead and analyze it,
                  --  and set Scope of entity in stub, for ASIS use.

                  else
                     Set_Corresponding_Stub (Unit (Comp_Unit), N);
                     Analyze_Subunit (Comp_Unit);
                     Set_Library_Unit (N, Comp_Unit);

                     --  We update the version. Although we are not technically
                     --  semantically dependent on the subunit, given our
                     --  approach of macro substitution of subunits, it makes
                     --  sense to include it in the version identification.

                     Version_Update (Cunit (Main_Unit), Comp_Unit);
                  end if;
               end;
            end if;
         end if;

         --  The remaining case is when the subunit is not already loaded and
         --  we are not generating code. In this case we are just performing
         --  semantic analysis on the parent, and we are not interested in
         --  the subunit. For subprograms, analyze the stub as a body. For
         --  other entities the stub has already been marked as completed.

      else
         Optional_Subunit;
      end if;

   end Analyze_Proper_Body;

   ----------------------------------
   -- Analyze_Protected_Body_Stub --
   ----------------------------------

   procedure Analyze_Protected_Body_Stub (N : Node_Id) is
      Nam : Entity_Id := Current_Entity_In_Scope (Defining_Identifier (N));

   begin
      Check_Stub_Level (N);

      --  First occurrence of name may have been as an incomplete type.

      if Present (Nam) and then Ekind (Nam) = E_Incomplete_Type then
         Nam := Full_View (Nam);
      end if;

      if No (Nam)
        or else not Is_Protected_Type (Etype (Nam))
      then
         Error_Msg_N ("missing specification for Protected body", N);
      else
         Set_Scope (Defining_Entity (N), Current_Scope);
         Set_Has_Completion (Etype (Nam));
         Analyze_Proper_Body (N, Etype (Nam));
      end if;
   end Analyze_Protected_Body_Stub;

   ----------------------------------
   -- Analyze_Subprogram_Body_Stub --
   ----------------------------------

   --  A subprogram body stub can appear with or without a previous
   --  specification. If there is one, the analysis of the body will
   --  find it and verify conformance.  The formals appearing in the
   --  specification of the stub play no role, except for requiring an
   --  additional conformance check. If there is no previous subprogram
   --  declaration, the stub acts as a spec, and provides the defining
   --  entity for the subprogram.

   procedure Analyze_Subprogram_Body_Stub (N : Node_Id) is
      Decl : Node_Id;

   begin
      Check_Stub_Level (N);

      --  Verify that the identifier for the stub is unique within this
      --  declarative part.

      if Nkind (Parent (N)) = N_Block_Statement
        or else Nkind (Parent (N)) = N_Package_Body
        or else Nkind (Parent (N)) = N_Subprogram_Body
      then
         Decl := First (Declarations (Parent (N)));

         while Present (Decl)
           and then Decl /= N
         loop
            if Nkind (Decl) = N_Subprogram_Body_Stub
              and then (Chars (Defining_Unit_Name (Specification (Decl)))
                      = Chars (Defining_Unit_Name (Specification (N))))
            then
               Error_Msg_N ("identifier for stub is not unique", N);
            end if;

            Next (Decl);
         end loop;
      end if;

      --  Treat stub as a body, which checks conformance if there is a previous
      --  declaration, or else introduces entity and its signature.

      Analyze_Subprogram_Body (N);

      if Serious_Errors_Detected = 0 then
         Analyze_Proper_Body (N, Empty);
      end if;

   end Analyze_Subprogram_Body_Stub;

   ---------------------
   -- Analyze_Subunit --
   ---------------------

   --  A subunit is compiled either by itself (for semantic checking)
   --  or as part of compiling the parent (for code generation). In
   --  either case, by the time we actually process the subunit, the
   --  parent has already been installed and analyzed. The node N is
   --  a compilation unit, whose context needs to be treated here,
   --  because we come directly here from the parent without calling
   --  Analyze_Compilation_Unit.

   --  The compilation context includes the explicit context of the
   --  subunit, and the context of the parent, together with the parent
   --  itself. In order to compile the current context, we remove the
   --  one inherited from the parent, in order to have a clean visibility
   --  table. We restore the parent context before analyzing the proper
   --  body itself. On exit, we remove only the explicit context of the
   --  subunit.

   procedure Analyze_Subunit (N : Node_Id) is
      Lib_Unit : constant Node_Id   := Library_Unit (N);
      Par_Unit : constant Entity_Id := Current_Scope;

      Lib_Spec        : Node_Id := Library_Unit (Lib_Unit);
      Num_Scopes      : Int := 0;
      Use_Clauses     : array (1 .. Scope_Stack.Last) of Node_Id;
      Enclosing_Child : Entity_Id := Empty;

      procedure Analyze_Subunit_Context;
      --  Capture names in use clauses of the subunit. This must be done
      --  before re-installing parent declarations, because items in the
      --  context must not be hidden by declarations local to the parent.

      procedure Re_Install_Parents (L : Node_Id; Scop : Entity_Id);
      --  Recursive procedure to restore scope of all ancestors of subunit,
      --  from outermost in. If parent is not a subunit, the call to install
      --  context installs context of spec and (if parent is a child unit)
      --  the context of its parents as well. It is confusing that parents
      --  should be treated differently in both cases, but the semantics are
      --  just not identical.

      procedure Re_Install_Use_Clauses;
      --  As part of the removal of the parent scope, the use clauses are
      --  removed, to be reinstalled when the context of the subunit has
      --  been analyzed. Use clauses may also have been affected by the
      --  analysis of the context of the subunit, so they have to be applied
      --  again, to insure that the compilation environment of the rest of
      --  the parent unit is identical.

      procedure Remove_Scope;
      --  Remove current scope from scope stack, and preserve the list
      --  of use clauses in it, to be reinstalled after context is analyzed.

      ------------------------------
      --  Analyze_Subunit_Context --
      ------------------------------

      procedure Analyze_Subunit_Context is
         Item      :  Node_Id;
         Nam       :  Node_Id;
         Unit_Name : Entity_Id;

      begin
         Analyze_Context (N);
         Item := First (Context_Items (N));

         --  make withed units immediately visible. If child unit, make the
         --  ultimate parent immediately visible.

         while Present (Item) loop

            if Nkind (Item) = N_With_Clause then
               Unit_Name := Entity (Name (Item));

               while Is_Child_Unit (Unit_Name) loop
                  Set_Is_Visible_Child_Unit (Unit_Name);
                  Unit_Name := Scope (Unit_Name);
               end loop;

               if not Is_Immediately_Visible (Unit_Name) then
                  Set_Is_Immediately_Visible (Unit_Name);
                  Set_Context_Installed (Item);
               end if;

            elsif Nkind (Item) = N_Use_Package_Clause then
               Nam := First (Names (Item));

               while Present (Nam) loop
                  Analyze (Nam);
                  Next (Nam);
               end loop;

            elsif Nkind (Item) = N_Use_Type_Clause then
               Nam := First (Subtype_Marks (Item));

               while Present (Nam) loop
                  Analyze (Nam);
                  Next (Nam);
               end loop;
            end if;

            Next (Item);
         end loop;

         Item := First (Context_Items (N));

         --  reset visibility of withed units. They will be made visible
         --  again when we install the subunit context.

         while Present (Item) loop

            if Nkind (Item) = N_With_Clause then
               Unit_Name := Entity (Name (Item));

               while Is_Child_Unit (Unit_Name) loop
                  Set_Is_Visible_Child_Unit (Unit_Name, False);
                  Unit_Name := Scope (Unit_Name);
               end loop;

               if Context_Installed (Item) then
                  Set_Is_Immediately_Visible (Unit_Name, False);
                  Set_Context_Installed (Item, False);
               end if;
            end if;

            Next (Item);
         end loop;

      end Analyze_Subunit_Context;

      ------------------------
      -- Re_Install_Parents --
      ------------------------

      procedure Re_Install_Parents (L : Node_Id; Scop : Entity_Id) is
         E : Entity_Id;

      begin
         if Nkind (Unit (L)) = N_Subunit then
            Re_Install_Parents (Library_Unit (L), Scope (Scop));
         end if;

         Install_Context (L);

         --  If the subunit occurs within a child unit, we must restore the
         --  immediate visibility of any siblings that may occur in context.

         if Present (Enclosing_Child) then
            Install_Siblings (Enclosing_Child, L);
         end if;

         New_Scope (Scop);

         if Scop /= Par_Unit then
            Set_Is_Immediately_Visible (Scop);
         end if;

         E := First_Entity (Current_Scope);

         while Present (E) loop
            Set_Is_Immediately_Visible (E);
            Next_Entity (E);
         end loop;

         --  A subunit appears within a body, and for a nested subunits
         --  all the parents are bodies. Restore full visibility of their
         --  private entities.

         if Ekind (Scop) = E_Package then
            Set_In_Package_Body (Scop);
            Install_Private_Declarations (Scop);
         end if;
      end Re_Install_Parents;

      ----------------------------
      -- Re_Install_Use_Clauses --
      ----------------------------

      procedure Re_Install_Use_Clauses is
         U  : Node_Id;

      begin
         for J in reverse 1 .. Num_Scopes loop
            U := Use_Clauses (J);
            Scope_Stack.Table (Scope_Stack.Last - J + 1).First_Use_Clause := U;
            Install_Use_Clauses (U);
         end loop;
      end Re_Install_Use_Clauses;

      ------------------
      -- Remove_Scope --
      ------------------

      procedure Remove_Scope is
         E : Entity_Id;

      begin
         Num_Scopes := Num_Scopes + 1;
         Use_Clauses (Num_Scopes) :=
               Scope_Stack.Table (Scope_Stack.Last).First_Use_Clause;
         E := First_Entity (Current_Scope);

         while Present (E) loop
            Set_Is_Immediately_Visible (E, False);
            Next_Entity (E);
         end loop;

         if Is_Child_Unit (Current_Scope) then
            Enclosing_Child := Current_Scope;
         end if;

         Pop_Scope;
      end Remove_Scope;

   --  Start of processing for Analyze_Subunit

   begin
      if not Is_Empty_List (Context_Items (N)) then

         --  Save current use clauses.

         Remove_Scope;
         Remove_Context (Lib_Unit);

         --  Now remove parents and their context, including enclosing
         --  subunits and the outer parent body which is not a subunit.

         if Present (Lib_Spec) then
            Remove_Context (Lib_Spec);

            while Nkind (Unit (Lib_Spec)) = N_Subunit loop
               Lib_Spec := Library_Unit (Lib_Spec);
               Remove_Scope;
               Remove_Context (Lib_Spec);
            end loop;

            if Nkind (Unit (Lib_Unit)) = N_Subunit then
               Remove_Scope;
            end if;

            if Nkind (Unit (Lib_Spec)) = N_Package_Body then
               Remove_Context (Library_Unit (Lib_Spec));
            end if;
         end if;

         Analyze_Subunit_Context;
         Re_Install_Parents (Lib_Unit, Par_Unit);

         --  If the context includes a child unit of the parent of the
         --  subunit, the parent will have been removed from visibility,
         --  after compiling that cousin in the context. The visibility
         --  of the parent must be restored now. This also applies if the
         --  context includes another subunit of the same parent which in
         --  turn includes a child unit in its context.

         if Ekind (Par_Unit) = E_Package then
            if not Is_Immediately_Visible (Par_Unit)
              or else (Present (First_Entity (Par_Unit))
                        and then not Is_Immediately_Visible
                                      (First_Entity (Par_Unit)))
            then
               Set_Is_Immediately_Visible   (Par_Unit);
               Install_Visible_Declarations (Par_Unit);
               Install_Private_Declarations (Par_Unit);
            end if;
         end if;

         Re_Install_Use_Clauses;
         Install_Context (N);

         --  If the subunit is within a child unit, then siblings of any
         --  parent unit that appear in the context clause of the subunit
         --  must also be made immediately visible.

         if Present (Enclosing_Child) then
            Install_Siblings (Enclosing_Child, N);
         end if;

      end if;

      Analyze (Proper_Body (Unit (N)));
      Remove_Context (N);

   end Analyze_Subunit;

   ----------------------------
   -- Analyze_Task_Body_Stub --
   ----------------------------

   procedure Analyze_Task_Body_Stub (N : Node_Id) is
      Nam : Entity_Id := Current_Entity_In_Scope (Defining_Identifier (N));
      Loc : constant Source_Ptr := Sloc (N);

   begin
      Check_Stub_Level (N);

      --  First occurrence of name may have been as an incomplete type.

      if Present (Nam) and then Ekind (Nam) = E_Incomplete_Type then
         Nam := Full_View (Nam);
      end if;

      if No (Nam)
        or else not Is_Task_Type (Etype (Nam))
      then
         Error_Msg_N ("missing specification for task body", N);
      else
         Set_Scope (Defining_Entity (N), Current_Scope);
         Set_Has_Completion (Etype (Nam));
         Analyze_Proper_Body (N, Etype (Nam));

         --  Set elaboration flag to indicate that entity is callable.
         --  This cannot be done in the expansion of the body  itself,
         --  because the proper body is not in a declarative part. This
         --  is only done if expansion is active, because the context
         --  may be generic and the flag not defined yet.

         if Expander_Active then
            Insert_After (N,
              Make_Assignment_Statement (Loc,
                Name =>
                  Make_Identifier (Loc,
                    New_External_Name (Chars (Etype (Nam)), 'E')),
                 Expression => New_Reference_To (Standard_True, Loc)));
         end if;

      end if;
   end Analyze_Task_Body_Stub;

   -------------------------
   -- Analyze_With_Clause --
   -------------------------

   --  Analyze the declaration of a unit in a with clause. At end,
   --  label the with clause with the defining entity for the unit.

   procedure Analyze_With_Clause (N : Node_Id) is
      Unit_Kind : constant Node_Kind := Nkind (Unit (Library_Unit (N)));
      E_Name    : Entity_Id;
      Par_Name  : Entity_Id;
      Pref      : Node_Id;
      U         : Node_Id;

      Intunit : Boolean;
      --  Set True if the unit currently being compiled is an internal unit

      Save_Style_Check : constant Boolean := Opt.Style_Check;
      Save_C_Restrict  : constant Save_Compilation_Unit_Restrictions :=
                           Compilation_Unit_Restrictions_Save;

   begin
      --  We reset ordinary style checking during the analysis of a with'ed
      --  unit, but we do NOT reset GNAT special analysis mode (the latter
      --  definitely *does* apply to with'ed units).

      if not GNAT_Mode then
         Style_Check := False;
      end if;

      --  If the library unit is a predefined unit, and we are in no
      --  run time mode, then temporarily reset No_Run_Time mode for the
      --  analysis of the with'ed unit. The No_Run_Time pragma does not
      --  prevent explicit with'ing of run-time units.

      if No_Run_Time
        and then
          Is_Predefined_File_Name
            (Unit_File_Name (Get_Source_Unit (Unit (Library_Unit (N)))))
      then
         No_Run_Time := False;
         Semantics (Library_Unit (N));
         No_Run_Time := True;

      else
         Semantics (Library_Unit (N));
      end if;

      U := Unit (Library_Unit (N));
      Intunit := Is_Internal_File_Name (Unit_File_Name (Current_Sem_Unit));

      --  Following checks are skipped for dummy packages (those supplied
      --  for with's where no matching file could be found). Such packages
      --  are identified by the Sloc value being set to No_Location

      if Sloc (U) /= No_Location then

         --  Check restrictions, except that we skip the check if this
         --  is an internal unit unless we are compiling the internal
         --  unit as the main unit. We also skip this for dummy packages.

         if not Intunit or else Current_Sem_Unit = Main_Unit then
            Check_Restricted_Unit (Unit_Name (Get_Source_Unit (U)), N);
         end if;

         --  Check for inappropriate with of internal implementation unit
         --  if we are currently compiling the main unit and the main unit
         --  is itself not an internal unit.

         if Implementation_Unit_Warnings
           and then Current_Sem_Unit = Main_Unit
           and then Implementation_Unit (Get_Source_Unit (U))
           and then not Intunit
         then
            Error_Msg_N ("& is an internal 'G'N'A'T unit?", Name (N));
            Error_Msg_N
              ("\use of this unit is non-portable and version-dependent?",
               Name (N));
         end if;
      end if;

      --  Semantic analysis of a generic unit is performed on a copy of
      --  the original tree. Retrieve the entity on  which semantic info
      --  actually appears.

      if Unit_Kind in N_Generic_Declaration then
         E_Name := Defining_Entity (U);

      --  Note: in the following test, Unit_Kind is the original Nkind, but
      --  in the case of an instantiation, semantic analysis above will
      --  have replaced the unit by its instantiated version. If the instance
      --  body has been generated, the instance now denotes the body entity.
      --  For visibility purposes we need the entity of its spec.

      elsif (Unit_Kind = N_Package_Instantiation
              or else Nkind (Original_Node (Unit (Library_Unit (N)))) =
                N_Package_Instantiation)
        and then Nkind (U) = N_Package_Body
      then
         E_Name := Corresponding_Spec (U);

      elsif Unit_Kind = N_Package_Instantiation
        and then Nkind (U) = N_Package_Instantiation
      then
         --  If the instance has not been rewritten as a package declaration,
         --  then it appeared already in a previous with clause. Retrieve
         --  the entity from the previous instance.

         E_Name := Defining_Entity (Specification (Instance_Spec (U)));

      elsif Unit_Kind = N_Procedure_Instantiation
        or else Unit_Kind = N_Function_Instantiation
      then
         --  Instantiation node is replaced with a package that contains
         --  renaming declarations and instance itself. The subprogram
         --  Instance is declared in the visible part of the wrapper package.

         E_Name := First_Entity (Defining_Entity (U));

         while Present (E_Name) loop
            exit when Is_Subprogram (E_Name)
              and then Is_Generic_Instance (E_Name);
            E_Name := Next_Entity (E_Name);
         end loop;

      elsif Unit_Kind = N_Package_Renaming_Declaration
        or else Unit_Kind in N_Generic_Renaming_Declaration
      then
         E_Name := Defining_Entity (U);

      elsif Unit_Kind = N_Subprogram_Body
        and then Nkind (Name (N)) = N_Selected_Component
        and then not Acts_As_Spec (Library_Unit (N))
      then
         --  For a child unit that has no spec, one has been created and
         --  analyzed. The entity required is that of the spec.

         E_Name := Corresponding_Spec (U);

      else
         E_Name := Defining_Entity (U);
      end if;

      if Nkind (Name (N)) = N_Selected_Component then

         --  Child unit in a with clause

         Change_Selected_Component_To_Expanded_Name (Name (N));
      end if;

      --  Restore style checks and restrictions

      Style_Check := Save_Style_Check;
      Compilation_Unit_Restrictions_Restore (Save_C_Restrict);

      --  Record the reference, but do NOT set the unit as referenced, we
      --  want to consider the unit as unreferenced if this is the only
      --  reference that occurs.

      Set_Entity_With_Style_Check (Name (N), E_Name);
      Generate_Reference (E_Name, Name (N), Set_Ref => False);

      if Is_Child_Unit (E_Name) then
         Pref     := Prefix (Name (N));
         Par_Name := Scope (E_Name);

         while Nkind (Pref) = N_Selected_Component loop
            Change_Selected_Component_To_Expanded_Name (Pref);
            Set_Entity_With_Style_Check (Pref, Par_Name);

            Generate_Reference (Par_Name, Pref);
            Pref := Prefix (Pref);
            Par_Name := Scope (Par_Name);
         end loop;

         if Present (Entity (Pref))
           and then not Analyzed (Parent (Parent (Entity (Pref))))
         then
            --  If the entity is set without its unit being compiled,
            --  the original parent is a renaming, and Par_Name is the
            --  renamed entity. For visibility purposes, we need the
            --  original entity, which must be analyzed now, because
            --  Load_Unit retrieves directly the renamed unit, and the
            --  renaming declaration itself has not been analyzed.

            Analyze (Parent (Parent (Entity (Pref))));
            pragma Assert (Renamed_Object (Entity (Pref)) = Par_Name);
            Par_Name := Entity (Pref);
         end if;

         Set_Entity_With_Style_Check (Pref, Par_Name);
         Generate_Reference (Par_Name, Pref);
      end if;

      --  If the withed unit is System, and a system extension pragma is
      --  present, compile the extension now, rather than waiting for
      --  a visibility check on a specific entity.

      if Chars (E_Name) = Name_System
        and then Scope (E_Name) = Standard_Standard
        and then Present (System_Extend_Pragma_Arg)
        and then Present_System_Aux (N)
      then
         --  If the extension is not present, an error will have been emitted.

         null;
      end if;
   end Analyze_With_Clause;

   ------------------------------
   -- Analyze_With_Type_Clause --
   ------------------------------

   procedure Analyze_With_Type_Clause (N : Node_Id) is
      Loc  : constant Source_Ptr := Sloc (N);
      Nam  : Node_Id := Name (N);
      Pack : Node_Id;
      Decl : Node_Id;
      P    : Entity_Id;
      Unum : Unit_Number_Type;
      Sel  : Node_Id;

      procedure Decorate_Tagged_Type (T : Entity_Id);
      --  Set basic attributes of type, including its class_wide type.

      function In_Chain (E : Entity_Id) return Boolean;
      --  Check that the imported type is not already in the homonym chain,
      --  for example through a with_type clause in a parent unit.

      --------------------------
      -- Decorate_Tagged_Type --
      --------------------------

      procedure Decorate_Tagged_Type (T : Entity_Id) is
         CW : Entity_Id;

      begin
         Set_Ekind (T, E_Record_Type);
         Set_Is_Tagged_Type (T);
         Set_Etype (T, T);
         Set_From_With_Type (T);
         Set_Scope (T, P);

         if not In_Chain (T) then
            Set_Homonym (T, Current_Entity (T));
            Set_Current_Entity (T);
         end if;

         --  Build bogus class_wide type, if not previously done.

         if No (Class_Wide_Type (T)) then
            CW := Make_Defining_Identifier (Loc,  New_Internal_Name ('S'));

            Set_Ekind            (CW, E_Class_Wide_Type);
            Set_Etype            (CW, T);
            Set_Scope            (CW, P);
            Set_Is_Tagged_Type   (CW);
            Set_Is_First_Subtype (CW, True);
            Init_Size_Align      (CW);
            Set_Has_Unknown_Discriminants
                                 (CW, True);
            Set_Class_Wide_Type  (CW, CW);
            Set_Equivalent_Type  (CW, Empty);
            Set_From_With_Type   (CW);

            Set_Class_Wide_Type (T, CW);
         end if;
      end Decorate_Tagged_Type;

      --------------
      -- In_Chain --
      --------------

      function In_Chain (E : Entity_Id) return Boolean is
         H : Entity_Id := Current_Entity (E);

      begin
         while Present (H) loop

            if H = E then
               return True;
            else
               H := Homonym (H);
            end if;
         end loop;

         return False;
      end In_Chain;

   --  Start of processing for Analyze_With_Type_Clause

   begin
      if Nkind (Nam) = N_Selected_Component then
         Pack := New_Copy_Tree (Prefix (Nam));
         Sel  := Selector_Name (Nam);

      else
         Error_Msg_N ("illegal name for imported type", Nam);
         return;
      end if;

      Decl :=
        Make_Package_Declaration (Loc,
          Specification =>
             (Make_Package_Specification (Loc,
               Defining_Unit_Name   => Pack,
               Visible_Declarations => New_List,
               End_Label            => Empty)));

      Unum :=
        Load_Unit
          (Load_Name  => Get_Unit_Name (Decl),
           Required   => True,
           Subunit    => False,
           Error_Node => Nam);

      if Unum = No_Unit
         or else Nkind (Unit (Cunit (Unum))) /= N_Package_Declaration
      then
         Error_Msg_N ("imported type must be declared in package", Nam);
         return;

      elsif Unum = Current_Sem_Unit then

         --  If type is defined in unit being analyzed, then the clause
         --  is redundant.

         return;

      else
         P := Cunit_Entity (Unum);
      end if;

      --  Find declaration for imported type, and set its basic attributes
      --  if it has not been analyzed (which will be the case if there is
      --  circular dependence).

      declare
         Decl : Node_Id;
         Typ  : Entity_Id;

      begin
         if not Analyzed (Cunit (Unum))
           and then not From_With_Type (P)
         then
            Set_Ekind (P, E_Package);
            Set_Etype (P, Standard_Void_Type);
            Set_From_With_Type (P);
            Set_Scope (P, Standard_Standard);
            Set_Homonym (P, Current_Entity (P));
            Set_Current_Entity (P);

         elsif Analyzed (Cunit (Unum))
           and then Is_Child_Unit (P)
         then
            --  If the child unit is already in scope, indicate that it is
            --  visible, and remains so after intervening calls to rtsfind.

            Set_Is_Visible_Child_Unit (P);
         end if;

         if Nkind (Parent (P)) = N_Defining_Program_Unit_Name then

            --  Make parent packages visible.

            declare
               Parent_Comp : Node_Id;
               Parent_Id   : Entity_Id;
               Child       : Entity_Id;

            begin
               Child   := P;
               Parent_Comp := Parent_Spec (Unit (Cunit (Unum)));

               loop
                  Parent_Id := Defining_Entity (Unit (Parent_Comp));
                  Set_Scope (Child, Parent_Id);

                  --  The type may be imported from a child unit, in which
                  --  case the current compilation appears in the name. Do
                  --  not change its visibility here because it will conflict
                  --  with the subsequent normal processing.

                  if not Analyzed (Unit_Declaration_Node (Parent_Id))
                    and then Parent_Id /= Cunit_Entity (Current_Sem_Unit)
                  then
                     Set_Ekind (Parent_Id, E_Package);
                     Set_Etype (Parent_Id, Standard_Void_Type);

                     --  The same package may appear is several with_type
                     --  clauses.

                     if not From_With_Type (Parent_Id) then
                        Set_Homonym (Parent_Id, Current_Entity (Parent_Id));
                        Set_Current_Entity (Parent_Id);
                        Set_From_With_Type (Parent_Id);
                     end if;
                  end if;

                  Set_Is_Immediately_Visible (Parent_Id);

                  Child := Parent_Id;
                  Parent_Comp := Parent_Spec (Unit (Parent_Comp));
                  exit when No (Parent_Comp);
               end loop;

               Set_Scope (Parent_Id, Standard_Standard);
            end;
         end if;

         --  Even if analyzed, the package may not be currently visible. It
         --  must be while the with_type clause is active.

         Set_Is_Immediately_Visible (P);

         Decl :=
           First (Visible_Declarations (Specification (Unit (Cunit (Unum)))));

         while Present (Decl) loop

            if Nkind (Decl) = N_Full_Type_Declaration
              and then Chars (Defining_Identifier (Decl)) = Chars (Sel)
            then
               Typ := Defining_Identifier (Decl);

               if Tagged_Present (N) then

                  --  The declaration must indicate that this is a tagged
                  --  type or a type extension.

                  if (Nkind (Type_Definition (Decl)) = N_Record_Definition
                       and then Tagged_Present (Type_Definition (Decl)))
                    or else
                      (Nkind (Type_Definition (Decl))
                          = N_Derived_Type_Definition
                         and then Present
                           (Record_Extension_Part (Type_Definition (Decl))))
                  then
                     null;
                  else
                     Error_Msg_N ("imported type is not a tagged type", Nam);
                     return;
                  end if;

                  if not Analyzed (Decl) then

                     --  Unit is not currently visible. Add basic attributes
                     --  to type and build its class-wide type.

                     Init_Size_Align (Typ);
                     Decorate_Tagged_Type (Typ);
                  end if;

               else
                  if Nkind (Type_Definition (Decl))
                     /= N_Access_To_Object_Definition
                  then
                     Error_Msg_N
                      ("imported type is not an access type", Nam);

                  elsif not Analyzed (Decl) then
                     Set_Ekind                    (Typ, E_Access_Type);
                     Set_Etype                    (Typ, Typ);
                     Set_Scope                    (Typ, P);
                     Init_Size                    (Typ, System_Address_Size);
                     Init_Alignment               (Typ);
                     Set_Directly_Designated_Type (Typ, Standard_Integer);
                     Set_From_With_Type           (Typ);

                     if not In_Chain (Typ) then
                        Set_Homonym               (Typ, Current_Entity (Typ));
                        Set_Current_Entity        (Typ);
                     end if;
                  end if;
               end if;

               Set_Entity (Sel, Typ);
               return;

            elsif ((Nkind (Decl) = N_Private_Type_Declaration
                      and then Tagged_Present (Decl))
                or else (Nkind (Decl) = N_Private_Extension_Declaration))
              and then Chars (Defining_Identifier (Decl)) = Chars (Sel)
            then
               Typ := Defining_Identifier (Decl);

               if not Tagged_Present (N) then
                  Error_Msg_N ("type must be declared tagged", N);

               elsif not Analyzed (Decl) then
                  Decorate_Tagged_Type (Typ);
               end if;

               Set_Entity (Sel, Typ);
               Set_From_With_Type (Typ);
               return;
            end if;

            Decl := Next (Decl);
         end loop;

         Error_Msg_NE ("not a visible access or tagged type in&", Nam, P);
      end;
   end Analyze_With_Type_Clause;

   -----------------------------
   -- Check_With_Type_Clauses --
   -----------------------------

   procedure Check_With_Type_Clauses (N : Node_Id) is
      Lib_Unit : constant Node_Id := Unit (N);

      procedure Check_Parent_Context (U : Node_Id);
      --  Examine context items of parent unit to locate with_type clauses.

      --------------------------
      -- Check_Parent_Context --
      --------------------------

      procedure Check_Parent_Context (U : Node_Id) is
         Item : Node_Id;

      begin
         Item := First (Context_Items (U));
         while Present (Item) loop
            if Nkind (Item) = N_With_Type_Clause
              and then not Error_Posted (Item)
              and then
                From_With_Type (Scope (Entity (Selector_Name (Name (Item)))))
            then
               Error_Msg_Sloc := Sloc (Item);
               Error_Msg_N ("Missing With_Clause for With_Type_Clause#", N);
            end if;

            Next (Item);
         end loop;
      end Check_Parent_Context;

   --  Start of processing for Check_With_Type_Clauses

   begin
      if Extensions_Allowed
        and then (Nkind (Lib_Unit) = N_Package_Body
                   or else Nkind (Lib_Unit) = N_Subprogram_Body)
      then
         Check_Parent_Context (Library_Unit (N));
         if Is_Child_Spec (Unit (Library_Unit (N))) then
            Check_Parent_Context (Parent_Spec (Unit (Library_Unit (N))));
         end if;
      end if;
   end Check_With_Type_Clauses;

   ------------------------------
   -- Check_Private_Child_Unit --
   ------------------------------

   procedure Check_Private_Child_Unit (N : Node_Id) is
      Lib_Unit   : constant Node_Id := Unit (N);
      Item       : Node_Id;
      Curr_Unit  : Entity_Id;
      Sub_Parent : Node_Id;
      Priv_Child : Entity_Id;
      Par_Lib    : Entity_Id;
      Par_Spec   : Node_Id;

      function Is_Private_Library_Unit (Unit : Entity_Id) return Boolean;
      --  Returns true if and only if the library unit is declared with
      --  an explicit designation of private.

      function Is_Private_Library_Unit (Unit : Entity_Id) return Boolean is
      begin
         return Private_Present (Parent (Unit_Declaration_Node (Unit)));
      end Is_Private_Library_Unit;

   --  Start of processing for Check_Private_Child_Unit

   begin
      if Nkind (Lib_Unit) = N_Package_Body
        or else Nkind (Lib_Unit) = N_Subprogram_Body
      then
         Curr_Unit := Defining_Entity (Unit (Library_Unit (N)));
         Par_Lib   := Curr_Unit;

      elsif Nkind (Lib_Unit) = N_Subunit then

         --  The parent is itself a body. The parent entity is to be found
         --  in the corresponding spec.

         Sub_Parent := Library_Unit (N);
         Curr_Unit  := Defining_Entity (Unit (Library_Unit (Sub_Parent)));

         --  If the parent itself is a subunit, Curr_Unit is the entity
         --  of the enclosing body, retrieve the spec entity which is
         --  the proper ancestor we need for the following tests.

         if Ekind (Curr_Unit) = E_Package_Body then
            Curr_Unit := Spec_Entity (Curr_Unit);
         end if;

         Par_Lib    := Curr_Unit;

      else
         Curr_Unit := Defining_Entity (Lib_Unit);

         Par_Lib := Curr_Unit;
         Par_Spec  := Parent_Spec (Lib_Unit);

         if No (Par_Spec) then
            Par_Lib := Empty;
         else
            Par_Lib := Defining_Entity (Unit (Par_Spec));
         end if;
      end if;

      --  Loop through context items

      Item := First (Context_Items (N));
      while Present (Item) loop

         if Nkind (Item) = N_With_Clause
            and then not Implicit_With (Item)
            and then Is_Private_Descendant (Entity (Name (Item)))
         then
            Priv_Child := Entity (Name (Item));

            declare
               Curr_Parent  : Entity_Id := Par_Lib;
               Child_Parent : Entity_Id := Scope (Priv_Child);
               Prv_Ancestor : Entity_Id := Child_Parent;
               Curr_Private : Boolean   := Is_Private_Library_Unit (Curr_Unit);

            begin
               --  If the child unit is a public child then locate
               --  the nearest private ancestor; Child_Parent will
               --  then be set to the parent of that ancestor.

               if not Is_Private_Library_Unit (Priv_Child) then
                  while Present (Prv_Ancestor)
                    and then not Is_Private_Library_Unit (Prv_Ancestor)
                  loop
                     Prv_Ancestor := Scope (Prv_Ancestor);
                  end loop;

                  if Present (Prv_Ancestor) then
                     Child_Parent := Scope (Prv_Ancestor);
                  end if;
               end if;

               while Present (Curr_Parent)
                 and then Curr_Parent /= Standard_Standard
                 and then Curr_Parent /= Child_Parent
               loop
                  Curr_Private :=
                    Curr_Private or else Is_Private_Library_Unit (Curr_Parent);
                  Curr_Parent := Scope (Curr_Parent);
               end loop;

               if not Present (Curr_Parent) then
                  Curr_Parent := Standard_Standard;
               end if;

               if Curr_Parent /= Child_Parent then

                  if Ekind (Priv_Child) = E_Generic_Package
                    and then Chars (Priv_Child) in Text_IO_Package_Name
                    and then Chars (Scope (Scope (Priv_Child))) = Name_Ada
                  then
                     Error_Msg_NE
                       ("& is a nested package, not a compilation unit",
                       Name (Item), Priv_Child);

                  else
                     Error_Msg_N
                       ("unit in with clause is private child unit!", Item);
                     Error_Msg_NE
                       ("current unit must also have parent&!",
                        Item, Child_Parent);
                  end if;

               elsif not Curr_Private
                 and then Nkind (Lib_Unit) /= N_Package_Body
                 and then Nkind (Lib_Unit) /= N_Subprogram_Body
                 and then Nkind (Lib_Unit) /= N_Subunit
               then
                  Error_Msg_NE
                    ("current unit must also be private descendant of&",
                     Item, Child_Parent);
               end if;
            end;
         end if;

         Next (Item);
      end loop;

   end Check_Private_Child_Unit;

   ----------------------
   -- Check_Stub_Level --
   ----------------------

   procedure Check_Stub_Level (N : Node_Id) is
      Par  : constant Node_Id   := Parent (N);
      Kind : constant Node_Kind := Nkind (Par);

   begin
      if (Kind = N_Package_Body
           or else Kind = N_Subprogram_Body
           or else Kind = N_Task_Body
           or else Kind = N_Protected_Body)

        and then (Nkind (Parent (Par)) = N_Compilation_Unit
                   or else Nkind (Parent (Par)) = N_Subunit)
      then
         null;

      --  In an instance, a missing stub appears at any level. A warning
      --  message will have been emitted already for the missing file.

      elsif not In_Instance then
         Error_Msg_N ("stub cannot appear in an inner scope", N);

      elsif Expander_Active then
         Error_Msg_N ("missing proper body", N);
      end if;
   end Check_Stub_Level;

   ------------------------
   -- Expand_With_Clause --
   ------------------------

   procedure Expand_With_Clause (Nam : Node_Id; N : Node_Id) is
      Loc   : constant Source_Ptr := Sloc (Nam);
      Ent   : constant Entity_Id := Entity (Nam);
      Withn : Node_Id;
      P     : Node_Id;

      function Build_Unit_Name (Nam : Node_Id) return Node_Id;

      function Build_Unit_Name (Nam : Node_Id) return Node_Id is
         Result : Node_Id;

      begin
         if Nkind (Nam) = N_Identifier then
            return New_Occurrence_Of (Entity (Nam), Loc);

         else
            Result :=
              Make_Expanded_Name (Loc,
                Chars  => Chars (Entity (Nam)),
                Prefix => Build_Unit_Name (Prefix (Nam)),
                Selector_Name => New_Occurrence_Of (Entity (Nam), Loc));
            Set_Entity (Result, Entity (Nam));
            return Result;
         end if;
      end Build_Unit_Name;

   begin
      New_Nodes_OK := New_Nodes_OK + 1;
      Withn :=
        Make_With_Clause (Loc, Name => Build_Unit_Name (Nam));

      P := Parent (Unit_Declaration_Node (Ent));
      Set_Library_Unit          (Withn, P);
      Set_Corresponding_Spec    (Withn, Ent);
      Set_First_Name            (Withn, True);
      Set_Implicit_With         (Withn, True);

      Prepend (Withn, Context_Items (N));
      Mark_Rewrite_Insertion (Withn);
      Install_Withed_Unit (Withn);

      if Nkind (Nam) = N_Expanded_Name then
         Expand_With_Clause (Prefix (Nam), N);
      end if;

      New_Nodes_OK := New_Nodes_OK - 1;
   end Expand_With_Clause;

   -----------------------
   -- Get_Parent_Entity --
   -----------------------

   function Get_Parent_Entity (Unit : Node_Id) return Entity_Id is
   begin
      if Nkind (Unit) = N_Package_Instantiation then
         return Defining_Entity (Specification (Instance_Spec (Unit)));
      else
         return Defining_Entity (Unit);
      end if;
   end Get_Parent_Entity;

   -----------------------------
   -- Implicit_With_On_Parent --
   -----------------------------

   procedure Implicit_With_On_Parent
     (Child_Unit : Node_Id;
      N          : Node_Id)
   is
      Loc    : constant Source_Ptr := Sloc (N);
      P      : constant Node_Id    := Parent_Spec (Child_Unit);
      P_Unit : constant Node_Id    := Unit (P);

      P_Name : Entity_Id := Get_Parent_Entity (P_Unit);
      Withn  : Node_Id;

      function Build_Ancestor_Name (P : Node_Id)  return Node_Id;
      --  Build prefix of child unit name. Recurse if needed.

      function Build_Unit_Name return Node_Id;
      --  If the unit is a child unit, build qualified name with all
      --  ancestors.

      -------------------------
      -- Build_Ancestor_Name --
      -------------------------

      function Build_Ancestor_Name (P : Node_Id) return Node_Id is
         P_Ref : Node_Id := New_Reference_To (Defining_Entity (P), Loc);

      begin
         if No (Parent_Spec (P)) then
            return P_Ref;
         else
            return
              Make_Selected_Component (Loc,
                Prefix => Build_Ancestor_Name (Unit (Parent_Spec (P))),
                Selector_Name => P_Ref);
         end if;
      end Build_Ancestor_Name;

      ---------------------
      -- Build_Unit_Name --
      ---------------------

      function Build_Unit_Name return Node_Id is
         Result : Node_Id;

      begin
         if No (Parent_Spec (P_Unit)) then
            return New_Reference_To (P_Name, Loc);
         else
            Result :=
              Make_Expanded_Name (Loc,
                Chars  => Chars (P_Name),
                Prefix => Build_Ancestor_Name (Unit (Parent_Spec (P_Unit))),
                Selector_Name => New_Reference_To (P_Name, Loc));
            Set_Entity (Result, P_Name);
            return Result;
         end if;
      end Build_Unit_Name;

   --  Start of processing for Implicit_With_On_Parent

   begin
      New_Nodes_OK := New_Nodes_OK + 1;
      Withn := Make_With_Clause (Loc, Name => Build_Unit_Name);

      Set_Library_Unit          (Withn, P);
      Set_Corresponding_Spec    (Withn, P_Name);
      Set_First_Name            (Withn, True);
      Set_Implicit_With         (Withn, True);

      --  Node is placed at the beginning of the context items, so that
      --  subsequent use clauses on the parent can be validated.

      Prepend (Withn, Context_Items (N));
      Mark_Rewrite_Insertion (Withn);
      Install_Withed_Unit (Withn);

      if Is_Child_Spec (P_Unit) then
         Implicit_With_On_Parent (P_Unit, N);
      end if;
      New_Nodes_OK := New_Nodes_OK - 1;
   end Implicit_With_On_Parent;

   ---------------------
   -- Install_Context --
   ---------------------

   procedure Install_Context (N : Node_Id) is
      Lib_Unit : Node_Id := Unit (N);

   begin
      Install_Context_Clauses (N);

      if Is_Child_Spec (Lib_Unit) then
         Install_Parents (Lib_Unit, Private_Present (Parent (Lib_Unit)));
      end if;

      Check_With_Type_Clauses (N);
   end Install_Context;

   -----------------------------
   -- Install_Context_Clauses --
   -----------------------------

   procedure Install_Context_Clauses (N : Node_Id) is
      Lib_Unit      : Node_Id := Unit (N);
      Item          : Node_Id;
      Uname_Node    : Entity_Id;
      Check_Private : Boolean := False;
      Decl_Node     : Node_Id;
      Lib_Parent    : Entity_Id;

   begin
      --  Loop through context clauses to find the with/use clauses

      Item := First (Context_Items (N));
      while Present (Item) loop

         --  Case of explicit WITH clause

         if Nkind (Item) = N_With_Clause
           and then not Implicit_With (Item)
         then
            --  If Name (Item) is not an entity name, something is wrong, and
            --  this will be detected in due course, for now ignore the item

            if not Is_Entity_Name (Name (Item)) then
               goto Continue;
            end if;

            Uname_Node := Entity (Name (Item));

            if Is_Private_Descendant (Uname_Node) then
               Check_Private := True;
            end if;

            Install_Withed_Unit (Item);

            Decl_Node := Unit_Declaration_Node (Uname_Node);

            --  If the unit is a subprogram instance, it appears nested
            --  within a package that carries the parent information.

            if Is_Generic_Instance (Uname_Node)
              and then Ekind (Uname_Node) /= E_Package
            then
               Decl_Node := Parent (Parent (Decl_Node));
            end if;

            if Is_Child_Spec (Decl_Node) then
               if Nkind (Name (Item)) = N_Expanded_Name then
                  Expand_With_Clause (Prefix (Name (Item)), N);
               else
                  --  if not an expanded name, the child unit must be a
                  --  renaming, nothing to do.

                  null;
               end if;

            elsif Nkind (Decl_Node) = N_Subprogram_Body
              and then not Acts_As_Spec (Parent (Decl_Node))
              and then Is_Child_Spec (Unit (Library_Unit (Parent (Decl_Node))))
            then
               Implicit_With_On_Parent
                 (Unit (Library_Unit (Parent (Decl_Node))), N);
            end if;

            --  Check license conditions unless this is a dummy unit

            if Sloc (Library_Unit (Item)) /= No_Location then
               License_Check : declare
                  Withl : constant License_Type :=
                            License (Source_Index
                                       (Get_Source_Unit
                                         (Library_Unit (Item))));

                  Unitl : constant License_Type :=
                           License (Source_Index (Current_Sem_Unit));

                  procedure License_Error;
                  --  Signal error of bad license

                  -------------------
                  -- License_Error --
                  -------------------

                  procedure License_Error is
                  begin
                     Error_Msg_N
                       ("?license of with'ed unit & is incompatible",
                        Name (Item));
                  end License_Error;

               --  Start of processing for License_Check

               begin
                  case Unitl is
                     when Unknown =>
                        null;

                     when Restricted =>
                        if Withl = GPL then
                           License_Error;
                        end if;

                     when GPL =>
                        if Withl = Restricted then
                           License_Error;
                        end if;

                     when Modified_GPL =>
                        if Withl = Restricted or else Withl = GPL then
                           License_Error;
                        end if;

                     when Unrestricted =>
                        null;
                  end case;
               end License_Check;
            end if;

         --  Case of USE PACKAGE clause

         elsif Nkind (Item) = N_Use_Package_Clause then
            Analyze_Use_Package (Item);

         --  Case of USE TYPE clause

         elsif Nkind (Item) = N_Use_Type_Clause then
            Analyze_Use_Type (Item);

         --  Case of WITH TYPE clause

         --  A With_Type_Clause is processed when installing the context,
         --  because it is a visibility mechanism and does not create a
         --  semantic dependence on other units, as a With_Clause does.

         elsif Nkind (Item) = N_With_Type_Clause then
            Analyze_With_Type_Clause (Item);

         --  case of PRAGMA

         elsif Nkind (Item) = N_Pragma then
            Analyze (Item);
         end if;

      <<Continue>>
         Next (Item);
      end loop;

      if Is_Child_Spec (Lib_Unit) then

         --  The unit also has implicit withs on its own parents.

         if No (Context_Items (N)) then
            Set_Context_Items (N, New_List);
         end if;

         Implicit_With_On_Parent (Lib_Unit, N);
      end if;

      --  If the unit is a body, the context of the specification must also
      --  be installed.

      if Nkind (Lib_Unit) = N_Package_Body
        or else (Nkind (Lib_Unit) = N_Subprogram_Body
                  and then not Acts_As_Spec (N))
      then
         Install_Context (Library_Unit (N));

         if Is_Child_Spec (Unit (Library_Unit (N))) then

            --  If the unit is the body of a public child unit, the private
            --  declarations of the parent must be made visible. If the child
            --  unit is private, the private declarations have been installed
            --  already in the call to Install_Parents for the spec. Installing
            --  private declarations must be done for all ancestors of public
            --  child units. In addition, sibling units mentioned in the
            --  context clause of the body are directly visible.

            declare
               Lib_Spec : Node_Id := Unit (Library_Unit (N));
               P        : Node_Id;
               P_Name   : Entity_Id;

            begin
               while Is_Child_Spec (Lib_Spec) loop
                  P := Unit (Parent_Spec (Lib_Spec));

                  if not (Private_Present (Parent (Lib_Spec))) then
                     P_Name := Defining_Entity (P);
                     Install_Private_Declarations (P_Name);
                     Set_Use (Private_Declarations (Specification (P)));
                  end if;

                  Lib_Spec := P;
               end loop;
            end;
         end if;

         --  For a package body, children in context are immediately visible

         Install_Siblings (Defining_Entity (Unit (Library_Unit (N))), N);
      end if;

      if Nkind (Lib_Unit) = N_Generic_Package_Declaration
        or else Nkind (Lib_Unit) = N_Generic_Subprogram_Declaration
        or else Nkind (Lib_Unit) = N_Package_Declaration
        or else Nkind (Lib_Unit) = N_Subprogram_Declaration
      then
         if Is_Child_Spec (Lib_Unit) then
            Lib_Parent := Defining_Entity (Unit (Parent_Spec (Lib_Unit)));
            Set_Is_Private_Descendant
              (Defining_Entity (Lib_Unit),
               Is_Private_Descendant (Lib_Parent)
                 or else Private_Present (Parent (Lib_Unit)));

         else
            Set_Is_Private_Descendant
              (Defining_Entity (Lib_Unit),
               Private_Present (Parent (Lib_Unit)));
         end if;
      end if;

      if Check_Private then
         Check_Private_Child_Unit (N);
      end if;
   end Install_Context_Clauses;

   ---------------------
   -- Install_Parents --
   ---------------------

   procedure Install_Parents (Lib_Unit : Node_Id; Is_Private : Boolean) is
      P      : Node_Id;
      E_Name : Entity_Id;
      P_Name : Entity_Id;
      P_Spec : Node_Id;

   begin
      P := Unit (Parent_Spec (Lib_Unit));
      P_Name := Get_Parent_Entity (P);

      if Etype (P_Name) = Any_Type then
         return;
      end if;

      if Ekind (P_Name) = E_Generic_Package
        and then Nkind (Lib_Unit) /= N_Generic_Subprogram_Declaration
        and then Nkind (Lib_Unit) /= N_Generic_Package_Declaration
        and then Nkind (Lib_Unit) not in N_Generic_Renaming_Declaration
      then
         Error_Msg_N
           ("child of a generic package must be a generic unit", Lib_Unit);

      elsif not Is_Package (P_Name) then
         Error_Msg_N
           ("parent unit must be package or generic package", Lib_Unit);
         raise Unrecoverable_Error;

      elsif Present (Renamed_Object (P_Name)) then
         Error_Msg_N ("parent unit cannot be a renaming", Lib_Unit);
         raise Unrecoverable_Error;

      --  Verify that a child of an instance is itself an instance, or
      --  the renaming of one. Given that an instance that is a unit is
      --  replaced with a package declaration, check against the original
      --  node.

      elsif Nkind (Original_Node (P)) = N_Package_Instantiation
        and then Nkind (Lib_Unit)
                   not in N_Renaming_Declaration
        and then Nkind (Original_Node (Lib_Unit))
                   not in N_Generic_Instantiation
      then
         Error_Msg_N
           ("child of an instance must be an instance or renaming", Lib_Unit);
      end if;

      --  This is the recursive call that ensures all parents are loaded

      if Is_Child_Spec (P) then
         Install_Parents (P,
           Is_Private or else Private_Present (Parent (Lib_Unit)));
      end if;

      --  Now we can install the context for this parent

      Install_Context_Clauses (Parent_Spec (Lib_Unit));
      Install_Siblings (P_Name, Parent (Lib_Unit));

      --  The child unit is in the declarative region of the parent. The
      --  parent must therefore appear in the scope stack and be visible,
      --  as when compiling the corresponding body. If the child unit is
      --  private or it is a package body, private declarations must be
      --  accessible as well. Use declarations in the parent must also
      --  be installed. Finally, other child units of the same parent that
      --  are in the context are immediately visible.

      --  Find entity for compilation unit, and set its private descendant
      --  status as needed.

      E_Name := Defining_Entity (Lib_Unit);

      Set_Is_Child_Unit (E_Name);

      Set_Is_Private_Descendant (E_Name,
         Is_Private_Descendant (P_Name)
           or else Private_Present (Parent (Lib_Unit)));

      P_Spec := Specification (Unit_Declaration_Node (P_Name));
      New_Scope (P_Name);

      --  Save current visibility of unit

      Scope_Stack.Table (Scope_Stack.Last).Previous_Visibility :=
        Is_Immediately_Visible (P_Name);
      Set_Is_Immediately_Visible (P_Name);
      Install_Visible_Declarations (P_Name);
      Set_Use (Visible_Declarations (P_Spec));

      if Is_Private
        or else Private_Present (Parent (Lib_Unit))
      then
         Install_Private_Declarations (P_Name);
         Set_Use (Private_Declarations (P_Spec));
      end if;
   end Install_Parents;

   ----------------------
   -- Install_Siblings --
   ----------------------

   procedure Install_Siblings (U_Name : Entity_Id; N : Node_Id) is
      Item : Node_Id;
      Id   : Entity_Id;
      Prev : Entity_Id;

      function Is_Ancestor (E : Entity_Id) return Boolean;
      --  Determine whether the scope of a child unit is an ancestor of
      --  the current unit.
      --  Shouldn't this be somewhere more general ???

      function Is_Ancestor (E : Entity_Id) return Boolean is
         Par : Entity_Id;

      begin
         Par := U_Name;

         while Present (Par)
           and then Par /= Standard_Standard
         loop

            if Par = E then
               return True;
            end if;

            Par := Scope (Par);
         end loop;

         return False;
      end Is_Ancestor;

   --  Start of processing for Install_Siblings

   begin
      --  Iterate over explicit with clauses, and check whether the
      --  scope of each entity is an ancestor of the current unit.

      Item := First (Context_Items (N));

      while Present (Item) loop

         if Nkind (Item) = N_With_Clause
           and then not Implicit_With (Item)
         then
            Id := Entity (Name (Item));

            if Is_Child_Unit (Id)
              and then Is_Ancestor (Scope (Id))
            then
               Set_Is_Immediately_Visible (Id);
               Prev := Current_Entity (Id);

               --  Check for the presence of another unit in the context,
               --  that may be inadvertently hidden by the child.

               if Present (Prev)
                 and then Is_Immediately_Visible (Prev)
                 and then not Is_Child_Unit (Prev)
               then
                  declare
                     Clause : Node_Id;

                  begin
                     Clause := First (Context_Items (N));

                     while Present (Clause) loop
                        if Nkind (Clause) = N_With_Clause
                          and then Entity (Name (Clause)) = Prev
                        then
                           Error_Msg_NE
                              ("child unit& hides compilation unit " &
                               "with the same name?",
                                 Name (Item), Id);
                           exit;
                        end if;

                        Next (Clause);
                     end loop;
                  end;
               end if;

            --  the With_Clause may be on a grand-child, which makes
            --  the child immediately visible.

            elsif Is_Child_Unit (Scope (Id))
              and then Is_Ancestor (Scope (Scope (Id)))
            then
               Set_Is_Immediately_Visible (Scope (Id));
            end if;
         end if;

         Next (Item);
      end loop;
   end Install_Siblings;

   -------------------------
   -- Install_Withed_Unit --
   -------------------------

   procedure Install_Withed_Unit (With_Clause : Node_Id) is
      Uname : Entity_Id := Entity (Name (With_Clause));
      P     : constant Entity_Id := Scope (Uname);

   begin
      --  We do not apply the restrictions to an internal unit unless
      --  we are compiling the internal unit as a main unit. This check
      --  is also skipped for dummy units (for missing packages).

      if Sloc (Uname) /= No_Location
        and then (not Is_Internal_File_Name (Unit_File_Name (Current_Sem_Unit))
                    or else Current_Sem_Unit = Main_Unit)
      then
         Check_Restricted_Unit
           (Unit_Name (Get_Source_Unit (Uname)), With_Clause);
      end if;

      if P /= Standard_Standard then

         --  If the unit is not analyzed after analysis of the with clause,
         --  and it is an instantiation, then it awaits a body and is the main
         --  unit. Its appearance in the context of some other unit indicates
         --  a circular dependency (DEC suite perversity).

         if not Analyzed (Uname)
           and then Nkind (Parent (Uname)) = N_Package_Instantiation
         then
            Error_Msg_N
              ("instantiation depends on itself", Name (With_Clause));

         elsif not Is_Visible_Child_Unit (Uname) then
            Set_Is_Visible_Child_Unit (Uname);

            if Is_Generic_Instance (Uname)
              and then Ekind (Uname) in Subprogram_Kind
            then
               --  Set flag as well on the visible entity that denotes the
               --  instance, which renames the current one.

               Set_Is_Visible_Child_Unit
                 (Related_Instance
                   (Defining_Entity (Unit (Library_Unit (With_Clause)))));
               null;
            end if;

            --  The parent unit may have been installed already, and
            --  may have appeared in a use clause.

            if In_Use (Scope (Uname)) then
               Set_Is_Potentially_Use_Visible (Uname);
            end if;

            Set_Context_Installed (With_Clause);
         end if;

      elsif not Is_Immediately_Visible (Uname) then
         Set_Is_Immediately_Visible (Uname);
         Set_Context_Installed (With_Clause);
      end if;

      --   A with-clause overrides a with-type clause: there are no restric-
      --   tions on the use of package entities.

      if Ekind (Uname) = E_Package then
         Set_From_With_Type (Uname, False);
      end if;
   end Install_Withed_Unit;

   -------------------
   -- Is_Child_Spec --
   -------------------

   function Is_Child_Spec (Lib_Unit : Node_Id) return Boolean is
      K : constant Node_Kind := Nkind (Lib_Unit);

   begin
      return (K in N_Generic_Declaration              or else
              K in N_Generic_Instantiation            or else
              K in N_Generic_Renaming_Declaration     or else
              K =  N_Package_Declaration              or else
              K =  N_Package_Renaming_Declaration     or else
              K =  N_Subprogram_Declaration           or else
              K =  N_Subprogram_Renaming_Declaration)
        and then Present (Parent_Spec (Lib_Unit));
   end Is_Child_Spec;

   -----------------------
   -- Load_Needed_Body --
   -----------------------

   --  N is a generic unit named in a with clause, or else it is
   --  a unit that contains a generic unit or an inlined function.
   --  In order to perform an instantiation, the body of the unit
   --  must be present. If the unit itself is generic, we assume
   --  that an instantiation follows, and  load and analyze the body
   --  unconditionally. This forces analysis of the spec as well.

   --  If the unit is not generic, but contains a generic unit, it
   --  is loaded on demand, at the point of instantiation (see ch12).

   procedure Load_Needed_Body (N : Node_Id; OK : out Boolean) is
      Body_Name : Unit_Name_Type;
      Unum      : Unit_Number_Type;

      Save_Style_Check : constant Boolean := Opt.Style_Check;
      --  The loading and analysis is done with style checks off

   begin
      if not GNAT_Mode then
         Style_Check := False;
      end if;

      Body_Name := Get_Body_Name (Get_Unit_Name (Unit (N)));
      Unum :=
        Load_Unit
          (Load_Name  => Body_Name,
           Required   => False,
           Subunit    => False,
           Error_Node => N,
           Renamings  => True);

      if Unum = No_Unit then
         OK := False;

      else
         Compiler_State := Analyzing; -- reset after load

         if not Fatal_Error (Unum) then
            if Debug_Flag_L then
               Write_Str ("*** Loaded generic body");
               Write_Eol;
            end if;

            Semantics (Cunit (Unum));
         end if;

         OK := True;
      end if;

      Style_Check := Save_Style_Check;
   end Load_Needed_Body;

   --------------------
   -- Remove_Context --
   --------------------

   procedure Remove_Context (N : Node_Id) is
      Lib_Unit : constant Node_Id := Unit (N);

   begin
      --  If this is a child unit, first remove the parent units.

      if Is_Child_Spec (Lib_Unit) then
         Remove_Parents (Lib_Unit);
      end if;

      Remove_Context_Clauses (N);
   end Remove_Context;

   ----------------------------
   -- Remove_Context_Clauses --
   ----------------------------

   procedure Remove_Context_Clauses (N : Node_Id) is
      Item      : Node_Id;
      Unit_Name : Entity_Id;

   begin

      --  Loop through context items and undo with_clauses and use_clauses.

      Item := First (Context_Items (N));

      while Present (Item) loop

         --  We are interested only in with clauses which got installed
         --  on entry, as indicated by their Context_Installed flag set

         if Nkind (Item) = N_With_Clause
            and then Context_Installed (Item)
         then
            --  Remove items from one with'ed unit

            Unit_Name := Entity (Name (Item));
            Remove_Unit_From_Visibility (Unit_Name);
            Set_Context_Installed (Item, False);

         elsif Nkind (Item) = N_Use_Package_Clause then
            End_Use_Package (Item);

         elsif Nkind (Item) = N_Use_Type_Clause then
            End_Use_Type (Item);

         elsif Nkind (Item) = N_With_Type_Clause then
            Remove_With_Type_Clause (Name (Item));
         end if;

         Next (Item);
      end loop;

   end Remove_Context_Clauses;

   --------------------
   -- Remove_Parents --
   --------------------

   procedure Remove_Parents (Lib_Unit : Node_Id) is
      P      : Node_Id;
      P_Name : Entity_Id;
      E      : Entity_Id;
      Vis    : constant Boolean :=
                 Scope_Stack.Table (Scope_Stack.Last).Previous_Visibility;

   begin
      if Is_Child_Spec (Lib_Unit) then
         P := Unit (Parent_Spec (Lib_Unit));
         P_Name := Defining_Entity (P);

         Remove_Context_Clauses (Parent_Spec (Lib_Unit));
         End_Package_Scope (P_Name);
         Set_Is_Immediately_Visible (P_Name, Vis);

         --  Remove from visibility the siblings as well, which are directly
         --  visible while the parent is in scope.

         E := First_Entity (P_Name);

         while Present (E) loop

            if Is_Child_Unit (E) then
               Set_Is_Immediately_Visible (E, False);
            end if;

            Next_Entity (E);
         end loop;

         Set_In_Package_Body (P_Name, False);

         --  This is the recursive call to remove the context of any
         --  higher level parent. This recursion ensures that all parents
         --  are removed in the reverse order of their installation.

         Remove_Parents (P);
      end if;
   end Remove_Parents;

   -----------------------------
   -- Remove_With_Type_Clause --
   -----------------------------

   procedure Remove_With_Type_Clause (Name : Node_Id) is
      Typ : Entity_Id;
      P   : Entity_Id;

      procedure Unchain (E : Entity_Id);
      --  Remove entity from visibility list.

      procedure Unchain (E : Entity_Id) is
         Prev : Entity_Id;

      begin
         Prev := Current_Entity (E);

         --  Package entity may appear is several with_type_clauses, and
         --  may have been removed already.

         if No (Prev) then
            return;

         elsif Prev = E then
            Set_Name_Entity_Id (Chars (E), Homonym (E));

         else
            while Present (Prev)
              and then Homonym (Prev) /= E
            loop
               Prev := Homonym (Prev);
            end loop;

            if (Present (Prev)) then
               Set_Homonym (Prev, Homonym (E));
            end if;
         end if;
      end Unchain;

   begin
      if Nkind (Name) = N_Selected_Component then
         Typ := Entity (Selector_Name (Name));

         if No (Typ) then    --  error in declaration.
            return;
         end if;
      else
         return;
      end if;

      P := Scope (Typ);

      --  If the exporting package has been analyzed, it has appeared in the
      --  context already and should be left alone. Otherwise, remove from
      --  visibility.

      if not Analyzed (Unit_Declaration_Node (P)) then
         Unchain (P);
         Unchain (Typ);
         Set_Is_Frozen (Typ, False);
      end if;

      if Ekind (Typ) = E_Record_Type then
         Set_From_With_Type (Class_Wide_Type (Typ), False);
         Set_From_With_Type (Typ, False);
      end if;

      Set_From_With_Type (P, False);

      --  If P is a child unit, remove parents as well.

      P := Scope (P);

      while Present (P)
        and then P /= Standard_Standard
      loop
         Set_From_With_Type (P, False);

         if not Analyzed (Unit_Declaration_Node (P)) then
            Unchain (P);
         end if;

         P := Scope (P);
      end loop;

      --  The back-end needs to know that an access type is imported, so it
      --  does not need elaboration and can appear in a mutually recursive
      --  record definition, so the imported flag on an access  type is
      --  preserved.

   end Remove_With_Type_Clause;

   ---------------------------------
   -- Remove_Unit_From_Visibility --
   ---------------------------------

   procedure Remove_Unit_From_Visibility (Unit_Name : Entity_Id) is
      P : Entity_Id := Scope (Unit_Name);

   begin

      if Debug_Flag_I then
         Write_Str ("remove withed unit ");
         Write_Name (Chars (Unit_Name));
         Write_Eol;
      end if;

      if P /= Standard_Standard then
         Set_Is_Visible_Child_Unit (Unit_Name, False);
      end if;

      Set_Is_Potentially_Use_Visible (Unit_Name, False);
      Set_Is_Immediately_Visible     (Unit_Name, False);

   end Remove_Unit_From_Visibility;

end Sem_Ch10;
