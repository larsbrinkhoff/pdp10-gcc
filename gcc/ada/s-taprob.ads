------------------------------------------------------------------------------
--                                                                          --
--                GNU ADA RUN-TIME LIBRARY (GNARL) COMPONENTS               --
--                                                                          --
--      S Y S T E M . T A S K I N G . P R O T E C T E D _ O B J E C T S     --
--                                                                          --
--                                  S p e c                                 --
--                                                                          --
--                                                                          --
--          Copyright (C) 1992-2001, Free Software Foundation, Inc.         --
--                                                                          --
-- GNARL is free software; you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 2,  or (at your option) any later ver- --
-- sion. GNARL is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNARL; see file COPYING.  If not, write --
-- to  the Free Software Foundation,  59 Temple Place - Suite 330,  Boston, --
-- MA 02111-1307, USA.                                                      --
--                                                                          --
-- As a special exception,  if other files  instantiate  generics from this --
-- unit, or you link  this unit with other files  to produce an executable, --
-- this  unit  does not  by itself cause  the resulting  executable  to  be --
-- covered  by the  GNU  General  Public  License.  This exception does not --
-- however invalidate  any other reasons why  the executable file  might be --
-- covered by the  GNU Public License.                                      --
--                                                                          --
-- GNARL was developed by the GNARL team at Florida State University. It is --
-- now maintained by Ada Core Technologies Inc. in cooperation with Florida --
-- State University (http://www.gnat.com).                                  --
--                                                                          --
------------------------------------------------------------------------------

--  This package provides necessary definitions to handle simple (i.e without
--  entries) protected objects.
--
--  All the routines that handle protected objects with entries have been moved
--  to two children: Entries and Operations. Note that Entries only contains
--  the type declaration and the OO primitives. This is needed to avoid
--  circular dependency.

--  This package is part of the high level tasking interface used by the
--  compiler to expand Ada 95 tasking constructs into simpler run time calls
--  (aka GNARLI, GNU Ada Run-time Library Interface)

--  Note: the compiler generates direct calls to this interface, via Rtsfind.
--  Any changes to this interface may require corresponding compiler changes
--  in exp_ch9.adb and possibly exp_ch7.adb

package System.Tasking.Protected_Objects is
   pragma Elaborate_Body;

   ---------------------------------
   -- Compiler Interface (GNARLI) --
   ---------------------------------

   --  The compiler will expand in the GNAT tree the following construct:
   --
   --  protected PO is
   --     procedure P;
   --  private
   --     open : boolean := false;
   --  end PO;
   --
   --  protected body PO is
   --     procedure P is
   --        ...variable declarations...
   --     begin
   --        ...B...
   --     end P;
   --  end PO;
   --
   --  as follows:
   --
   --  protected type poT is
   --     procedure p;
   --  private
   --     open : boolean := false;
   --  end poT;
   --  type poTV is limited record
   --     open : boolean := false;
   --     _object : aliased protection;
   --  end record;
   --  procedure poPT__pN (_object : in out poTV);
   --  procedure poPT__pP (_object : in out poTV);
   --  freeze poTV [
   --     procedure _init_proc (_init : in out poTV) is
   --     begin
   --        _init.open := false;
   --        _init_proc (_init._object);
   --        initialize_protection (_init._object'unchecked_access,
   --          unspecified_priority);
   --        return;
   --     end _init_proc;
   --  ]
   --  po : poT;
   --  _init_proc (poTV!(po));
   --
   --  procedure poPT__pN (_object : in out poTV) is
   --     poR : protection renames _object._object;
   --     openP : boolean renames _object.open;
   --     ...variable declarations...
   --  begin
   --     ...B...
   --     return;
   --  end poPT__pN;
   --
   --  procedure poPT__pP (_object : in out poTV) is
   --     procedure _clean is
   --     begin
   --        unlock (_object._object'unchecked_access);
   --        return;
   --     end _clean;
   --  begin
   --     lock (_object._object'unchecked_access);
   --     B2b : begin
   --        poPT__pN (_object);
   --     at end
   --        _clean;
   --     end B2b;
   --     return;
   --  end poPT__pP;

   Null_Protected_Entry : constant := Null_Entry;

   Max_Protected_Entry : constant := Max_Entry;

   type Protected_Entry_Index is new Entry_Index
     range Null_Protected_Entry .. Max_Protected_Entry;

   type Barrier_Function_Pointer is access
     function
       (O    : System.Address;
        E    : Protected_Entry_Index)
        return Boolean;
   --  Pointer to a function which evaluates the barrier of a protected
   --  entry body. O is a pointer to the compiler-generated record
   --  representing the protected object, and E is the index of the
   --  entry serviced by the body.

   type Entry_Action_Pointer is access
     procedure
       (O : System.Address;
        P : System.Address;
        E : Protected_Entry_Index);
   --  Pointer to a procedure which executes the sequence of statements
   --  of a protected entry body. O is a pointer to the compiler-generated
   --  record representing the protected object, P is a pointer to the
   --  record of entry parameters, and E is the index of the
   --  entry serviced by the body.

   type Entry_Body is record
      Barrier : Barrier_Function_Pointer;
      Action  : Entry_Action_Pointer;
   end record;
   --  The compiler-generated code passes objects of this type to the GNARL
   --  to allow it to access the executable code of an entry body.

   type Entry_Body_Access is access all Entry_Body;

   type Protection is limited private;
   --  This type contains the GNARL state of a protected object. The
   --  application-defined portion of the state (i.e. private objects)
   --  is maintained by the compiler-generated code.
   --  Note that there are now 2 Protection types. One for the simple
   --  case (no entries) and one for the general case that needs the whole
   --  Finalization mechanism.
   --  This split helps in the case of restricted run time where we want to
   --  minimize the size of the code.

   type Protection_Access is access all Protection;

   Null_PO : constant Protection_Access := null;

   procedure Initialize_Protection
     (Object           : Protection_Access;
      Ceiling_Priority : Integer);
   --  Initialize the Object parameter so that it can be used by the runtime
   --  to keep track of the runtime state of a protected object.

   procedure Lock (Object : Protection_Access);
   --  Lock a protected object for write access. Upon return, the caller
   --  owns the lock to this object, and no other call to Lock or
   --  Lock_Read_Only with the same argument will return until the
   --  corresponding call to Unlock has been made by the caller.

   procedure Lock_Read_Only (Object : Protection_Access);
   --  Lock a protected object for read access. Upon return, the caller
   --  owns the lock for read access, and no other calls to Lock with the
   --  same argument will return until the corresponding call to Unlock
   --  has been made by the caller. Other calls to Lock_Read_Only may (but
   --  need not) return before the call to Unlock, and the corresponding
   --  callers will also own the lock for read access.
   --
   --  Note: we are not currently using this interface, it is provided
   --  for possible future use. At the current time, everyone uses Lock
   --  for both read and write locks.

   procedure Unlock (Object : Protection_Access);
   --  Relinquish ownership of the lock for the object represented by
   --  the Object parameter. If this ownership was for write access, or
   --  if it was for read access where there are no other read access
   --  locks outstanding, one (or more, in the case of Lock_Read_Only)
   --  of the tasks waiting on this lock (if any) will be given the
   --  lock and allowed to return from the Lock or Lock_Read_Only call.

private
   type Protection is record
      L       : aliased Task_Primitives.Lock;
      Ceiling : System.Any_Priority;
   end record;
   pragma Volatile (Protection);
   for Protection'Alignment use Standard'Maximum_Alignment;
   --  Needed so that we can uncheck convert a Protection_Access to a
   --  Protection_Entries_Access.

   procedure Finalize_Protection (Object : in out Protection);
   --  Clean up a Protection object; in particular, finalize the associated
   --  Lock object. The compiler generates automatically calls to this
   --  procedure

end System.Tasking.Protected_Objects;
