------------------------------------------------------------------------------
--                                                                          --
--                GNU ADA RUN-TIME LIBRARY (GNARL) COMPONENTS               --
--                                                                          --
--     S Y S T E M . T A S K _ P R I M I T I V E S . O P E R A T I O N S    --
--                                                                          --
--                                  B o d y                                 --
--                                                                          --
--                                                                          --
--         Copyright (C) 1992-2001, Free Software Foundation, Inc.          --
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

--  This is a no tasking version of this package

--  This package contains all the GNULL primitives that interface directly
--  with the underlying OS.

pragma Polling (Off);
--  Turn off polling, we do not want ATC polling to take place during
--  tasking operations. It causes infinite loops and other problems.

with System.Tasking;
--  used for Ada_Task_Control_Block
--           Task_ID

with System.OS_Primitives;
--  used for Delay_Modes

with System.Error_Reporting;
--  used for Shutdown

package body System.Task_Primitives.Operations is

   use System.Tasking;
   use System.Parameters;
   use System.OS_Primitives;

   -----------------
   -- Stack_Guard --
   -----------------

   procedure Stack_Guard (T : ST.Task_ID; On : Boolean) is
   begin
      null;
   end Stack_Guard;

   --------------------
   -- Get_Thread_Id  --
   --------------------

   function Get_Thread_Id (T : ST.Task_ID) return OSI.Thread_Id is
   begin
      return OSI.Thread_Id (T.Common.LL.Thread);
   end Get_Thread_Id;

   ----------
   -- Self --
   ----------

   function Self return Task_ID is
   begin
      return Null_Task;
   end Self;

   ---------------------
   -- Initialize_Lock --
   ---------------------

   procedure Initialize_Lock
     (Prio : System.Any_Priority;
      L    : access Lock) is
   begin
      null;
   end Initialize_Lock;

   procedure Initialize_Lock (L : access RTS_Lock; Level : Lock_Level) is
   begin
      null;
   end Initialize_Lock;

   -------------------
   -- Finalize_Lock --
   -------------------

   procedure Finalize_Lock (L : access Lock) is
   begin
      null;
   end Finalize_Lock;

   procedure Finalize_Lock (L : access RTS_Lock) is
   begin
      null;
   end Finalize_Lock;

   ----------------
   -- Write_Lock --
   ----------------

   procedure Write_Lock (L : access Lock; Ceiling_Violation : out Boolean) is
   begin
      Ceiling_Violation := False;
   end Write_Lock;

   procedure Write_Lock
     (L : access RTS_Lock; Global_Lock : Boolean := False)
   is
   begin
      null;
   end Write_Lock;

   procedure Write_Lock (T : Task_ID) is
   begin
      null;
   end Write_Lock;

   ---------------
   -- Read_Lock --
   ---------------

   procedure Read_Lock (L : access Lock; Ceiling_Violation : out Boolean) is
   begin
      Ceiling_Violation := False;
   end Read_Lock;

   ------------
   -- Unlock --
   ------------

   procedure Unlock (L : access Lock) is
   begin
      null;
   end Unlock;

   procedure Unlock (L : access RTS_Lock; Global_Lock : Boolean := False) is
   begin
      null;
   end Unlock;

   procedure Unlock (T : Task_ID) is
   begin
      null;
   end Unlock;

   -----------
   -- Sleep --
   -----------

   procedure Sleep (Self_ID : Task_ID; Reason  : System.Tasking.Task_States) is
   begin
      null;
   end Sleep;

   -----------------
   -- Timed_Sleep --
   -----------------

   procedure Timed_Sleep
     (Self_ID  : Task_ID;
      Time     : Duration;
      Mode     : ST.Delay_Modes;
      Reason   : System.Tasking.Task_States;
      Timedout : out Boolean;
      Yielded  : out Boolean) is
   begin
      Timedout := False;
      Yielded := False;
   end Timed_Sleep;

   -----------------
   -- Timed_Delay --
   -----------------

   procedure Timed_Delay
     (Self_ID : Task_ID;
      Time    : Duration;
      Mode    : ST.Delay_Modes) is
   begin
      null;
   end Timed_Delay;

   ---------------------
   -- Monotonic_Clock --
   ---------------------

   function Monotonic_Clock return Duration is
   begin
      return 0.0;
   end Monotonic_Clock;

   -------------------
   -- RT_Resolution --
   -------------------

   function RT_Resolution return Duration is
   begin
      return 10#1.0#E-6;
   end RT_Resolution;

   ------------
   -- Wakeup --
   ------------

   procedure Wakeup (T : Task_ID; Reason : System.Tasking.Task_States) is
   begin
      null;
   end Wakeup;

   ------------------
   -- Set_Priority --
   ------------------

   procedure Set_Priority
     (T                   : Task_ID;
      Prio                : System.Any_Priority;
      Loss_Of_Inheritance : Boolean := False) is
   begin
      null;
   end Set_Priority;

   ------------------
   -- Get_Priority --
   ------------------

   function Get_Priority (T : Task_ID) return System.Any_Priority is
   begin
      return 0;
   end Get_Priority;

   ----------------
   -- Enter_Task --
   ----------------

   procedure Enter_Task (Self_ID : Task_ID) is
   begin
      null;
   end Enter_Task;

   --------------
   -- New_ATCB --
   --------------

   function New_ATCB (Entry_Num : Task_Entry_Index) return Task_ID is
   begin
      return new Ada_Task_Control_Block (Entry_Num);
   end New_ATCB;

   ----------------------
   --  Initialize_TCB  --
   ----------------------

   procedure Initialize_TCB (Self_ID : Task_ID; Succeeded : out Boolean) is
   begin
      Succeeded := False;
   end Initialize_TCB;

   -----------------
   -- Create_Task --
   -----------------

   procedure Create_Task
     (T          : Task_ID;
      Wrapper    : System.Address;
      Stack_Size : System.Parameters.Size_Type;
      Priority   : System.Any_Priority;
      Succeeded  : out Boolean) is
   begin
      Succeeded := False;
   end Create_Task;

   ------------------
   -- Finalize_TCB --
   ------------------

   procedure Finalize_TCB (T : Task_ID) is
   begin
      null;
   end Finalize_TCB;

   ---------------
   -- Exit_Task --
   ---------------

   procedure Exit_Task is
   begin
      null;
   end Exit_Task;

   ----------------
   -- Abort_Task --
   ----------------

   procedure Abort_Task (T : Task_ID) is
   begin
      null;
   end Abort_Task;

   -----------
   -- Yield --
   -----------

   procedure Yield (Do_Yield : Boolean := True) is
   begin
      null;
   end Yield;

   ----------------
   -- Check_Exit --
   ----------------

   --  Dummy versions.  The only currently working versions is for solaris
   --  (native).

   function Check_Exit (Self_ID : ST.Task_ID) return Boolean is
   begin
      return True;
   end Check_Exit;

   --------------------
   -- Check_No_Locks --
   --------------------

   function Check_No_Locks (Self_ID : ST.Task_ID) return Boolean is
   begin
      return True;
   end Check_No_Locks;

   ----------------------
   -- Environment_Task --
   ----------------------

   function Environment_Task return Task_ID is
   begin
      return null;
   end Environment_Task;

   --------------
   -- Lock_RTS --
   --------------

   procedure Lock_RTS is
   begin
      null;
   end Lock_RTS;

   ----------------
   -- Unlock_RTS --
   ----------------

   procedure Unlock_RTS is
   begin
      null;
   end Unlock_RTS;

   ------------------
   -- Suspend_Task --
   ------------------

   function Suspend_Task
     (T           : ST.Task_ID;
      Thread_Self : OSI.Thread_Id) return Boolean is
   begin
      return False;
   end Suspend_Task;

   -----------------
   -- Resume_Task --
   -----------------

   function Resume_Task
     (T           : ST.Task_ID;
      Thread_Self : OSI.Thread_Id) return Boolean is
   begin
      return False;
   end Resume_Task;

   ----------------
   -- Initialize --
   ----------------

   procedure Initialize (Environment_Task : Task_ID) is
   begin
      null;
   end Initialize;

   No_Tasking : Boolean;

begin
   --  Can't raise an exception because target independent packages try to
   --  do an Abort_Defer, which gets a memory fault.

   No_Tasking :=
     System.Error_Reporting.Shutdown
       ("Tasking not implemented on this configuration");
end System.Task_Primitives.Operations;
