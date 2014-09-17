------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUN-TIME COMPONENTS                         --
--                                                                          --
--              A D A . S T R I N G S . W I D E _ S E A R C H               --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--                                                                          --
--          Copyright (C) 1992-1997 Free Software Foundation, Inc.          --
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

--  This package contains the search functions from Ada.Strings.Wide_Fixed.
--  They are separated out because they are shared by Ada.Strings.Wide_Bounded
--  and Ada.Strings.Wide_Unbounded, and we don't want to drag other irrelevant
--  stuff from Ada.Strings.Wide_Fixed when using the other two packages. We
--  make this a private package, since user programs should access these
--  subprograms via one of the standard string packages.

with Ada.Strings.Wide_Maps;

private package Ada.Strings.Wide_Search is
pragma Preelaborate (Wide_Search);

   function Index (Source   : in Wide_String;
                   Pattern  : in Wide_String;
                   Going    : in Direction := Forward;
                   Mapping  : in Wide_Maps.Wide_Character_Mapping :=
                                          Wide_Maps.Identity)
      return Natural;

   function Index (Source   : in Wide_String;
                   Pattern  : in Wide_String;
                   Going    : in Direction := Forward;
                   Mapping  : in Wide_Maps.Wide_Character_Mapping_Function)
      return Natural;

   function Index (Source : in Wide_String;
                   Set    : in Wide_Maps.Wide_Character_Set;
                   Test   : in Membership := Inside;
                   Going  : in Direction  := Forward)
      return Natural;

   function Index_Non_Blank (Source : in Wide_String;
                             Going  : in Direction := Forward)
      return Natural;

   function Count (Source   : in Wide_String;
                   Pattern  : in Wide_String;
                   Mapping  : in Wide_Maps.Wide_Character_Mapping :=
                                          Wide_Maps.Identity)
      return Natural;

   function Count (Source   : in Wide_String;
                   Pattern  : in Wide_String;
                   Mapping  : in Wide_Maps.Wide_Character_Mapping_Function)
      return Natural;

   function Count (Source   : in Wide_String;
                   Set      : in Wide_Maps.Wide_Character_Set)
      return Natural;


   procedure Find_Token (Source : in Wide_String;
                         Set    : in Wide_Maps.Wide_Character_Set;
                         Test   : in Membership;
                         First  : out Positive;
                         Last   : out Natural);

end Ada.Strings.Wide_Search;
