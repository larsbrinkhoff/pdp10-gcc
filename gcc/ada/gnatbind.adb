------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             G N A T B I N D                              --
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
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- It is now maintained by Ada Core Technologies Inc (http://www.gnat.com). --
--                                                                          --
------------------------------------------------------------------------------

with ALI;      use ALI;
with ALI.Util; use ALI.Util;
with Bcheck;   use Bcheck;
with Binde;    use Binde;
with Binderr;  use Binderr;
with Bindgen;  use Bindgen;
with Bindusg;
with Butil;    use Butil;
with Csets;
with Gnatvsn;  use Gnatvsn;
with Namet;    use Namet;
with Opt;      use Opt;
with Osint;    use Osint;
with Osint.B;  use Osint.B;
with Output;   use Output;
with Switch;   use Switch;
with Switch.B; use Switch.B;
with Targparm; use Targparm;
with Types;    use Types;

procedure Gnatbind is

   Total_Errors : Nat := 0;
   --  Counts total errors in all files

   Total_Warnings : Nat := 0;
   --  Total warnings in all files

   Main_Lib_File : File_Name_Type;
   --  Current main library file

   Std_Lib_File : File_Name_Type;
   --  Standard library

   Text : Text_Buffer_Ptr;
   Id   : ALI_Id;

   Next_Arg : Positive;

   Output_File_Name_Seen : Boolean := False;

   Output_File_Name : String_Ptr := new String'("");

   procedure Scan_Bind_Arg (Argv : String);
   --  Scan and process binder specific arguments. Argv is a single argument.
   --  All the one character arguments are still handled by Switch. This
   --  routine handles -aO -aI and -I-.

   -------------------
   -- Scan_Bind_Arg --
   -------------------

   procedure Scan_Bind_Arg (Argv : String) is
   begin
      --  Now scan arguments that are specific to the binder and are not
      --  handled by the common circuitry in Switch.

      if Opt.Output_File_Name_Present
        and then not Output_File_Name_Seen
      then
         Output_File_Name_Seen := True;

         if Argv'Length = 0
           or else (Argv'Length >= 1 and then Argv (1) = '-')
         then
            Fail ("output File_Name missing after -o");

         else
            Output_File_Name := new String'(Argv);
         end if;

      elsif Argv'Length >= 2 and then Argv (1) = '-' then

         --  -I-

         if Argv (2 .. Argv'Last) = "I-" then
            Opt.Look_In_Primary_Dir := False;

         --  -Idir

         elsif Argv (2) = 'I' then
            Add_Src_Search_Dir (Argv (3 .. Argv'Last));
            Add_Lib_Search_Dir (Argv (3 .. Argv'Last));

         --  -Ldir

         elsif Argv (2) = 'L' then
            if Argv'Length >= 3 then
               Opt.Bind_For_Library := True;
               Opt.Ada_Init_Name :=
                 new String'(Argv (3 .. Argv'Last) & Opt.Ada_Init_Suffix);
               Opt.Ada_Final_Name :=
                 new String'(Argv (3 .. Argv'Last) & Opt.Ada_Final_Suffix);
               Opt.Ada_Main_Name :=
                 new String'(Argv (3 .. Argv'Last) & Opt.Ada_Main_Name_Suffix);

               --  This option (-Lxxx) implies -n

               Opt.Bind_Main_Program := False;
            else
               Fail
                 ("Prefix of initialization and finalization " &
                  "procedure names missing in -L");
            end if;

         --  -Sin -Slo -Shi -Sxx

         elsif Argv'Length = 4
           and then Argv (2) = 'S'
         then
            declare
               C1 : Character := Argv (3);
               C2 : Character := Argv (4);

            begin
               if C1 in 'a' .. 'z' then
                  C1 := Character'Val (Character'Pos (C1) - 32);
               end if;

               if C2 in 'a' .. 'z' then
                  C2 := Character'Val (Character'Pos (C2) - 32);
               end if;

               if C1 = 'I' and then C2 = 'N' then
                  Initialize_Scalars_Mode := 'I';

               elsif C1 = 'L' and then C2 = 'O' then
                  Initialize_Scalars_Mode := 'L';

               elsif C1 = 'H' and then C2 = 'I' then
                  Initialize_Scalars_Mode := 'H';

               elsif (C1 in '0' .. '9' or else C1 in 'A' .. 'F')
                       and then
                     (C2 in '0' .. '9' or else C2 in 'A' .. 'F')
               then
                  Initialize_Scalars_Mode := 'X';
                  Initialize_Scalars_Val (1) := C1;
                  Initialize_Scalars_Val (2) := C2;

               --  Invalid -S switch, let Switch give error

               else
                  Scan_Binder_Switches (Argv);
               end if;
            end;

         --  -aIdir

         elsif Argv'Length >= 3
           and then Argv (2 .. 3) = "aI"
         then
            Add_Src_Search_Dir (Argv (4 .. Argv'Last));

         --  -aOdir

         elsif Argv'Length >= 3
           and then Argv (2 .. 3) = "aO"
         then
            Add_Lib_Search_Dir (Argv (4 .. Argv'Last));

         --  -nostdlib

         elsif Argv (2 .. Argv'Last) = "nostdlib" then
            Opt.No_Stdlib := True;

         --  -nostdinc

         elsif Argv (2 .. Argv'Last) = "nostdinc" then
            Opt.No_Stdinc := True;

         --  -static

         elsif Argv (2 .. Argv'Last) = "static" then
            Opt.Shared_Libgnat := False;

         --  -shared

         elsif Argv (2 .. Argv'Last) = "shared" then
            Opt.Shared_Libgnat := True;

         --  -Mname

         elsif Argv'Length >= 3 and then Argv (2) = 'M' then
            Opt.Bind_Alternate_Main_Name := True;
            Opt.Alternate_Main_Name := new String '(Argv (3 .. Argv'Last));

         --  All other options are single character and are handled
         --  by Scan_Binder_Switches.

         else
            Scan_Binder_Switches (Argv);
         end if;

      --  Not a switch, so must be a file name (if non-empty)

      elsif Argv'Length /= 0 then
         if Argv'Length > 4
           and then Argv (Argv'Last - 3 .. Argv'Last) = ".ali"
         then
            Add_File (Argv);
         else
            Add_File (Argv & ".ali");
         end if;
      end if;
   end Scan_Bind_Arg;

--  Start of processing for Gnatbind

begin

   --  Set default for Shared_Libgnat option

   declare
      Shared_Libgnat_Default : Character;
      pragma Import (C, Shared_Libgnat_Default, "shared_libgnat_default");

      SHARED : constant Character := 'H';
      STATIC : constant Character := 'T';

   begin
      pragma Assert
        (Shared_Libgnat_Default = SHARED
         or else
        Shared_Libgnat_Default = STATIC);
      Shared_Libgnat := (Shared_Libgnat_Default = SHARED);
   end;

   --  Use low level argument routines to avoid dragging in the secondary stack

   Next_Arg := 1;
   Scan_Args : while Next_Arg < Arg_Count loop
      declare
         Next_Argv : String (1 .. Len_Arg (Next_Arg));

      begin
         Fill_Arg (Next_Argv'Address, Next_Arg);
         Scan_Bind_Arg (Next_Argv);
      end;
      Next_Arg := Next_Arg + 1;
   end loop Scan_Args;

   --  Test for trailing -o switch

   if Opt.Output_File_Name_Present
     and then not Output_File_Name_Seen
   then
      Fail ("output file name missing after -o");
   end if;

   --  Output usage if requested

   if Usage_Requested then
      Bindusg;
   end if;

   --  Check that the Ada binder file specified has extension .adb and that
   --  the C binder file has extension .c

   if Opt.Output_File_Name_Present
     and then Output_File_Name_Seen
   then
      Check_Extensions : declare
         Length : constant Natural := Output_File_Name'Length;
         Last   : constant Natural := Output_File_Name'Last;

      begin
         if Ada_Bind_File then
            if Length <= 4
              or else Output_File_Name (Last - 3 .. Last) /= ".adb"
            then
               Fail ("output file name should have .adb extension");
            end if;

         else
            if Length <= 2
              or else Output_File_Name (Last - 1 .. Last) /= ".c"
            then
               Fail ("output file name should have .c extension");
            end if;
         end if;
      end Check_Extensions;
   end if;

   Osint.Add_Default_Search_Dirs;

   if Verbose_Mode then
      Namet.Initialize;
      Targparm.Get_Target_Parameters;

      Write_Eol;
      Write_Str ("GNATBIND ");

      if Targparm.High_Integrity_Mode_On_Target then
         Write_Str ("Pro High Integrity ");
      end if;

      Write_Str (Gnat_Version_String);
      Write_Str (" Copyright 1995-2002 Free Software Foundation, Inc.");
      Write_Eol;
   end if;

   --  Output usage information if no files

   if not More_Lib_Files then
      Bindusg;
      Exit_Program (E_Fatal);
   end if;

   --  The block here is to catch the Unrecoverable_Error exception in the
   --  case where we exceed the maximum number of permissible errors or some
   --  other unrecoverable error occurs.

   begin
      --  Carry out package initializations. These are initializations which
      --  might logically be performed at elaboration time, but Namet at
      --  least can't be done that way (because it is used in the Compiler),
      --  and we decide to be consistent. Like elaboration, the order in
      --  which these calls are made is in some cases important.

      Csets.Initialize;
      Namet.Initialize;
      Initialize_Binderr;
      Initialize_ALI;
      Initialize_ALI_Source;

      if Verbose_Mode then
         Write_Eol;
      end if;

      --  Input ALI files

      while More_Lib_Files loop
         Main_Lib_File := Next_Main_Lib_File;

         if Verbose_Mode then
            if Check_Only then
               Write_Str ("Checking: ");
            else
               Write_Str ("Binding: ");
            end if;

            Write_Name (Main_Lib_File);
            Write_Eol;
         end if;

         Text := Read_Library_Info (Main_Lib_File, True);
         Id := Scan_ALI
                 (F         => Main_Lib_File,
                  T         => Text,
                  Ignore_ED => Force_RM_Elaboration_Order,
                  Err       => False);
         Free (Text);
      end loop;

      --  Add System.Standard_Library to list to ensure that these files are
      --  included in the bind, even if not directly referenced from Ada code
      --  This is of course omitted in No_Run_Time mode

      if not No_Run_Time_Specified then
         Name_Buffer (1 .. 12) := "s-stalib.ali";
         Name_Len := 12;
         Std_Lib_File := Name_Find;
         Text := Read_Library_Info (Std_Lib_File, True);
         Id :=
           Scan_ALI
             (F         => Std_Lib_File,
              T         => Text,
              Ignore_ED => Force_RM_Elaboration_Order,
              Err       => False);
         Free (Text);
      end if;

      --  Acquire all information in ALI files that have been read in

      for Index in ALIs.First .. ALIs.Last loop
         Read_ALI (Index);
      end loop;

      --  Warn if -f switch used

      if Force_RM_Elaboration_Order then
         Error_Msg
           ("?-f is obsolescent and should not be used");
         Error_Msg
           ("?may result in missing run-time elaboration checks");
         Error_Msg
           ("?use -gnatE, pragma Suppress (Elaboration_Checks) instead");
      end if;

      --  Quit if some file needs compiling

      if No_Object_Specified then
         raise Unrecoverable_Error;
      end if;

      --  Build source file table from the ALI files we have read in

      Set_Source_Table;

      --  Check that main library file is a suitable main program

      if Bind_Main_Program
        and then ALIs.Table (ALIs.First).Main_Program = None
        and then not No_Main_Subprogram
      then
         Error_Msg_Name_1 := Main_Lib_File;
         Error_Msg ("% does not contain a unit that can be a main program");
      end if;

      --  Perform consistency and correctness checks

      Check_Duplicated_Subunits;
      Check_Versions;
      Check_Consistency;
      Check_Configuration_Consistency;

      --  Complete bind if no errors

      if Errors_Detected = 0 then
         Find_Elab_Order;

         if Errors_Detected = 0 then
            if Elab_Order_Output then
               Write_Eol;
               Write_Str ("ELABORATION ORDER");
               Write_Eol;

               for J in Elab_Order.First .. Elab_Order.Last loop
                  Write_Str ("   ");
                  Write_Unit_Name (Units.Table (Elab_Order.Table (J)).Uname);
                  Write_Eol;
               end loop;

               Write_Eol;
            end if;

            if not Check_Only then
               Gen_Output_File (Output_File_Name.all);
            end if;
         end if;
      end if;

      Total_Errors := Total_Errors + Errors_Detected;
      Total_Warnings := Total_Warnings + Warnings_Detected;

   exception
      when Unrecoverable_Error =>
         Total_Errors := Total_Errors + Errors_Detected;
         Total_Warnings := Total_Warnings + Warnings_Detected;
   end;

   --  All done. Set proper exit status.

   Finalize_Binderr;
   Namet.Finalize;

   if Total_Errors > 0 then
      Exit_Program (E_Errors);
   elsif Total_Warnings > 0 then
      Exit_Program (E_Warnings);
   else
      Exit_Program (E_Success);
   end if;

end Gnatbind;
