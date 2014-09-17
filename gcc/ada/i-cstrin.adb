------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                 I N T E R F A C E S . C . S T R I N G S                  --
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

with System; use System;
with System.Address_To_Access_Conversions;

package body Interfaces.C.Strings is

   package Char_Access is new Address_To_Access_Conversions (char);

   -----------------------
   -- Local Subprograms --
   -----------------------

   function Peek (From : chars_ptr) return char;
   pragma Inline (Peek);
   --  Given a chars_ptr value, obtain referenced character

   procedure Poke (Value : char; Into : chars_ptr);
   pragma Inline (Poke);
   --  Given a chars_ptr, modify referenced Character value

   function "+" (Left : chars_ptr; Right : size_t) return chars_ptr;
   pragma Inline ("+");
   --  Address arithmetic on chars_ptr value

   function Position_Of_Nul (Into : char_array) return size_t;
   --  Returns position of the first Nul in Into or Into'Last + 1 if none

   --  We can't use directly System.Memory because the categorization is not
   --  compatible, so we directly import here the malloc and free routines.

   function Memory_Alloc (Size : size_t) return chars_ptr;
   pragma Import (C, Memory_Alloc, "__gnat_malloc");

   procedure Memory_Free (Address : chars_ptr);
   pragma Import (C, Memory_Free, "__gnat_free");

   ---------
   -- "+" --
   ---------

   function "+" (Left : chars_ptr; Right : size_t) return chars_ptr is
   begin
      return Left + chars_ptr (Right);
   end "+";

   ----------
   -- Free --
   ----------

   procedure Free (Item : in out chars_ptr) is
   begin
      if Item = Null_Ptr then
         return;
      end if;

      Memory_Free (Item);
      Item := Null_Ptr;
   end Free;

   --------------------
   -- New_Char_Array --
   --------------------

   function New_Char_Array (Chars : in char_array) return chars_ptr is
      Index   : size_t;
      Pointer : chars_ptr;

   begin
      --  Get index of position of null. If Index > Chars'last,
      --  nul is absent and must be added explicitly.

      Index := Position_Of_Nul (Into => Chars);
      Pointer := Memory_Alloc ((Index - Chars'First + 1));

      --  If nul is present, transfer string up to and including it.

      if Index <= Chars'Last then
         Update (Item   => Pointer,
                 Offset => 0,
                 Chars  => Chars (Chars'First .. Index),
                 Check  => False);
      else
         --  If original string has no nul, transfer whole string and add
         --  terminator explicitly.

         Update (Item   => Pointer,
                 Offset => 0,
                 Chars  => Chars,
                 Check  => False);
         Poke (nul, into => Pointer + size_t '(Chars'Length));
      end if;

      return Pointer;
   end New_Char_Array;

   ----------------
   -- New_String --
   ----------------

   function New_String (Str : in String) return chars_ptr is
   begin
      return New_Char_Array (To_C (Str));
   end New_String;

   ----------
   -- Peek --
   ----------

   function Peek (From : chars_ptr) return char is
      use Char_Access;
   begin
      return To_Pointer (Address (To_Address (From))).all;
   end Peek;

   ----------
   -- Poke --
   ----------

   procedure Poke (Value : char; Into : chars_ptr) is
      use Char_Access;
   begin
      To_Pointer (Address (To_Address (Into))).all := Value;
   end Poke;

   ---------------------
   -- Position_Of_Nul --
   ---------------------

   function Position_Of_Nul (Into : char_array) return size_t is
   begin
      for J in Into'Range loop
         if Into (J) = nul then
            return J;
         end if;
      end loop;

      return Into'Last + 1;
   end Position_Of_Nul;

   ------------
   -- Strlen --
   ------------

   function Strlen (Item : in chars_ptr) return size_t is
      Item_Index : size_t := 0;

   begin
      if Item = Null_Ptr then
         raise Dereference_Error;
      end if;

      loop
         if Peek (Item + Item_Index) = nul then
            return Item_Index;
         end if;

         Item_Index := Item_Index + 1;
      end loop;
   end Strlen;

   ------------------
   -- To_Chars_Ptr --
   ------------------

   function To_Chars_Ptr
     (Item      : in char_array_access;
      Nul_Check : in Boolean := False)
      return      chars_ptr
   is
   begin
      if Item = null then
         return Null_Ptr;
      elsif Nul_Check
        and then Position_Of_Nul (Into => Item.all) > Item'Last
      then
         raise Terminator_Error;
      else
         return To_Integer (Item (Item'First)'Address);
      end if;
   end To_Chars_Ptr;

   ------------
   -- Update --
   ------------

   procedure Update
     (Item   : in chars_ptr;
      Offset : in size_t;
      Chars  : in char_array;
      Check  : Boolean := True)
   is
      Index : chars_ptr := Item + Offset;

   begin
      if Check and then Offset + Chars'Length  > Strlen (Item) then
         raise Update_Error;
      end if;

      for J in Chars'Range loop
         Poke (Chars (J), Into => Index);
         Index := Index + size_t'(1);
      end loop;
   end Update;

   procedure Update
     (Item   : in chars_ptr;
      Offset : in size_t;
      Str    : in String;
      Check  : in Boolean := True)
   is
   begin
      Update (Item, Offset, To_C (Str), Check);
   end Update;

   -----------
   -- Value --
   -----------

   function Value (Item : in chars_ptr) return char_array is
      Result : char_array (0 .. Strlen (Item));

   begin
      if Item = Null_Ptr then
         raise Dereference_Error;
      end if;

      --  Note that the following loop will also copy the terminating Nul

      for J in Result'Range loop
         Result (J) := Peek (Item + J);
      end loop;

      return Result;
   end Value;

   function Value
     (Item   : in chars_ptr;
      Length : in size_t)
      return   char_array
   is
   begin
      if Item = Null_Ptr then
         raise Dereference_Error;
      end if;

      --  ACATS cxb3010 checks that Constraint_Error gets raised when Length
      --  is 0. Seems better to check that Length is not null before declaring
      --  an array with size_t bounds of 0 .. Length - 1 anyway.

      if Length = 0 then
         raise Constraint_Error;
      end if;

      declare
         Result : char_array (0 .. Length - 1);

      begin
         for J in Result'Range loop
            Result (J) := Peek (Item + J);

            if Result (J) = nul then
               return Result (0 .. J);
            end if;
         end loop;

         return Result;
      end;
   end Value;

   function Value (Item : in chars_ptr) return String is
   begin
      return To_Ada (Value (Item));
   end Value;

   --  As per AI-00177, this is equivalent to
   --          To_Ada (Value (Item, Length) & nul);

   function Value (Item : in chars_ptr; Length : in size_t) return String is
      Result : char_array (0 .. Length);

   begin
      if Item = Null_Ptr then
         raise Dereference_Error;
      end if;

      for J in 0 .. Length - 1 loop
         Result (J) := Peek (Item + J);

         if Result (J) = nul then
            return To_Ada (Result (0 .. J));
         end if;
      end loop;

      Result (Length) := nul;
      return To_Ada (Result);
   end Value;

end Interfaces.C.Strings;
