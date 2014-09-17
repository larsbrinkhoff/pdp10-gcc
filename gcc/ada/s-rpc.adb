------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                           S Y S T E M . R P C                            --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                                                                          --
--          Copyright (C) 1992-2002 Free Software Foundation, Inc.          --
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

--  Note: this is a dummy implementation which does not support distribution.
--  All the bodies but one therefore raise an exception as defined below.
--  Establish_RPC_Receiver is callable, so that the ACVC scripts can simulate
--  the presence of a master partition to run a test which is otherwise not
--  distributed.

--  The GLADE distribution package includes a replacement for this file.

with Ada.Exceptions; use Ada.Exceptions;

package body System.RPC is

   GNAT : constant Boolean := True;
   pragma Unreferenced (GNAT);
   --  This dummy entity allows the compiler to recognize that this is the
   --  version of this package that is supplied by GNAT, not by the user.
   --  This is used to cause a compile time error if an attempt is made to
   --  use features in System.RPC that are only available from a true PCS.

   CRLF : constant String := ASCII.CR & ASCII.LF;

   Msg : constant String :=
           CRLF & "Distribution support not installed in your environment" &
           CRLF & "For information on GLADE, contact Ada Core Technologies";

   pragma Warnings (Off);
   --  Kill messages about out parameters not set

   ----------
   -- Read --
   ----------

   procedure Read
     (Stream : in out Params_Stream_Type;
      Item   : out Ada.Streams.Stream_Element_Array;
      Last   : out Ada.Streams.Stream_Element_Offset)
   is
   begin
      Raise_Exception (Program_Error'Identity, Msg);
   end Read;

   -----------
   -- Write --
   -----------

   procedure Write
     (Stream : in out Params_Stream_Type;
      Item   : in Ada.Streams.Stream_Element_Array)
   is
   begin
      Raise_Exception (Program_Error'Identity, Msg);
   end Write;

   ------------
   -- Do_RPC --
   ------------

   procedure Do_RPC
     (Partition : in Partition_ID;
      Params    : access Params_Stream_Type;
      Result    : access Params_Stream_Type)
   is
   begin
      Raise_Exception (Program_Error'Identity, Msg);
   end Do_RPC;

   ------------
   -- Do_APC --
   ------------

   procedure Do_APC
     (Partition : in Partition_ID;
      Params    : access Params_Stream_Type)
   is
   begin
      Raise_Exception (Program_Error'Identity, Msg);
   end Do_APC;

   ----------------------------
   -- Establish_RPC_Receiver --
   ----------------------------

   procedure Establish_RPC_Receiver
     (Partition : in Partition_ID;
      Receiver  : in RPC_Receiver)
   is
   begin
      null;
   end Establish_RPC_Receiver;

end System.RPC;
