------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                    S Y S T E M . S O F T _ L I N K S                     --
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
-- As a special exception,  if other files  instantiate  generics from this --
-- unit, or you link  this unit with other files  to produce an executable, --
-- this  unit  does not  by itself cause  the resulting  executable  to  be --
-- covered  by the  GNU  General  Public  License.  This exception does not --
-- however invalidate  any other reasons why  the executable file  might be --
-- covered by the  GNU Public License.                                      --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- It is now maintained by Ada Core Technologies Inc (http://www.gnat.com). --
--                                                                          --
------------------------------------------------------------------------------

pragma Polling (Off);
--  We must turn polling off for this unit, because otherwise we get
--  an infinite loop from the code within the Poll routine itself.

with System.Machine_State_Operations; use System.Machine_State_Operations;
--  Used for Create_TSD, Destroy_TSD

with System.Parameters;
--  Used for Sec_Stack_Ratio

with System.Secondary_Stack;

package body System.Soft_Links is

   package SST renames System.Secondary_Stack;

   --  Allocate an exception stack for the main program to use.
   --  We make sure that the stack has maximum alignment. Some systems require
   --  this (e.g. Sun), and in any case it is a good idea for efficiency.

   NT_Exc_Stack : array (0 .. 8192) of aliased Character;
   for NT_Exc_Stack'Alignment use Standard'Maximum_Alignment;

   NT_TSD : TSD;

   --------------------
   -- Abort_Defer_NT --
   --------------------

   procedure Abort_Defer_NT is
   begin
      null;
   end Abort_Defer_NT;

   ----------------------
   -- Abort_Handler_NT --
   ----------------------

   procedure Abort_Handler_NT is
   begin
      null;
   end Abort_Handler_NT;

   ----------------------
   -- Abort_Undefer_NT --
   ----------------------

   procedure Abort_Undefer_NT is
   begin
      null;
   end Abort_Undefer_NT;

   ---------------------------
   -- Check_Abort_Status_NT --
   ---------------------------

   function Check_Abort_Status_NT return Integer is
   begin
      return Boolean'Pos (False);
   end Check_Abort_Status_NT;

   ------------------------
   -- Complete_Master_NT --
   ------------------------

   procedure Complete_Master_NT is
   begin
      null;
   end Complete_Master_NT;

   ----------------
   -- Create_TSD --
   ----------------

   procedure Create_TSD (New_TSD : in out TSD) is
      use type Parameters.Size_Type;

      SS_Ratio_Dynamic : constant Boolean :=
                           Parameters.Sec_Stack_Ratio = Parameters.Dynamic;

   begin
      if SS_Ratio_Dynamic then
         SST.SS_Init
           (New_TSD.Sec_Stack_Addr, SST.Default_Secondary_Stack_Size);
      end if;

      New_TSD.Machine_State_Addr :=
        System.Address
          (System.Machine_State_Operations.Allocate_Machine_State);
   end Create_TSD;

   -----------------------
   -- Current_Master_NT --
   -----------------------

   function Current_Master_NT return Integer is
   begin
      return 0;
   end Current_Master_NT;

   -----------------
   -- Destroy_TSD --
   -----------------

   procedure Destroy_TSD (Old_TSD : in out TSD) is
   begin
      SST.SS_Free (Old_TSD.Sec_Stack_Addr);
      System.Machine_State_Operations.Free_Machine_State
        (Machine_State (Old_TSD.Machine_State_Addr));
   end Destroy_TSD;

   ---------------------
   -- Enter_Master_NT --
   ---------------------

   procedure Enter_Master_NT is
   begin
      null;
   end Enter_Master_NT;

   --------------------------
   -- Get_Current_Excep_NT --
   --------------------------

   function Get_Current_Excep_NT return EOA is
   begin
      return NT_TSD.Current_Excep'Access;
   end Get_Current_Excep_NT;

   ---------------------------
   -- Get_Exc_Stack_Addr_NT --
   ---------------------------

   function Get_Exc_Stack_Addr_NT return Address is
   begin
      return NT_TSD.Exc_Stack_Addr;
   end Get_Exc_Stack_Addr_NT;

   -----------------------------
   -- Get_Exc_Stack_Addr_Soft --
   -----------------------------

   function Get_Exc_Stack_Addr_Soft return  Address is
   begin
      return Get_Exc_Stack_Addr.all;
   end Get_Exc_Stack_Addr_Soft;

   ------------------------
   -- Get_GNAT_Exception --
   ------------------------

   function Get_GNAT_Exception return Ada.Exceptions.Exception_Id is
   begin
      return Ada.Exceptions.Exception_Identity (Get_Current_Excep.all.all);
   end Get_GNAT_Exception;

   ---------------------------
   -- Get_Jmpbuf_Address_NT --
   ---------------------------

   function Get_Jmpbuf_Address_NT return  Address is
   begin
      return NT_TSD.Jmpbuf_Address;
   end Get_Jmpbuf_Address_NT;

   -----------------------------
   -- Get_Jmpbuf_Address_Soft --
   -----------------------------

   function Get_Jmpbuf_Address_Soft return  Address is
   begin
      return Get_Jmpbuf_Address.all;
   end Get_Jmpbuf_Address_Soft;

   -------------------------------
   -- Get_Machine_State_Addr_NT --
   -------------------------------

   function Get_Machine_State_Addr_NT return  Address is
   begin
      return NT_TSD.Machine_State_Addr;
   end Get_Machine_State_Addr_NT;

   ---------------------------------
   -- Get_Machine_State_Addr_Soft --
   ---------------------------------

   function Get_Machine_State_Addr_Soft return  Address is
   begin
      return Get_Machine_State_Addr.all;
   end Get_Machine_State_Addr_Soft;

   ---------------------------
   -- Get_Sec_Stack_Addr_NT --
   ---------------------------

   function Get_Sec_Stack_Addr_NT return  Address is
   begin
      return NT_TSD.Sec_Stack_Addr;
   end Get_Sec_Stack_Addr_NT;

   -----------------------------
   -- Get_Sec_Stack_Addr_Soft --
   -----------------------------

   function Get_Sec_Stack_Addr_Soft return  Address is
   begin
      return Get_Sec_Stack_Addr.all;
   end Get_Sec_Stack_Addr_Soft;

   -----------------------
   -- Get_Stack_Info_NT --
   -----------------------

   function Get_Stack_Info_NT return Stack_Checking.Stack_Access is
   begin
      return NT_TSD.Pri_Stack_Info'Access;
   end Get_Stack_Info_NT;

   -------------------
   -- Null_Adafinal --
   -------------------

   procedure Null_Adafinal is
   begin
      null;
   end Null_Adafinal;

   ---------------------------
   -- Set_Exc_Stack_Addr_NT --
   ---------------------------

   procedure Set_Exc_Stack_Addr_NT (Self_ID : Address; Addr : Address) is
      pragma Warnings (Off, Self_ID);

   begin
      NT_TSD.Exc_Stack_Addr := Addr;
   end Set_Exc_Stack_Addr_NT;

   -----------------------------
   -- Set_Exc_Stack_Addr_Soft --
   -----------------------------

   procedure Set_Exc_Stack_Addr_Soft (Self_ID : Address; Addr : Address) is
   begin
      Set_Exc_Stack_Addr (Self_ID, Addr);
   end Set_Exc_Stack_Addr_Soft;

   ---------------------------
   -- Set_Jmpbuf_Address_NT --
   ---------------------------

   procedure Set_Jmpbuf_Address_NT (Addr : Address) is
   begin
      NT_TSD.Jmpbuf_Address := Addr;
   end Set_Jmpbuf_Address_NT;

   procedure Set_Jmpbuf_Address_Soft (Addr : Address) is
   begin
      Set_Jmpbuf_Address (Addr);
   end Set_Jmpbuf_Address_Soft;

   -------------------------------
   -- Set_Machine_State_Addr_NT --
   -------------------------------

   procedure Set_Machine_State_Addr_NT (Addr : Address) is
   begin
      NT_TSD.Machine_State_Addr := Addr;
   end Set_Machine_State_Addr_NT;

   ---------------------------------
   -- Set_Machine_State_Addr_Soft --
   ---------------------------------

   procedure Set_Machine_State_Addr_Soft (Addr : Address) is
   begin
      Set_Machine_State_Addr (Addr);
   end Set_Machine_State_Addr_Soft;

   ---------------------------
   -- Set_Sec_Stack_Addr_NT --
   ---------------------------

   procedure Set_Sec_Stack_Addr_NT (Addr : Address) is
   begin
      NT_TSD.Sec_Stack_Addr := Addr;
   end Set_Sec_Stack_Addr_NT;

   -----------------------------
   -- Set_Sec_Stack_Addr_Soft --
   -----------------------------

   procedure Set_Sec_Stack_Addr_Soft (Addr : Address) is
   begin
      Set_Sec_Stack_Addr (Addr);
   end Set_Sec_Stack_Addr_Soft;

   ------------------
   -- Task_Lock_NT --
   ------------------

   procedure Task_Lock_NT is
   begin
      null;
   end Task_Lock_NT;

   --------------------
   -- Task_Unlock_NT --
   --------------------

   procedure Task_Unlock_NT is
   begin
      null;
   end Task_Unlock_NT;

   -------------------------
   -- Update_Exception_NT --
   -------------------------

   procedure Update_Exception_NT (X : EO := Current_Target_Exception) is
   begin
      Ada.Exceptions.Save_Occurrence (NT_TSD.Current_Excep, X);
   end Update_Exception_NT;

   ------------------
   -- Task_Name_NT --
   -------------------

   function Task_Name_NT return String is
   begin
      return "main_task";
   end Task_Name_NT;

   -------------------------
   -- Package Elaboration --
   -------------------------

begin
   NT_TSD.Exc_Stack_Addr := NT_Exc_Stack (8192)'Address;
   Ada.Exceptions.Save_Occurrence
     (NT_TSD.Current_Excep, Ada.Exceptions.Null_Occurrence);

end System.Soft_Links;
