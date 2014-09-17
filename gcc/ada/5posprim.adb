------------------------------------------------------------------------------
--                                                                          --
--                GNU ADA RUN-TIME LIBRARY (GNARL) COMPONENTS               --
--                                                                          --
--                  S Y S T E M . O S _ P R I M I T I V E S                 --
--                                                                          --
--                                  B o d y                                 --
--                                                                          --
--                                                                          --
--          Copyright (C) 1998-2001 Free Software Foundation, Inc.          --
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

--  This version uses gettimeofday and select
--  Currently OpenNT, Dec Unix, Solaris and SCO UnixWare use this file.

package body System.OS_Primitives is

   --  ??? These definitions are duplicated from System.OS_Interface
   --  because we don't want to depend on any package. Consider removing
   --  these declarations in System.OS_Interface and move these ones in
   --  the spec.

   type struct_timezone is record
      tz_minuteswest  : Integer;
      tz_dsttime   : Integer;
   end record;
   pragma Convention (C, struct_timezone);
   type struct_timezone_ptr is access all struct_timezone;

   type struct_timeval is record
      tv_sec       : Integer;
      tv_usec      : Integer;
   end record;
   pragma Convention (C, struct_timeval);

   function gettimeofday
     (tv : access struct_timeval;
      tz : struct_timezone_ptr) return Integer;
   pragma Import (C, gettimeofday, "gettimeofday");

   type fd_set is null record;
   type fd_set_ptr is access all fd_set;

   function C_select
     (n         : Integer    := 0;
      readfds,
      writefds,
      exceptfds : fd_set_ptr := null;
      timeout   : access struct_timeval) return Integer;
   pragma Import (C, C_select, "select");

   -----------
   -- Clock --
   -----------

   function Clock return Duration is
      TV     : aliased struct_timeval;
      Result : Integer;

   begin
      Result := gettimeofday (TV'Access, null);
      return Duration (TV.tv_sec) + Duration (TV.tv_usec) / 10#1#E6;
   end Clock;

   ---------------------
   -- Monotonic_Clock --
   ---------------------

   function Monotonic_Clock return Duration renames Clock;

   -----------------
   -- Timed_Delay --
   -----------------

   procedure Timed_Delay
     (Time : Duration;
      Mode : Integer)
   is
      Result     : Integer;
      Rel_Time   : Duration;
      Abs_Time   : Duration;
      Check_Time : Duration := Clock;
      timeval    : aliased struct_timeval;

   begin
      if Mode = Relative then
         Rel_Time := Time;
         Abs_Time := Time + Check_Time;
      else
         Rel_Time := Time - Check_Time;
         Abs_Time := Time;
      end if;

      if Rel_Time > 0.0 then
         loop
            timeval.tv_sec := Integer (Rel_Time);

            if Duration (timeval.tv_sec) > Rel_Time then
               timeval.tv_sec := timeval.tv_sec - 1;
            end if;

            timeval.tv_usec :=
              Integer ((Rel_Time - Duration (timeval.tv_sec)) * 10#1#E6);

            Result := C_select (timeout => timeval'Unchecked_Access);
            Check_Time := Clock;

            exit when Abs_Time <= Check_Time;

            Rel_Time := Abs_Time - Check_Time;
         end loop;
      end if;
   end Timed_Delay;

end System.OS_Primitives;
