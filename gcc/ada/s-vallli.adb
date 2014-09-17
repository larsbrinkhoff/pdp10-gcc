------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                       S Y S T E M . V A L _ L L I                        --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--                                                                          --
--   Copyright (C) 1992,1993,1994,1995,1996 Free Software Foundation, Inc.  --
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

with System.Unsigned_Types; use System.Unsigned_Types;
with System.Val_LLU;        use System.Val_LLU;
with System.Val_Util;       use System.Val_Util;

package body System.Val_LLI is

   ---------------------------
   -- Scn_Long_Long_Integer --
   ---------------------------

   function Scan_Long_Long_Integer
     (Str  : String;
      Ptr  : access Integer;
      Max  : Integer)
      return Long_Long_Integer
   is
      Uval : Long_Long_Unsigned;
      --  Unsigned result

      Minus : Boolean := False;
      --  Set to True if minus sign is present, otherwise to False

      Start : Positive;
      --  Saves location of first non-blank (not used in this case)

   begin
      Scan_Sign (Str, Ptr, Max, Minus, Start);
      Uval := Scan_Long_Long_Unsigned (Str, Ptr, Max);

      --  Deal with overflow cases, and also with maximum negative number

      if Uval > Long_Long_Unsigned (Long_Long_Integer'Last) then
         if Minus
           and then Uval = Long_Long_Unsigned (-(Long_Long_Integer'First)) then
            return Long_Long_Integer'First;
         else
            raise Constraint_Error;
         end if;

      --  Negative values

      elsif Minus then
         return -(Long_Long_Integer (Uval));

      --  Positive values

      else
         return Long_Long_Integer (Uval);
      end if;

   end Scan_Long_Long_Integer;

   -----------------------------
   -- Value_Long_Long_Integer --
   -----------------------------

   function Value_Long_Long_Integer (Str : String) return Long_Long_Integer is
      V : Long_Long_Integer;
      P : aliased Integer := Str'First;

   begin
      V := Scan_Long_Long_Integer (Str, P'Access, Str'Last);
      Scan_Trailing_Blanks (Str, P);
      return V;

   end Value_Long_Long_Integer;

end System.Val_LLI;
