------------------------------------------------------------------------------
--                                                                          --
--                 GNU ADA RUN-TIME LIBRARY (GNARL) COMPONENTS              --
--                                                                          --
--                 S Y S T E M . T A S K _ P R I M I T I V E S              --
--                                                                          --
--                                  S p e c                                 --
--                                                                          --
--                                                                          --
--            Copyright (C) 1991-2000, Florida State University             --
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

--  This is a POSIX-like version of this package.
--  Note: this file can only be used for POSIX compliant systems.

pragma Polling (Off);
--  Turn off polling, we do not want ATC polling to take place during
--  tasking operations. It causes infinite loops and other problems.

with System.OS_Interface;
--  used for pthread_mutex_t
--           pthread_cond_t
--           pthread_t

package System.Task_Primitives is

   type Lock is limited private;
   --  Should be used for implementation of protected objects.

   type RTS_Lock is limited private;
   --  Should be used inside the runtime system.
   --  The difference between Lock and the RTS_Lock is that the later
   --  one serves only as a semaphore so that do not check for
   --  ceiling violations.

   type Task_Body_Access is access procedure;
   --  Pointer to the task body's entry point (or possibly a wrapper
   --  declared local to the GNARL).

   type Private_Data is limited private;
   --  Any information that the GNULLI needs maintained on a per-task
   --  basis.  A component of this type is guaranteed to be included
   --  in the Ada_Task_Control_Block.

private

   type Lock is new System.OS_Interface.pthread_mutex_t;
   type RTS_Lock is new System.OS_Interface.pthread_mutex_t;

   type Private_Data is record
      Thread : aliased System.OS_Interface.pthread_t;
      pragma Atomic (Thread);
      --  Thread field may be updated by two different threads of control.
      --  (See, Enter_Task and Create_Task in s-taprop.adb).
      --  They put the same value (thr_self value). We do not want to
      --  use lock on those operations and the only thing we have to
      --  make sure is that they are updated in atomic fashion.

      LWP : aliased System.Address;
      --  The purpose of this field is to provide a better tasking support on
      --  gdb. The order of the two first fields (Thread and LWP) is important.
      --  On targets where lwp is not relevant, this is equivalent to Thread.

      CV : aliased System.OS_Interface.pthread_cond_t;

      L  : aliased RTS_Lock;
      --  Protection for all components is lock L
   end record;

end System.Task_Primitives;
