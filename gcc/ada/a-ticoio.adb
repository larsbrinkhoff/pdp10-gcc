------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUNTIME COMPONENTS                          --
--                                                                          --
--               A D A . T E X T _ I O . C O M P L E X _ I O                --
--                                                                          --
--                                 B o d y                                  --
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

with Ada.Text_IO;

with Ada.Text_IO.Complex_Aux;

package body Ada.Text_IO.Complex_IO is

   package Aux renames Ada.Text_IO.Complex_Aux;

   subtype LLF is Long_Long_Float;
   --  Type used for calls to routines in Aux

   ---------
   -- Get --
   ---------

   procedure Get
     (File  : in  File_Type;
      Item  : out Complex_Types.Complex;
      Width : in  Field := 0)
   is
      Real_Item  : Real'Base;
      Imag_Item  : Real'Base;

   begin
      Aux.Get (File, LLF (Real_Item), LLF (Imag_Item), Width);
      Item := (Real_Item, Imag_Item);

   exception
      when Constraint_Error => raise Data_Error;
   end Get;

   ---------
   -- Get --
   ---------

   procedure Get
     (Item  : out Complex_Types.Complex;
      Width : in  Field := 0)
   is
   begin
      Get (Current_In, Item, Width);
   end Get;

   ---------
   -- Get --
   ---------

   procedure Get
     (From : in  String;
      Item : out Complex_Types.Complex;
      Last : out Positive)
   is
      Real_Item : Real'Base;
      Imag_Item : Real'Base;

   begin
      Aux.Gets (From, LLF (Real_Item), LLF (Imag_Item), Last);
      Item := (Real_Item, Imag_Item);

   exception
      when Data_Error => raise Constraint_Error;
   end Get;

   ---------
   -- Put --
   ---------

   procedure Put
     (File : in File_Type;
      Item : in Complex_Types.Complex;
      Fore : in Field := Default_Fore;
      Aft  : in Field := Default_Aft;
      Exp  : in Field := Default_Exp)
   is
   begin
      Aux.Put (File, LLF (Re (Item)), LLF (Im (Item)), Fore, Aft, Exp);
   end Put;

   ---------
   -- Put --
   ---------

   procedure Put
     (Item : in Complex_Types.Complex;
      Fore : in Field := Default_Fore;
      Aft  : in Field := Default_Aft;
      Exp  : in Field := Default_Exp)
   is
   begin
      Put (Current_Out, Item, Fore, Aft, Exp);
   end Put;

   ---------
   -- Put --
   ---------

   procedure Put
     (To   : out String;
      Item : in  Complex_Types.Complex;
      Aft  : in  Field := Default_Aft;
      Exp  : in  Field := Default_Exp)
   is
   begin
      Aux.Puts (To, LLF (Re (Item)), LLF (Im (Item)), Aft, Exp);
   end Put;

end Ada.Text_IO.Complex_IO;
