------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUNTIME COMPONENTS                          --
--                                                                          --
--          A D A . T E X T _ I O . E N U M E R A T I O N _ A U X           --
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

with Ada.Text_IO.Generic_Aux; use Ada.Text_IO.Generic_Aux;
with Ada.Characters.Handling; use Ada.Characters.Handling;
with Interfaces.C_Streams;    use Interfaces.C_Streams;

--  Note: this package does not yet deal properly with wide characters ???

package body Ada.Text_IO.Enumeration_Aux is

   -----------------------
   -- Local Subprograms --
   -----------------------

   --  These definitions replace the ones in Ada.Characters.Handling, which
   --  do not seem to work for some strange not understood reason ??? at
   --  least in the OS/2 version.

   function To_Lower (C : Character) return Character;
   function To_Upper (C : Character) return Character;

   ------------------
   -- Get_Enum_Lit --
   ------------------

   procedure Get_Enum_Lit
     (File   : File_Type;
      Buf    : out String;
      Buflen : out Natural)
   is
      ch  : int;
      C   : Character;

   begin
      Buflen := 0;
      Load_Skip (File);
      ch := Getc (File);
      C := Character'Val (ch);

      --  Character literal case. If the initial character is a quote, then
      --  we read as far as we can without backup (see ACVC test CE3905L)

      if C = ''' then
         Store_Char (File, ch, Buf, Buflen);

         ch := Getc (File);

         if ch in 16#20# .. 16#7E# or else ch >= 16#80# then
            Store_Char (File, ch, Buf, Buflen);

            ch := Getc (File);

            if ch = Character'Pos (''') then
               Store_Char (File, ch, Buf, Buflen);
            else
               Ungetc (ch, File);
            end if;

         else
            Ungetc (ch, File);
         end if;

      --  Similarly for identifiers, read as far as we can, in particular,
      --  do read a trailing underscore (again see ACVC test CE3905L to
      --  understand why we do this, although it seems somewhat peculiar).

      else
         --  Identifier must start with a letter

         if not Is_Letter (C) then
            Ungetc (ch, File);
            return;
         end if;

         --  If we do have a letter, loop through the characters quitting on
         --  the first non-identifier character (note that this includes the
         --  cases of hitting a line mark or page mark).

         loop
            C := Character'Val (ch);
            Store_Char (File, Character'Pos (To_Upper (C)), Buf, Buflen);

            ch := Getc (File);
            exit when ch = EOF;
            C := Character'Val (ch);

            exit when not Is_Letter (C)
              and then not Is_Digit (C)
              and then C /= '_';

            exit when C = '_'
              and then Buf (Buflen) = '_';
         end loop;

         Ungetc (ch, File);
      end if;
   end Get_Enum_Lit;

   ---------
   -- Put --
   ---------

   procedure Put
     (File  : File_Type;
      Item  : String;
      Width : Field;
      Set   : Type_Set)
   is
      Actual_Width : constant Count := Count'Max (Count (Width), Item'Length);

   begin
      if Set = Lower_Case and then Item (1) /= ''' then
         declare
            Iteml : String (Item'First .. Item'Last);

         begin
            for J in Item'Range loop
               Iteml (J) := To_Lower (Item (J));
            end loop;

            Put_Item (File, Iteml);
         end;

      else
         Put_Item (File, Item);
      end if;

      for J in 1 .. Actual_Width - Item'Length loop
         Put (File, ' ');
      end loop;
   end Put;

   ----------
   -- Puts --
   ----------

   procedure Puts
     (To    : out String;
      Item  : in String;
      Set   : Type_Set)
   is
      Ptr : Natural;

   begin
      if Item'Length > To'Length then
         raise Layout_Error;

      else
         Ptr := To'First;
         for J in Item'Range loop
            if Set = Lower_Case and then Item (1) /= ''' then
               To (Ptr) := To_Lower (Item (J));
            else
               To (Ptr) := Item (J);
            end if;

            Ptr := Ptr + 1;
         end loop;

         while Ptr <= To'Last loop
            To (Ptr) := ' ';
            Ptr := Ptr + 1;
         end loop;
      end if;
   end Puts;

   -------------------
   -- Scan_Enum_Lit --
   -------------------

   procedure Scan_Enum_Lit
     (From  : String;
      Start : out Natural;
      Stop  : out Natural)
   is
      C  : Character;

   --  Processing for Scan_Enum_Lit

   begin
      String_Skip (From, Start);

      --  Character literal case. If the initial character is a quote, then
      --  we read as far as we can without backup (see ACVC test CE3905L
      --  which is for the analogous case for reading from a file).

      if From (Start) = ''' then
         Stop := Start;

         if Stop = From'Last then
            raise Data_Error;
         else
            Stop := Stop + 1;
         end if;

         if From (Stop) in ' ' .. '~'
           or else From (Stop) >= Character'Val (16#80#)
         then
            if Stop = From'Last then
               raise Data_Error;
            else
               Stop := Stop + 1;

               if From (Stop) = ''' then
                  return;
               end if;
            end if;
         end if;

         Stop := Stop - 1;
         raise Data_Error;

      --  Similarly for identifiers, read as far as we can, in particular,
      --  do read a trailing underscore (again see ACVC test CE3905L to
      --  understand why we do this, although it seems somewhat peculiar).

      else
         --  Identifier must start with a letter

         if not Is_Letter (From (Start)) then
            raise Data_Error;
         end if;

         --  If we do have a letter, loop through the characters quitting on
         --  the first non-identifier character (note that this includes the
         --  cases of hitting a line mark or page mark).

         Stop := Start;
         while Stop < From'Last loop
            C := From (Stop + 1);

            exit when not Is_Letter (C)
              and then not Is_Digit (C)
              and then C /= '_';

            exit when C = '_'
              and then From (Stop) = '_';

            Stop := Stop + 1;
         end loop;
      end if;

   end Scan_Enum_Lit;

   --------------
   -- To_Lower --
   --------------

   function To_Lower (C : Character) return Character is
   begin
      if C in 'A' .. 'Z' then
         return Character'Val (Character'Pos (C) + 32);
      else
         return C;
      end if;
   end To_Lower;

   function To_Upper (C : Character) return Character is
   begin
      if C in 'a' .. 'z' then
         return Character'Val (Character'Pos (C) - 32);
      else
         return C;
      end if;
   end To_Upper;

end Ada.Text_IO.Enumeration_Aux;
