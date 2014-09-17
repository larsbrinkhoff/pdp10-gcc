------------------------------------------------------------------------------
--                                                                          --
--                GNU ADA RUN-TIME LIBRARY (GNARL) COMPONENTS               --
--                                                                          --
--                 S Y S T E M . T A S K I N G . S T A G E S                --
--                                                                          --
--                                  B o d y                                 --
--                                                                          --
--                                                                          --
--         Copyright (C) 1992-2002, Free Software Foundation, Inc.          --
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

pragma Polling (Off);
--  Turn off polling, we do not want ATC polling to take place during
--  tasking operations. It causes infinite loops and other problems.

with Ada.Exceptions;
--  used for Raise_Exception

with System.Tasking.Debug;
pragma Warnings (Off, System.Tasking.Debug);
--  used for enabling tasking facilities with gdb

with System.Address_Image;
--  used for the function itself.

with System.Parameters;
--  used for Size_Type
--           Single_Lock
--           Runtime_Traces

with System.Task_Info;
--  used for Task_Info_Type
--           Task_Image_Type

with System.Task_Primitives.Operations;
--  used for Finalize_Lock
--           Enter_Task
--           Write_Lock
--           Unlock
--           Sleep
--           Wakeup
--           Get_Priority
--           Lock/Unlock_RTS
--           New_ATCB

with System.Soft_Links;
--  These are procedure pointers to non-tasking routines that use
--  task specific data. In the absence of tasking, these routines
--  refer to global data. In the presense of tasking, they must be
--  replaced with pointers to task-specific versions.
--  Also used for Create_TSD, Destroy_TSD, Get_Current_Excep

with System.Tasking.Initialization;
--  Used for Remove_From_All_Tasks_List
--           Defer_Abort
--           Undefer_Abort
--           Initialization.Poll_Base_Priority_Change
--           Finalize_Attributes_Link
--           Initialize_Attributes_Link

pragma Elaborate_All (System.Tasking.Initialization);
--  This insures that tasking is initialized if any tasks are created.

with System.Tasking.Utilities;
--  Used for Make_Passive
--           Abort_One_Task

with System.Tasking.Queuing;
--  Used for Dequeue_Head

with System.Tasking.Rendezvous;
--  Used for Call_Simple

with System.OS_Primitives;
--  Used for Delay_Modes

with System.Finalization_Implementation;
--  Used for System.Finalization_Implementation.Finalize_Global_List

with Interfaces.C;
--  Used for type Unsigned.

with System.Secondary_Stack;
--  used for SS_Init;

with System.Storage_Elements;
--  used for Storage_Array;

with System.Standard_Library;
--  used for Exception_Trace

with System.Traces.Tasking;
--  used for Send_Trace_Info

package body System.Tasking.Stages is

   package STPO renames System.Task_Primitives.Operations;
   package SSL  renames System.Soft_Links;
   package SSE  renames System.Storage_Elements;
   package SST  renames System.Secondary_Stack;

   use Ada.Exceptions;

   use Parameters;
   use Task_Primitives;
   use Task_Primitives.Operations;
   use Task_Info;

   use System.Traces;
   use System.Traces.Tasking;

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Notify_Exception
     (Self_Id : Task_ID;
      Excep   : Exception_Occurrence);
   --  This procedure will output the task ID and the exception information,
   --  including traceback if available.

   procedure Task_Wrapper (Self_ID : Task_ID);
   --  This is the procedure that is called by the GNULL from the
   --  new context when a task is created. It waits for activation
   --  and then calls the task body procedure. When the task body
   --  procedure completes, it terminates the task.

   procedure Vulnerable_Complete_Task (Self_ID : Task_ID);
   --  Complete the calling task.
   --  This procedure must be called with abort deferred.
   --  It should only be called by Complete_Task and
   --  Finalizate_Global_Tasks (for the environment task).

   procedure Vulnerable_Complete_Master (Self_ID : Task_ID);
   --  Complete the current master of the calling task.
   --  This procedure must be called with abort deferred.
   --  It should only be called by Vulnerable_Complete_Task and
   --  Complete_Master.

   procedure Vulnerable_Complete_Activation (Self_ID : Task_ID);
   --  Signal to Self_ID's activator that Self_ID has
   --  completed activation.
   --
   --  Call this procedure with abort deferred.

   procedure Abort_Dependents (Self_ID : Task_ID);
   --  Abort all the direct dependents of Self at its current master
   --  nesting level, plus all of their dependents, transitively.
   --  RTS_Lock should be locked by the caller.

   procedure Vulnerable_Free_Task (T : Task_ID);
   --  Recover all runtime system storage associated with the task T.
   --  This should only be called after T has terminated and will no
   --  longer be referenced.
   --
   --  For tasks created by an allocator that fails, due to an exception,
   --  it is called from Expunge_Unactivated_Tasks.
   --
   --  It is also called from Unchecked_Deallocation, for objects that
   --  are or contain tasks.
   --
   --  Different code is used at master completion, in Terminate_Dependents,
   --  due to a need for tighter synchronization with the master.

   procedure Terminate_Task (Self_ID : Task_ID);
   --  Terminate the calling task.
   --  This should only be called by the Task_Wrapper procedure.

   ----------------------
   -- Abort_Dependents --
   ----------------------

   procedure Abort_Dependents (Self_ID : Task_ID) is
      C : Task_ID;
      P : Task_ID;

   begin
      C := All_Tasks_List;

      while C /= null loop
         P := C.Common.Parent;

         while P /= null loop
            if P = Self_ID then
               --  ??? C is supposed to take care of its own dependents, so
               --  there should be no need to worry about them. Need to double
               --  check this.

               if C.Master_of_Task = Self_ID.Master_Within then
                  Utilities.Abort_One_Task (Self_ID, C);
                  C.Dependents_Aborted := True;
               end if;

               exit;
            end if;

            P := P.Common.Parent;
         end loop;

         C := C.Common.All_Tasks_Link;
      end loop;

      Self_ID.Dependents_Aborted := True;
   end Abort_Dependents;

   -----------------
   -- Abort_Tasks --
   -----------------

   procedure Abort_Tasks (Tasks : Task_List) is
   begin
      Utilities.Abort_Tasks (Tasks);
   end Abort_Tasks;

   --------------------
   -- Activate_Tasks --
   --------------------

   --  Note that locks of activator and activated task are both locked
   --  here. This is necessary because C.Common.State and
   --  Self.Common.Wait_Count have to be synchronized. This is safe from
   --  deadlock because the activator is always created before the activated
   --  task. That satisfies our in-order-of-creation ATCB locking policy.

   --  At one point, we may also lock the parent, if the parent is
   --  different from the activator. That is also consistent with the
   --  lock ordering policy, since the activator cannot be created
   --  before the parent.

   --  Since we are holding both the activator's lock, and Task_Wrapper
   --  locks that before it does anything more than initialize the
   --  low-level ATCB components, it should be safe to wait to update
   --  the counts until we see that the thread creation is successful.

   --  If the thread creation fails, we do need to close the entries
   --  of the task. The first phase, of dequeuing calls, only requires
   --  locking the acceptor's ATCB, but the waking up of the callers
   --  requires locking the caller's ATCB. We cannot safely do this
   --  while we are holding other locks. Therefore, the queue-clearing
   --  operation is done in a separate pass over the activation chain.

   procedure Activate_Tasks (Chain_Access : Activation_Chain_Access) is
      Self_ID        : constant Task_ID := STPO.Self;
      P              : Task_ID;
      C              : Task_ID;
      Next_C, Last_C : Task_ID;
      Activate_Prio  : System.Any_Priority;
      Success        : Boolean;
      All_Elaborated : Boolean := True;

   begin
      pragma Debug
        (Debug.Trace (Self_ID, "Activate_Tasks", 'C'));

      Initialization.Defer_Abort_Nestable (Self_ID);

      pragma Assert (Self_ID.Common.Wait_Count = 0);

      --  Lock RTS_Lock, to prevent activated tasks
      --  from racing ahead before we finish activating the chain.

      Lock_RTS;

      --  Check that all task bodies have been elaborated.

      C := Chain_Access.T_ID;
      Last_C := null;

      while C /= null loop
         if C.Common.Elaborated /= null
           and then not C.Common.Elaborated.all
         then
            All_Elaborated := False;
         end if;

         --  Reverse the activation chain so that tasks are
         --  activated in the same order they're declared.

         Next_C := C.Common.Activation_Link;
         C.Common.Activation_Link := Last_C;
         Last_C := C;
         C := Next_C;
      end loop;

      Chain_Access.T_ID := Last_C;

      if not All_Elaborated then
         Unlock_RTS;
         Initialization.Undefer_Abort_Nestable (Self_ID);
         Raise_Exception
           (Program_Error'Identity, "Some tasks have not been elaborated");
      end if;

      --  Activate all the tasks in the chain.
      --  Creation of the thread of control was deferred until
      --  activation. So create it now.

      C := Chain_Access.T_ID;

      while C /= null loop
         if C.Common.State /= Terminated then
            pragma Assert (C.Common.State = Unactivated);

            P := C.Common.Parent;
            Write_Lock (P);
            Write_Lock (C);

            if C.Common.Base_Priority < Get_Priority (Self_ID) then
               Activate_Prio := Get_Priority (Self_ID);
            else
               Activate_Prio := C.Common.Base_Priority;
            end if;

            System.Task_Primitives.Operations.Create_Task
              (C, Task_Wrapper'Address,
               Parameters.Size_Type
                 (C.Common.Compiler_Data.Pri_Stack_Info.Size),
               Activate_Prio, Success);

            --  There would be a race between the created task and
            --  the creator to do the following initialization,
            --  if we did not have a Lock/Unlock_RTS pair
            --  in the task wrapper, to prevent it from racing ahead.

            if Success then
               C.Common.State := Runnable;
               C.Awake_Count := 1;
               C.Alive_Count := 1;
               P.Awake_Count := P.Awake_Count + 1;
               P.Alive_Count := P.Alive_Count + 1;

               if P.Common.State = Master_Completion_Sleep and then
                 C.Master_of_Task = P.Master_Within
               then
                  pragma Assert (Self_ID /= P);
                  P.Common.Wait_Count := P.Common.Wait_Count + 1;
               end if;

               Unlock (C);
               Unlock (P);

            else
               --  No need to set Awake_Count, State, etc. here since the loop
               --  below will do that for any Unactivated tasks.

               Unlock (C);
               Unlock (P);
               Self_ID.Common.Activation_Failed := True;
            end if;
         end if;

         C := C.Common.Activation_Link;
      end loop;

      if not Single_Lock then
         Unlock_RTS;
      end if;

      --  Close the entries of any tasks that failed thread creation,
      --  and count those that have not finished activation.

      Write_Lock (Self_ID);
      Self_ID.Common.State := Activator_Sleep;

      C :=  Chain_Access.T_ID;
      while C /= null loop
         Write_Lock (C);

         if C.Common.State = Unactivated then
            C.Common.Activator := null;
            C.Common.State := Terminated;
            C.Callable := False;
            Utilities.Cancel_Queued_Entry_Calls (C);

         elsif C.Common.Activator /= null then
            Self_ID.Common.Wait_Count := Self_ID.Common.Wait_Count + 1;
         end if;

         Unlock (C);
         P := C.Common.Activation_Link;
         C.Common.Activation_Link := null;
         C := P;
      end loop;

      --  Wait for the activated tasks to complete activation.
      --  It is unsafe to abort any of these tasks until the count goes to
      --  zero.

      loop
         Initialization.Poll_Base_Priority_Change (Self_ID);
         exit when Self_ID.Common.Wait_Count = 0;
         Sleep (Self_ID, Activator_Sleep);
      end loop;

      Self_ID.Common.State := Runnable;
      Unlock (Self_ID);

      if Single_Lock then
         Unlock_RTS;
      end if;

      --  Remove the tasks from the chain.

      Chain_Access.T_ID := null;
      Initialization.Undefer_Abort_Nestable (Self_ID);

      if Self_ID.Common.Activation_Failed then
         Self_ID.Common.Activation_Failed := False;
         Raise_Exception (Tasking_Error'Identity,
           "Failure during activation");
      end if;
   end Activate_Tasks;

   -------------------------
   -- Complete_Activation --
   -------------------------

   procedure Complete_Activation is
      Self_ID : constant Task_ID := STPO.Self;
   begin
      Initialization.Defer_Abort_Nestable (Self_ID);

      if Single_Lock then
         Lock_RTS;
      end if;

      Vulnerable_Complete_Activation (Self_ID);

      if Single_Lock then
         Unlock_RTS;
      end if;

      Initialization.Undefer_Abort_Nestable (Self_ID);

      --  ???
      --  Why do we need to allow for nested deferral here?

      if Runtime_Traces then
         Send_Trace_Info (T_Activate);
      end if;
   end Complete_Activation;

   ---------------------
   -- Complete_Master --
   ---------------------

   procedure Complete_Master is
      Self_ID : Task_ID := STPO.Self;

   begin
      pragma Assert (Self_ID.Deferral_Level > 0);

      Vulnerable_Complete_Master (Self_ID);
   end Complete_Master;

   -------------------
   -- Complete_Task --
   -------------------

   --  See comments on Vulnerable_Complete_Task for details.

   procedure Complete_Task is
      Self_ID  : constant Task_ID := STPO.Self;
   begin
      pragma Assert (Self_ID.Deferral_Level > 0);

      Vulnerable_Complete_Task (Self_ID);

      --  All of our dependents have terminated.
      --  Never undefer abort again!
   end Complete_Task;

   -----------------
   -- Create_Task --
   -----------------

   --  Compiler interface only. Do not call from within the RTS.
   --  This must be called to create a new task.

   procedure Create_Task
     (Priority      : Integer;
      Size          : System.Parameters.Size_Type;
      Task_Info     : System.Task_Info.Task_Info_Type;
      Num_Entries   : Task_Entry_Index;
      Master        : Master_Level;
      State         : Task_Procedure_Access;
      Discriminants : System.Address;
      Elaborated    : Access_Boolean;
      Chain         : in out Activation_Chain;
      Task_Image    : System.Task_Info.Task_Image_Type;
      Created_Task  : out Task_ID)
   is
      T, P          : Task_ID;
      Self_ID       : constant Task_ID := STPO.Self;
      Success       : Boolean;
      Base_Priority : System.Any_Priority;

   begin
      pragma Debug
        (Debug.Trace (Self_ID, "Create_Task", 'C'));

      if Priority = Unspecified_Priority then
         Base_Priority := Self_ID.Common.Base_Priority;
      else
         Base_Priority := System.Any_Priority (Priority);
      end if;

      --  Find parent P of new Task, via master level number.

      P := Self_ID;

      if P /= null then
         while P.Master_of_Task >= Master loop
            P := P.Common.Parent;
            exit when P = null;
         end loop;
      end if;

      Initialization.Defer_Abort_Nestable (Self_ID);

      begin
         T := New_ATCB (Num_Entries);

      exception
         when others =>
            Initialization.Undefer_Abort_Nestable (Self_ID);
            Raise_Exception (Storage_Error'Identity, "Cannot allocate task");
      end;

      --  RTS_Lock is used by Abort_Dependents and Abort_Tasks.
      --  Up to this point, it is possible that we may be part of
      --  a family of tasks that is being aborted.

      Lock_RTS;
      Write_Lock (Self_ID);

      --  Now, we must check that we have not been aborted.
      --  If so, we should give up on creating this task,
      --  and simply return.

      if not Self_ID.Callable then
         pragma Assert (Self_ID.Pending_ATC_Level = 0);
         pragma Assert (Self_ID.Pending_Action);
         pragma Assert (Chain.T_ID = null
             or else Chain.T_ID.Common.State = Unactivated);

         Unlock (Self_ID);
         Unlock_RTS;
         Initialization.Undefer_Abort_Nestable (Self_ID);

         --  ??? Should never get here

         pragma Assert (False);
         raise Standard'Abort_Signal;
      end if;

      Initialize_ATCB (Self_ID, State, Discriminants, P, Elaborated,
        Base_Priority, Task_Info, Size, T, Success);

      if not Success then
         Unlock (Self_ID);
         Unlock_RTS;
         Initialization.Undefer_Abort_Nestable (Self_ID);
         Raise_Exception
           (Storage_Error'Identity, "Failed to initialize task");
      end if;

      T.Master_of_Task := Master;
      T.Master_Within := T.Master_of_Task + 1;

      for L in T.Entry_Calls'Range loop
         T.Entry_Calls (L).Self := T;
         T.Entry_Calls (L).Level := L;
      end loop;

      T.Common.Task_Image := Task_Image;
      Unlock (Self_ID);
      Unlock_RTS;

      --  Create TSD as early as possible in the creation of a task, since it
      --  may be used by the operation of Ada code within the task.

      SSL.Create_TSD (T.Common.Compiler_Data);
      T.Common.Activation_Link := Chain.T_ID;
      Chain.T_ID := T;
      Initialization.Initialize_Attributes_Link.all (T);
      Created_Task := T;
      Initialization.Undefer_Abort_Nestable (Self_ID);

      if Runtime_Traces then
         Send_Trace_Info (T_Create, T);
      end if;
   end Create_Task;

   --------------------
   -- Current_Master --
   --------------------

   function Current_Master return Master_Level is
   begin
      return STPO.Self.Master_Within;
   end Current_Master;

   ------------------
   -- Enter_Master --
   ------------------

   procedure Enter_Master is
      Self_ID : constant Task_ID := STPO.Self;

   begin
      Self_ID.Master_Within := Self_ID.Master_Within + 1;
   end Enter_Master;

   -------------------------------
   -- Expunge_Unactivated_Tasks --
   -------------------------------

   --  See procedure Close_Entries for the general case.

   procedure Expunge_Unactivated_Tasks (Chain : in out Activation_Chain) is
      Self_ID : constant Task_ID := STPO.Self;
      C       : Task_ID;
      Call    : Entry_Call_Link;
      Temp    : Task_ID;

   begin
      pragma Debug
        (Debug.Trace (Self_ID, "Expunge_Unactivated_Tasks", 'C'));

      Initialization.Defer_Abort_Nestable (Self_ID);

      --  ???
      --  Experimentation has shown that abort is sometimes (but not
      --  always) already deferred when this is called.
      --  That may indicate an error. Find out what is going on.

      C := Chain.T_ID;

      while C /= null loop
         pragma Assert (C.Common.State = Unactivated);

         Temp := C.Common.Activation_Link;

         if C.Common.State = Unactivated then
            Lock_RTS;
            Write_Lock (C);

            for J in 1 .. C.Entry_Num loop
               Queuing.Dequeue_Head (C.Entry_Queues (J), Call);
               pragma Assert (Call = null);
            end loop;

            Unlock (C);

            Initialization.Remove_From_All_Tasks_List (C);
            Unlock_RTS;

            Vulnerable_Free_Task (C);
            C := Temp;
         end if;
      end loop;

      Chain.T_ID := null;
      Initialization.Undefer_Abort_Nestable (Self_ID);
   end Expunge_Unactivated_Tasks;

   ---------------------------
   -- Finalize_Global_Tasks --
   ---------------------------

   --  ???
   --  We have a potential problem here if finalization of global
   --  objects does anything with signals or the timer server, since
   --  by that time those servers have terminated.

   --  It is hard to see how that would occur.

   --  However, a better solution might be to do all this finalization
   --  using the global finalization chain.

   procedure Finalize_Global_Tasks is
      Self_ID : constant Task_ID := STPO.Self;
      Ignore  : Boolean;

   begin
      if Self_ID.Deferral_Level = 0 then
         --  ???
         --  In principle, we should be able to predict whether
         --  abort is already deferred here (and it should not be deferred
         --  yet but in practice it seems Finalize_Global_Tasks is being
         --  called sometimes, from RTS code for exceptions, with abort already
         --  deferred.

         Initialization.Defer_Abort_Nestable (Self_ID);

         --  Never undefer again!!!
      end if;

      --  This code is only executed by the environment task

      pragma Assert (Self_ID = Environment_Task);

      --  Set Environment_Task'Callable to false to notify library-level tasks
      --  that it is waiting for them (cf 5619-003).

      Self_ID.Callable := False;

      --  Exit level 2 master, for normal tasks in library-level packages.

      Complete_Master;

      --  Force termination of "independent" library-level server tasks.

      Lock_RTS;

      Abort_Dependents (Self_ID);

      if not Single_Lock then
         Unlock_RTS;
      end if;

      --  We need to explicitly wait for the task to be
      --  terminated here because on true concurrent system, we
      --  may end this procedure before the tasks are really
      --  terminated.

      Write_Lock (Self_ID);

      loop
         exit when Utilities.Independent_Task_Count = 0;

         --  We used to yield here, but this did not take into account
         --  low priority tasks that would cause dead lock in some cases.
         --  See 8126-020.

         Timed_Sleep
           (Self_ID, 0.01, System.OS_Primitives.Relative,
            Self_ID.Common.State, Ignore, Ignore);
      end loop;

      --  ??? On multi-processor environments, it seems that the above loop
      --  isn't sufficient, so we need to add an additional delay.

      Timed_Sleep
        (Self_ID, 0.01, System.OS_Primitives.Relative,
         Self_ID.Common.State, Ignore, Ignore);

      Unlock (Self_ID);

      if Single_Lock then
         Unlock_RTS;
      end if;

      --  Complete the environment task.

      Vulnerable_Complete_Task (Self_ID);

      System.Finalization_Implementation.Finalize_Global_List;

      SSL.Abort_Defer        := SSL.Abort_Defer_NT'Access;
      SSL.Abort_Undefer      := SSL.Abort_Undefer_NT'Access;
      SSL.Lock_Task          := SSL.Task_Lock_NT'Access;
      SSL.Unlock_Task        := SSL.Task_Unlock_NT'Access;
      SSL.Get_Jmpbuf_Address := SSL.Get_Jmpbuf_Address_NT'Access;
      SSL.Set_Jmpbuf_Address := SSL.Set_Jmpbuf_Address_NT'Access;
      SSL.Get_Sec_Stack_Addr := SSL.Get_Sec_Stack_Addr_NT'Access;
      SSL.Set_Sec_Stack_Addr := SSL.Set_Sec_Stack_Addr_NT'Access;
      SSL.Get_Exc_Stack_Addr := SSL.Get_Exc_Stack_Addr_NT'Access;
      SSL.Set_Exc_Stack_Addr := SSL.Set_Exc_Stack_Addr_NT'Access;
      SSL.Check_Abort_Status := SSL.Check_Abort_Status_NT'Access;
      SSL.Get_Stack_Info     := SSL.Get_Stack_Info_NT'Access;

      --  Don't bother trying to finalize Initialization.Global_Task_Lock
      --  and System.Task_Primitives.RTS_Lock.

   end Finalize_Global_Tasks;

   ---------------
   -- Free_Task --
   ---------------

   procedure Free_Task (T : Task_ID) is
      Self_Id : constant Task_ID := Self;

   begin
      if T.Common.State = Terminated then
         --  It is not safe to call Abort_Defer or Write_Lock at this stage

         Initialization.Task_Lock (Self_Id);

         if T.Common.Task_Image /= null then
            Free_Task_Image (T.Common.Task_Image);
         end if;

         Lock_RTS;
         Initialization.Remove_From_All_Tasks_List (T);
         Unlock_RTS;

         Initialization.Task_Unlock (Self_Id);

         System.Task_Primitives.Operations.Finalize_TCB (T);

      --  If the task is not terminated, then we simply ignore the call. This
      --  happens when a user program attempts an unchecked deallocation on
      --  a non-terminated task.

      else
         null;
      end if;
   end Free_Task;

   ----------------------
   -- Notify_Exception --
   ----------------------

   procedure Notify_Exception
     (Self_Id : Task_ID;
      Excep   : Exception_Occurrence)
   is
      procedure To_Stderr (S : String);
      pragma Import (Ada, To_Stderr, "__gnat_to_stderr");

      use System.Task_Info;
      use System.Soft_Links;

      function To_Address is new
        Unchecked_Conversion (Task_ID, System.Address);

      function Tailored_Exception_Information
        (E : Exception_Occurrence) return String;
      pragma Import
        (Ada, Tailored_Exception_Information,
         "__gnat_tailored_exception_information");

   begin
      To_Stderr ("task ");

      if Self_Id.Common.Task_Image /= null then
         To_Stderr (Self_Id.Common.Task_Image.all);
         To_Stderr ("_");
      end if;

      To_Stderr (System.Address_Image (To_Address (Self_Id)));
      To_Stderr (" terminated by unhandled exception");
      To_Stderr ((1 => ASCII.LF));
      To_Stderr (Tailored_Exception_Information (Excep));
   end Notify_Exception;

   ------------------
   -- Task_Wrapper --
   ------------------

   --  The task wrapper is a procedure that is called first for each task
   --  task body, and which in turn calls the compiler-generated task body
   --  procedure. The wrapper's main job is to do initialization for the task.
   --  It also has some locally declared objects that server as per-task local
   --  data. Task finalization is done by Complete_Task, which is called from
   --  an at-end handler that the compiler generates.

   --  The variable ID in the task wrapper is used to implement the Self
   --  function on targets where there is a fast way to find the stack base
   --  of the current thread, since it should be at a fixed offset from the
   --  stack base.

   --  The variable Magic_Number is also used in such implementations
   --  of Self, to check whether the current task is an Ada task, as
   --  compared to other-language threads.

   --  Both act as constants, once initialized, but need to be marked as
   --  volatile or aliased to prevent the compiler from optimizing away the
   --  storage. See System.Task_Primitives.Operations.Self for more info.

   procedure Task_Wrapper (Self_ID : Task_ID) is
      ID : Task_ID := Self_ID;
      pragma Volatile (ID);
      --  Do not delete this variable.
      --  In some targets, we need this variable to implement a fast Self.

      Magic_Number : Interfaces.C.unsigned := 16#ADAADAAD#;
      pragma Volatile (Magic_Number);
      --  We use this to verify that we are looking at an Ada task,
      --  inside of System.Task_Primitives.Operations.Self.

      use type System.Parameters.Size_Type;
      use type SSE.Storage_Offset;
      use System.Standard_Library;

      Secondary_Stack : aliased SSE.Storage_Array
        (1 .. ID.Common.Compiler_Data.Pri_Stack_Info.Size *
           SSE.Storage_Offset (Parameters.Sec_Stack_Ratio) / 100);
      Secondary_Stack_Address : System.Address := Secondary_Stack'Address;

   begin
      pragma Assert (Self_ID.Deferral_Level = 1);

      if not Parameters.Sec_Stack_Dynamic then
         ID.Common.Compiler_Data.Sec_Stack_Addr := Secondary_Stack'Address;
         SST.SS_Init (Secondary_Stack_Address, Integer (Secondary_Stack'Last));
      end if;

      --  Set the guard page at the bottom of the stack.
      --  The call to unprotect the page is done in Terminate_Task

      Stack_Guard (Self_ID, True);

      --  Initialize low-level TCB components, that
      --  cannot be initialized by the creator.
      --  Enter_Task sets Self_ID.Known_Tasks_Index
      --  and Self_ID.LL.Thread

      Enter_Task (Self_ID);

      --  We lock RTS_Lock to wait for activator to finish activating
      --  the rest of the chain, so that everyone in the chain comes out
      --  in priority order.
      --  This also protects the value of
      --   Self_ID.Common.Activator.Common.Wait_Count.

      Lock_RTS;
      Unlock_RTS;

      begin
         --  We are separating the following portion of the code in order to
         --  place the exception handlers in a different block.
         --  In this way we do not call Set_Jmpbuf_Address (which needs
         --  Self) before we set Self in Enter_Task

         --  Call the task body procedure.

         --  The task body is called with abort still deferred. That
         --  eliminates a dangerous window, for which we had to patch-up in
         --  Terminate_Task.
         --  During the expansion of the task body, we insert an RTS-call
         --  to Abort_Undefer, at the first point where abort should be
         --  allowed.

         Self_ID.Common.Task_Entry_Point (Self_ID.Common.Task_Arg);
         Terminate_Task (Self_ID);

      exception
         when Standard'Abort_Signal =>
            Terminate_Task (Self_ID);

         when others =>
            --  ??? Using an E : others here causes CD2C11A  to fail on
            --      DEC Unix, see 7925-005.

            if Exception_Trace = Unhandled_Raise then
               Notify_Exception (Self_ID, SSL.Get_Current_Excep.all.all);
            end if;

            Terminate_Task (Self_ID);
      end;
   end Task_Wrapper;

   --------------------
   -- Terminate_Task --
   --------------------

   --  Before we allow the thread to exit, we must clean up. This is a
   --  a delicate job. We must wake up the task's master, who may immediately
   --  try to deallocate the ATCB out from under the current task WHILE IT IS
   --  STILL EXECUTING.

   --  To avoid this, the parent task must be blocked up to the last thing
   --  done before the call to Exit_Task. The trouble is that we have another
   --  step that we also want to postpone to the very end, i.e., calling
   --  SSL.Destroy_TSD. We have to postpone that until the end because
   --  compiler-generated code is likely to try to access that data at just
   --  about any point.

   --  We can't call Destroy_TSD while we are holding any other locks, because
   --  it locks Global_Task_Lock, and our deadlock prevention rules require
   --  that to be the outermost lock. Our first "solution" was to just lock
   --  Global_Task_Lock in addition to the other locks, and force the parent
   --  to also lock this lock between its wakeup and its freeing of the ATCB.
   --  See Complete_Task for the parent-side of the code that has the matching
   --  calls to Task_Lock and Task_Unlock. That was not really a solution,
   --  since the operation Task_Unlock continued to access the ATCB after
   --  unlocking, after which the parent was observed to race ahead,
   --  deallocate the ATCB, and then reallocate it to another task. The
   --  call to Undefer_Abortion in Task_Unlock by the "terminated" task was
   --  overwriting the data of the new task that reused the ATCB! To solve
   --  this problem, we introduced the new operation Final_Task_Unlock.

   procedure Terminate_Task (Self_ID : Task_ID) is
      Environment_Task : constant Task_ID := STPO.Environment_Task;

   begin
      if Runtime_Traces then
         Send_Trace_Info (T_Terminate);
      end if;

      --  Since GCC cannot allocate stack chunks efficiently without reordering
      --  some of the allocations, we have to handle this unexpected situation
      --  here. We should normally never have to call Vulnerable_Complete_Task
      --  here. See 6602-003 for more details.

      if Self_ID.Common.Activator /= null then
         Vulnerable_Complete_Task (Self_ID);
      end if;

      Initialization.Task_Lock (Self_ID);

      if Single_Lock then
         Lock_RTS;
      end if;

      --  Check if the current task is an independent task
      --  If so, decrement the Independent_Task_Count value.

      if Self_ID.Master_of_Task = 2 then
         if Single_Lock then
            Utilities.Independent_Task_Count :=
              Utilities.Independent_Task_Count - 1;

         else
            Write_Lock (Environment_Task);
            Utilities.Independent_Task_Count :=
              Utilities.Independent_Task_Count - 1;
            Unlock (Environment_Task);
         end if;
      end if;

      --  Unprotect the guard page if needed.

      Stack_Guard (Self_ID, False);

      Utilities.Make_Passive (Self_ID, Task_Completed => True);

      if Single_Lock then
         Unlock_RTS;
      end if;

      pragma Assert (Check_Exit (Self_ID));

      SSL.Destroy_TSD (Self_ID.Common.Compiler_Data);
      Initialization.Final_Task_Unlock (Self_ID);

      --  WARNING
      --  past this point, this thread must assume that the ATCB
      --  has been deallocated. It should not be accessed again.

      STPO.Exit_Task;
   end Terminate_Task;

   ----------------
   -- Terminated --
   ----------------

   function Terminated (T : Task_ID) return Boolean is
      Result  : Boolean;
      Self_ID : Task_ID := STPO.Self;

   begin
      Initialization.Defer_Abort_Nestable (Self_ID);

      if Single_Lock then
         Lock_RTS;
      end if;

      Write_Lock (T);
      Result := T.Common.State = Terminated;
      Unlock (T);

      if Single_Lock then
         Unlock_RTS;
      end if;

      Initialization.Undefer_Abort_Nestable (Self_ID);
      return Result;
   end Terminated;

   ------------------------------------
   -- Vulnerable_Complete_Activation --
   ------------------------------------

   --  As in several other places, the locks of the activator and activated
   --  task are both locked here. This follows our deadlock prevention lock
   --  ordering policy, since the activated task must be created after the
   --  activator.

   procedure Vulnerable_Complete_Activation (Self_ID : Task_ID) is
      Activator : constant Task_ID := Self_ID.Common.Activator;

   begin
      pragma Debug (Debug.Trace (Self_ID, "V_Complete_Activation", 'C'));

      Write_Lock (Activator);
      Write_Lock (Self_ID);

      pragma Assert (Self_ID.Common.Activator /= null);

      --  Remove dangling reference to Activator,
      --  since a task may outlive its activator.

      Self_ID.Common.Activator := null;

      --  Wake up the activator, if it is waiting for a chain
      --  of tasks to activate, and we are the last in the chain
      --  to complete activation

      if Activator.Common.State = Activator_Sleep then
         Activator.Common.Wait_Count := Activator.Common.Wait_Count - 1;

         if Activator.Common.Wait_Count = 0 then
            Wakeup (Activator, Activator_Sleep);
         end if;
      end if;

      --  The activator raises a Tasking_Error if any task
      --  it is activating is completed before the activation is
      --  done. However, if the reason for the task completion is
      --  an abortion, we do not raise an exception. ARM 9.2(5).

      if not Self_ID.Callable and then Self_ID.Pending_ATC_Level /= 0 then
         Activator.Common.Activation_Failed := True;
      end if;

      Unlock (Self_ID);
      Unlock (Activator);

      --  After the activation, active priority should be the same
      --  as base priority. We must unlock the Activator first,
      --  though, since it should not wait if we have lower priority.

      if Get_Priority (Self_ID) /= Self_ID.Common.Base_Priority then
         Write_Lock (Self_ID);
         Set_Priority (Self_ID, Self_ID.Common.Base_Priority);
         Unlock (Self_ID);
      end if;
   end Vulnerable_Complete_Activation;

   --------------------------------
   -- Vulnerable_Complete_Master --
   --------------------------------

   procedure Vulnerable_Complete_Master (Self_ID : Task_ID) is
      C      : Task_ID;
      P      : Task_ID;
      CM     : Master_Level := Self_ID.Master_Within;
      T      : aliased Task_ID;

      To_Be_Freed : Task_ID;
      --  This is a list of ATCBs to be freed, after we have released
      --  all RTS locks. This is necessary because of the locking order
      --  rules, since the storage manager uses Global_Task_Lock.

      pragma Warnings (Off);
      function Check_Unactivated_Tasks return Boolean;
      pragma Warnings (On);
      --  Temporary error-checking code below. This is part of the checks
      --  added in the new run time. Call it only inside a pragma Assert.

      -----------------------------
      -- Check_Unactivated_Tasks --
      -----------------------------

      function Check_Unactivated_Tasks return Boolean is
      begin
         if not Single_Lock then
            Lock_RTS;
         end if;

         Write_Lock (Self_ID);
         C := All_Tasks_List;

         while C /= null loop
            if C.Common.Activator = Self_ID then
               return False;
            end if;

            if C.Common.Parent = Self_ID and then C.Master_of_Task = CM then
               Write_Lock (C);

               if C.Common.State = Unactivated then
                  return False;
               end if;

               Unlock (C);
            end if;

            C := C.Common.All_Tasks_Link;
         end loop;

         Unlock (Self_ID);

         if not Single_Lock then
            Unlock_RTS;
         end if;

         return True;
      end Check_Unactivated_Tasks;

   --  Start of processing for Vulnerable_Complete_Master

   begin
      pragma Debug
        (Debug.Trace (Self_ID, "V_Complete_Master", 'C'));

      pragma Assert (Self_ID.Common.Wait_Count = 0);
      pragma Assert (Self_ID.Deferral_Level > 0);

      --  Count how many active dependent tasks this master currently
      --  has, and record this in Wait_Count.

      --  This count should start at zero, since it is initialized to
      --  zero for new tasks, and the task should not exit the
      --  sleep-loops that use this count until the count reaches zero.

      Lock_RTS;
      Write_Lock (Self_ID);
      C := All_Tasks_List;

      while C /= null loop
         if C.Common.Activator = Self_ID then
            pragma Assert (C.Common.State = Unactivated);

            Write_Lock (C);
            C.Common.Activator := null;
            C.Common.State := Terminated;
            C.Callable := False;
            Utilities.Cancel_Queued_Entry_Calls (C);
            Unlock (C);
         end if;

         if C.Common.Parent = Self_ID and then C.Master_of_Task = CM then
            Write_Lock (C);

            if C.Awake_Count /= 0 then
               Self_ID.Common.Wait_Count := Self_ID.Common.Wait_Count + 1;
            end if;

            Unlock (C);
         end if;

         C := C.Common.All_Tasks_Link;
      end loop;

      Self_ID.Common.State := Master_Completion_Sleep;
      Unlock (Self_ID);

      if not Single_Lock then
         Unlock_RTS;
      end if;

      --  Wait until dependent tasks are all terminated or ready to terminate.
      --  While waiting, the task may be awakened if the task's priority needs
      --  changing, or this master is aborted. In the latter case, we want
      --  to abort the dependents, and resume waiting until Wait_Count goes
      --  to zero.

      Write_Lock (Self_ID);

      loop
         Initialization.Poll_Base_Priority_Change (Self_ID);
         exit when Self_ID.Common.Wait_Count = 0;

         --  Here is a difference as compared to Complete_Master

         if Self_ID.Pending_ATC_Level < Self_ID.ATC_Nesting_Level
           and then not Self_ID.Dependents_Aborted
         then
            if Single_Lock then
               Abort_Dependents (Self_ID);
            else
               Unlock (Self_ID);
               Lock_RTS;
               Abort_Dependents (Self_ID);
               Unlock_RTS;
               Write_Lock (Self_ID);
            end if;
         else
            Sleep (Self_ID, Master_Completion_Sleep);
         end if;
      end loop;

      Self_ID.Common.State := Runnable;
      Unlock (Self_ID);

      --  Dependents are all terminated or on terminate alternatives.
      --  Now, force those on terminate alternatives to terminate, by
      --  aborting them.

      pragma Assert (Check_Unactivated_Tasks);

      if Self_ID.Alive_Count > 1 then
         --  ???
         --  Consider finding a way to skip the following extra steps if there
         --  are no dependents with terminate alternatives. This could be done
         --  by adding another count to the ATCB, similar to Awake_Count, but
         --  keeping track of tasks that are on terminate alternatives.

         pragma Assert (Self_ID.Common.Wait_Count = 0);

         --  Force any remaining dependents to terminate, by aborting them.

         if not Single_Lock then
            Lock_RTS;
         end if;

         Abort_Dependents (Self_ID);

         --  Above, when we "abort" the dependents we are simply using this
         --  operation for convenience. We are not required to support the full
         --  abort-statement semantics; in particular, we are not required to
         --  immediately cancel any queued or in-service entry calls. That is
         --  good, because if we tried to cancel a call we would need to lock
         --  the caller, in order to wake the caller up. Our anti-deadlock
         --  rules prevent us from doing that without releasing the locks on C
         --  and Self_ID. Releasing and retaking those locks would be wasteful
         --  at best, and should not be considered further without more
         --  detailed analysis of potential concurrent accesses to the
         --  ATCBs of C and Self_ID.

         --  Count how many "alive" dependent tasks this master currently
         --  has, and record this in Wait_Count. This count should start at
         --  zero, since it is initialized to zero for new tasks, and the
         --  task should not exit the sleep-loops that use this count until
         --  the count reaches zero.

         pragma Assert (Self_ID.Common.Wait_Count = 0);

         Write_Lock (Self_ID);
         C := All_Tasks_List;

         while C /= null loop
            if C.Common.Parent = Self_ID and then C.Master_of_Task = CM then
               Write_Lock (C);

               pragma Assert (C.Awake_Count = 0);

               if C.Alive_Count > 0 then
                  pragma Assert (C.Terminate_Alternative);
                  Self_ID.Common.Wait_Count := Self_ID.Common.Wait_Count + 1;
               end if;

               Unlock (C);
            end if;

            C := C.Common.All_Tasks_Link;
         end loop;

         Self_ID.Common.State := Master_Phase_2_Sleep;
         Unlock (Self_ID);

         if not Single_Lock then
            Unlock_RTS;
         end if;

         --  Wait for all counted tasks to finish terminating themselves.

         Write_Lock (Self_ID);

         loop
            Initialization.Poll_Base_Priority_Change (Self_ID);
            exit when Self_ID.Common.Wait_Count = 0;
            Sleep (Self_ID, Master_Phase_2_Sleep);
         end loop;

         Self_ID.Common.State := Runnable;
         Unlock (Self_ID);
      end if;

      --  We don't wake up for abortion here. We are already terminating
      --  just as fast as we can, so there is no point.

      --  Remove terminated tasks from the list of Self_ID's dependents, but
      --  don't free their ATCBs yet, because of lock order restrictions,
      --  which don't allow us to call "free" or "malloc" while holding any
      --  other locks. Instead, we put those ATCBs to be freed onto a
      --  temporary list, called To_Be_Freed.

      if not Single_Lock then
         Lock_RTS;
      end if;

      C := All_Tasks_List;
      P := null;

      while C /= null loop
         if C.Common.Parent = Self_ID and then C.Master_of_Task >= CM then
            if P /= null then
               P.Common.All_Tasks_Link := C.Common.All_Tasks_Link;
            else
               All_Tasks_List := C.Common.All_Tasks_Link;
            end if;

            T := C.Common.All_Tasks_Link;
            C.Common.All_Tasks_Link := To_Be_Freed;
            To_Be_Freed := C;
            C := T;

         else
            P := C;
            C := C.Common.All_Tasks_Link;
         end if;
      end loop;

      Unlock_RTS;

      --  Free all the ATCBs on the list To_Be_Freed.

      --  The ATCBs in the list are no longer in All_Tasks_List, and after
      --  any interrupt entries are detached from them they should no longer
      --  be referenced.

      --  Global_Task_Lock (Task_Lock/Unlock) is locked in the loop below to
      --  avoid a race between a terminating task and its parent. The parent
      --  might try to deallocate the ACTB out from underneath the exiting
      --  task. Note that Free will also lock Global_Task_Lock, but that is
      --  OK, since this is the *one* lock for which we have a mechanism to
      --  support nested locking. See Task_Wrapper and its finalizer for more
      --  explanation.

      --  ???
      --  The check "T.Common.Parent /= null ..." below is to prevent dangling
      --  references to terminated library-level tasks, which could
      --  otherwise occur during finalization of library-level objects.
      --  A better solution might be to hook task objects into the
      --  finalization chain and deallocate the ATCB when the task
      --  object is deallocated. However, this change is not likely
      --  to gain anything significant, since all this storage should
      --  be recovered en-masse when the process exits.

      while To_Be_Freed /= null loop
         T := To_Be_Freed;
         To_Be_Freed := T.Common.All_Tasks_Link;

         --  ??? On SGI there is currently no Interrupt_Manager, that's
         --  why we need to check if the Interrupt_Manager_ID is null

         if T.Interrupt_Entry and Interrupt_Manager_ID /= null then
            declare
               Detach_Interrupt_Entries_Index : Task_Entry_Index := 1;
               --  Corresponds to the entry index of System.Interrupts.
               --  Interrupt_Manager.Detach_Interrupt_Entries.
               --  Be sure to update this value when changing
               --  Interrupt_Manager specs.

               type Param_Type is access all Task_ID;

               Param : aliased Param_Type := T'Access;

            begin
               System.Tasking.Rendezvous.Call_Simple
                 (Interrupt_Manager_ID, Detach_Interrupt_Entries_Index,
                  Param'Address);
            end;
         end if;

         if (T.Common.Parent /= null
              and then T.Common.Parent.Common.Parent /= null)
           or else T.Master_of_Task > 3
         then
            Initialization.Task_Lock (Self_ID);

            --  If Sec_Stack_Addr is not null, it means that Destroy_TSD
            --  has not been called yet (case of an unactivated task).

            if T.Common.Compiler_Data.Sec_Stack_Addr /= Null_Address then
               SSL.Destroy_TSD (T.Common.Compiler_Data);
            end if;

            Vulnerable_Free_Task (T);
            Initialization.Task_Unlock (Self_ID);
         end if;
      end loop;

      --  It might seem nice to let the terminated task deallocate its own
      --  ATCB. That would not cover the case of unactivated tasks. It also
      --  would force us to keep the underlying thread around past termination,
      --  since references to the ATCB are possible past termination.
      --  Currently, we get rid of the thread as soon as the task terminates,
      --  and let the parent recover the ATCB later.

      --  Some day, if we want to recover the ATCB earlier, at task
      --  termination, we could consider using "fat task IDs", that include the
      --  serial number with the ATCB pointer, to catch references to tasks
      --  that no longer have ATCBs. It is not clear how much this would gain,
      --  since the user-level task object would still be occupying storage.

      --  Make next master level up active.
      --  We don't need to lock the ATCB, since the value is only updated by
      --  each task for itself.

      Self_ID.Master_Within := CM - 1;
   end Vulnerable_Complete_Master;

   ------------------------------
   -- Vulnerable_Complete_Task --
   ------------------------------

   --  Complete the calling task

   --  This procedure must be called with abort deferred. (That's why the
   --  name has "Vulnerable" in it.) It should only be called by Complete_Task
   --  and Finalize_Global_Tasks (for the environment task).

   --  The effect is similar to that of Complete_Master. Differences include
   --  the closing of entries here, and computation of the number of active
   --  dependent tasks in Complete_Master.

   --  We don't lock Self_ID before the call to Vulnerable_Complete_Activation,
   --  because that does its own locking, and because we do not need the lock
   --  to test Self_ID.Common.Activator. That value should only be read and
   --  modified by Self.

   procedure Vulnerable_Complete_Task (Self_ID : Task_ID) is
   begin
      pragma Assert (Self_ID.Deferral_Level > 0);
      pragma Assert (Self_ID = Self);
      pragma Assert (Self_ID.Master_Within = Self_ID.Master_of_Task + 1
                       or else
                     Self_ID.Master_Within = Self_ID.Master_of_Task + 2);
      pragma Assert (Self_ID.Common.Wait_Count = 0);
      pragma Assert (Self_ID.Open_Accepts = null);
      pragma Assert (Self_ID.ATC_Nesting_Level = 1);

      pragma Debug (Debug.Trace (Self_ID, "V_Complete_Task", 'C'));

      if Single_Lock then
         Lock_RTS;
      end if;

      Write_Lock (Self_ID);
      Self_ID.Callable := False;

      --  In theory, Self should have no pending entry calls left on its
      --  call-stack. Each async. select statement should clean its own call,
      --  and blocking entry calls should defer abort until the calls are
      --  cancelled, then clean up.

      Utilities.Cancel_Queued_Entry_Calls (Self_ID);
      Unlock (Self_ID);

      if Self_ID.Common.Activator /= null then
         Vulnerable_Complete_Activation (Self_ID);
      end if;

      if Single_Lock then
         Unlock_RTS;
      end if;

      --  If Self_ID.Master_Within = Self_ID.Master_of_Task + 2
      --  we may have dependent tasks for which we need to wait.
      --  Otherwise, we can just exit.

      if Self_ID.Master_Within = Self_ID.Master_of_Task + 2 then
         Vulnerable_Complete_Master (Self_ID);
      end if;
   end Vulnerable_Complete_Task;

   --------------------------
   -- Vulnerable_Free_Task --
   --------------------------

   --  Recover all runtime system storage associated with the task T.
   --  This should only be called after T has terminated and will no
   --  longer be referenced.

   --  For tasks created by an allocator that fails, due to an exception,
   --  it is called from Expunge_Unactivated_Tasks.

   --  For tasks created by elaboration of task object declarations it
   --  is called from the finalization code of the Task_Wrapper procedure.
   --  It is also called from Unchecked_Deallocation, for objects that
   --  are or contain tasks.

   procedure Vulnerable_Free_Task (T : Task_ID) is
   begin
      pragma Debug
        (Debug.Trace ("Vulnerable_Free_Task", T, 'C'));

      if Single_Lock then
         Lock_RTS;
      end if;

      Write_Lock (T);
      Initialization.Finalize_Attributes_Link.all (T);
      Unlock (T);

      if Single_Lock then
         Unlock_RTS;
      end if;

      if T.Common.Task_Image /= null then
         Free_Task_Image (T.Common.Task_Image);
      end if;

      System.Task_Primitives.Operations.Finalize_TCB (T);
   end Vulnerable_Free_Task;

begin
   --  Establish the Adafinal softlink.
   --  This is not done inside the central RTS initialization routine
   --  to avoid with-ing this package from System.Tasking.Initialization.

   SSL.Adafinal := Finalize_Global_Tasks'Access;

   --  Establish soft links for subprograms that manipulate master_id's.
   --  This cannot be done when the RTS is initialized, because of various
   --  elaboration constraints.

   SSL.Current_Master  := Stages.Current_Master'Access;
   SSL.Enter_Master    := Stages.Enter_Master'Access;
   SSL.Complete_Master := Stages.Complete_Master'Access;
end System.Tasking.Stages;
