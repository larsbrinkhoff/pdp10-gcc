------------------------------------------------------------------------------
--                                                                          --
--                GNU ADA RUN-TIME LIBRARY (GNARL) COMPONENTS               --
--                                                                          --
--         S Y S T E M . T A S K I N G . T A S K _ A T T R I B U T E S      --
--                                                                          --
--                                  S p e c                                 --
--                                                                          --
--                                                                          --
--             Copyright (C) 1995-2002 Florida State University             --
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
-- now maintained by Ada Core Technologies, Inc. (http://www.gnat.com).     --
--                                                                          --
------------------------------------------------------------------------------

--  This package provides support for the body of Ada.Task_Attributes.

with Ada.Finalization;
--  used for Limited_Controlled

with System.Storage_Elements;
--  used for Integer_Address

package System.Tasking.Task_Attributes is

   type Attribute is new Integer;
   --  A stand-in for the generic formal type of Ada.Task_Attributes
   --  in the following declarations.

   type Node;
   type Access_Node is access all Node;
   --  This needs comments ???

   type Dummy_Wrapper;
   type Access_Dummy_Wrapper is access all Dummy_Wrapper;
   for Access_Dummy_Wrapper'Storage_Size use 0;
   --  This is a stand-in for the generic type Wrapper defined in
   --  Ada.Task_Attributes. The real objects allocated are always
   --  of type Wrapper, no Dummy_Wrapper objects are ever created.

   type Deallocator is access procedure (P : in out Access_Node);
   --  Called to deallocate an Wrapper. P is a pointer to a Node within.

   type Instance;

   type Access_Instance is access all Instance;

   type Instance is new Ada.Finalization.Limited_Controlled with record
      Deallocate    : Deallocator;
      Initial_Value : aliased System.Storage_Elements.Integer_Address;

      Index : Direct_Index;
      --  The index of the TCB location used by this instantiation,
      --  if it is stored in the TCB, otherwise zero.

      Next : Access_Instance;
      --  Next instance in All_Attributes list.
   end record;

   procedure Finalize (X : in out Instance);

   type Node is record
      Wrapper  : Access_Dummy_Wrapper;
      Instance : Access_Instance;
      Next     : Access_Node;
   end record;

   --  The following type is a stand-in for the actual
   --  wrapper type, which is different for each instantiation
   --  of Ada.Task_Attributes.

   type Dummy_Wrapper is record
      Noed : aliased Node;

      Value : aliased Attribute;
      --  The generic formal type, may be controlled
   end record;

   In_Use : Direct_Index_Vector := 0;
   --  is True for direct indices that are already used.

   All_Attributes : Access_Instance;
   --  A linked list of all indirectly access attributes,
   --  which includes all those that require finalization.

   procedure Initialize_Attributes (T : Task_ID);
   --  Initialize all attributes created via Ada.Task_Attributes for T.
   --  This must be called by the creator of the task, inside Create_Task,
   --  via soft-link Initialize_Attributes_Link. On entry, abortion must
   --  be deferred and the caller must hold no locks

   procedure Finalize_Attributes (T : Task_ID);
   --  Finalize all attributes created via Ada.Task_Attributes for T.
   --  This is to be called by the task after it is marked as terminated
   --  (and before it actually dies), inside Vulnerable_Free_Task, via the
   --  soft-link Finalize_Attributes_Link. On entry, abortion must be deferred
   --  and T.L must be write-locked.

end System.Tasking.Task_Attributes;
