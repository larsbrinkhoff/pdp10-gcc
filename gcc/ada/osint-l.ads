------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              O S I N T - L                               --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--                                                                          --
--          Copyright (C) 2001 Free Software Foundation, Inc.               --
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
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- It is now maintained by Ada Core Technologies Inc (http://www.gnat.com). --
--                                                                          --
------------------------------------------------------------------------------

--  This package contains the low level, operating system routines used only
--  in gnatls for command line processing and file input output.

package Osint.L is

   function More_Lib_Files return Boolean;
   --  Indicates whether more library information files remain to be processed.
   --  Returns False right away if no source files, or if all source files
   --  have been processed.

   function Next_Main_Lib_File return File_Name_Type;
   --  This function returns the name of the next library info file specified
   --  on the command line. It is an error to call Next_Main_Lib_File if no
   --  more library information files exist (i.e. Next_Main_Lib_File may be
   --  called only if a previous call to More_Lib_Files returned True). This
   --  name is the simple name, excluding any directory information.

end Osint.L;
