------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              G N A T C M D                               --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                                                                          --
--          Copyright (C) 1996-2002 Free Software Foundation, Inc.          --
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

with GNAT.Directory_Operations; use GNAT.Directory_Operations;

with Csets;
with MLib.Tgt;
with MLib.Utl;
with Namet;    use Namet;
with Opt;
with Osint;    use Osint;
with Output;
with Prj;      use Prj;
with Prj.Env;
with Prj.Ext;  use Prj.Ext;
with Prj.Pars;
with Prj.Util; use Prj.Util;
with Sdefault; use Sdefault;
with Snames;   use Snames;
with Stringt;  use Stringt;
with Table;
with Types;    use Types;
with Hostparm; use Hostparm;
--  Used to determine if we are in VMS or not for error message purposes

with Ada.Characters.Handling; use Ada.Characters.Handling;
with Ada.Command_Line;        use Ada.Command_Line;
with Ada.Text_IO;             use Ada.Text_IO;

with Gnatvsn;
with GNAT.OS_Lib;             use GNAT.OS_Lib;

with Table;

procedure GNATCmd is
   pragma Ident (Gnatvsn.Gnat_Version_String);

   Ada_Include_Path : constant String := "ADA_INCLUDE_PATH";
   Ada_Objects_Path : constant String := "ADA_OBJECTS_PATH";

   Project_File      : String_Access;
   Project           : Prj.Project_Id;
   Current_Verbosity : Prj.Verbosity := Prj.Default;
   Tool_Package_Name : Name_Id       := No_Name;

   --  This flag indicates a switch -p (for gnatxref and gnatfind) for
   --  an old fashioned project file. -p cannot be used in conjonction
   --  with -P.

   Old_Project_File_Used : Boolean := False;

   --  A table to keep the switches on the command line

   package Last_Switches is new Table.Table
     (Table_Component_Type => String_Access,
      Table_Index_Type     => Integer,
      Table_Low_Bound      => 1,
      Table_Initial        => 20,
      Table_Increment      => 100,
      Table_Name           => "Gnatcmd.Last_Switches");

   --  A table to keep the switches from the project file

   package First_Switches is new Table.Table
     (Table_Component_Type => String_Access,
      Table_Index_Type     => Integer,
      Table_Low_Bound      => 1,
      Table_Initial        => 20,
      Table_Increment      => 100,
      Table_Name           => "Gnatcmd.First_Switches");

   ------------------
   -- SWITCH TABLE --
   ------------------

   --  The switch tables contain an entry for each switch recognized by the
   --  command processor. The syntax of entries is as follows:

   --    SWITCH_STRING ::= "/ command-qualifier-name TRANSLATION"

   --    TRANSLATION ::=
   --      DIRECT_TRANSLATION
   --    | DIRECTORIES_TRANSLATION
   --    | FILE_TRANSLATION
   --    | NO_SPACE_FILE_TRANSL
   --    | NUMERIC_TRANSLATION
   --    | STRING_TRANSLATION
   --    | OPTIONS_TRANSLATION
   --    | COMMANDS_TRANSLATION
   --    | ALPHANUMPLUS_TRANSLATION
   --    | OTHER_TRANSLATION

   --    DIRECT_TRANSLATION       ::= space UNIX_SWITCHES
   --    DIRECTORIES_TRANSLATION  ::= =* UNIX_SWITCH *
   --    DIRECTORY_TRANSLATION    ::= =% UNIX_SWITCH %
   --    FILE_TRANSLATION         ::= =@ UNIX_SWITCH @
   --    NO_SPACE_FILE_TRANSL     ::= =< UNIX_SWITCH >
   --    NUMERIC_TRANSLATION      ::= =# UNIX_SWITCH # | # number #
   --    STRING_TRANSLATION       ::= =" UNIX_SWITCH "
   --    OPTIONS_TRANSLATION      ::= =OPTION {space OPTION}
   --    COMMANDS_TRANSLATION     ::= =? ARGS space command-name
   --    ALPHANUMPLUS_TRANSLATION ::= =| UNIX_SWITCH |

   --    UNIX_SWITCHES ::= UNIX_SWITCH {, UNIX_SWITCH}

   --    UNIX_SWITCH ::= unix-switch-string | !unix-switch-string | `string'

   --    OPTION ::= option-name space UNIX_SWITCHES

   --    ARGS ::= -cargs | -bargs | -largs

   --  Here command-qual is the name of the switch recognized by the GNATCmd.
   --  This is always given in upper case in the templates, although in the
   --  actual commands, either upper or lower case is allowed.

   --  The unix-switch-string always starts with a minus, and has no commas
   --  or spaces in it. Case is significant in the unix switch string. If a
   --  unix switch string is preceded by the not sign (!) it means that the
   --  effect of the corresponding command qualifer is to remove any previous
   --  occurrence of the given switch in the command line.

   --  The DIRECTORIES_TRANSLATION format is used where a list of directories
   --  is given. This possible corresponding formats recognized by GNATCmd are
   --  as shown by the following example for the case of PATH

   --    PATH=direc
   --    PATH=(direc,direc,direc,direc)

   --  When more than one directory is present for the DIRECTORIES case, then
   --  multiple instances of the corresponding unix switch are generated,
   --  with the file name being substituted for the occurrence of *.

   --  The FILE_TRANSLATION format is similar except that only a single
   --  file is allowed, not a list of files, and only one unix switch is
   --  generated as a result.

   --  the NO_SPACE_FILE_TRANSL is similar to FILE_TRANSLATION, except that
   --  no space is inserted between the switch and the file name.

   --  The NUMERIC_TRANSLATION format is similar to the FILE_TRANSLATION case
   --  except that the parameter is a decimal integer in the range 0 to 999.

   --  For the OPTIONS_TRANSLATION case, GNATCmd similarly permits one or
   --  more options to appear (although only in some cases does the use of
   --  multiple options make logical sense). For example, taking the
   --  case of ERRORS for GCC, the following are all allowed:

   --    /ERRORS=BRIEF
   --    /ERRORS=(FULL,VERBOSE)
   --    /ERRORS=(BRIEF IMMEDIATE)

   --  If no option is provided (e.g. just /ERRORS is written), then the
   --  first option in the list is the default option. For /ERRORS this
   --  is NORMAL, so /ERRORS with no option is equivalent to /ERRORS=NORMAL.

   --  The COMMANDS_TRANSLATION case is only used for gnatmake, to correspond
   --  to the use of -cargs, -bargs and -largs (the ARGS string as indicated
   --  is one of these three possibilities). The name given by COMMAND is the
   --  corresponding command name to be used to interprete the switches to be
   --  passed on. Switches of this type set modes, e.g. /COMPILER_QUALIFIERS
   --  sets the mode so that all subsequent switches, up to another switch
   --  with COMMANDS_TRANSLATION apply to the corresponding commands issued
   --  by the make utility. For example

   --    /COMPILER_QUALIFIERS /LIST /BINDER_QUALIFIERS /MAIN
   --    /COMPILER_QUALIFIERS /NOLIST /COMPILE_CHECKS=SYNTAX

   --  Clearly these switches must come at the end of the list of switches
   --  since all subsequent switches apply to an issued command.

   --  For the DIRECT_TRANSLATION case, an implicit additional entry is
   --  created by prepending NO to the name of the qualifer, and then
   --  inverting the sense of the UNIX_SWITCHES string. For example,
   --  given the entry:

   --     "/LIST -gnatl"

   --  An implicit entry is created:

   --     "/NOLIST !-gnatl"

   --  In the case where, a ! is already present, inverting the sense of the
   --  switch means removing it.

   subtype S is String;
   --  A synonym to shorten the table

   type String_Ptr is access constant String;
   --  String pointer type used throughout

   type Switches is array (Natural range <>) of String_Ptr;
   --  Type used for array of swtiches

   type Switches_Ptr is access constant Switches;

   --------------------------------
   -- Switches for project files --
   --------------------------------

   S_Ext_Ref      : aliased constant S := "/EXTERNAL_REFERENCE=" & '"'    &
                                            "-X" & '"';

   S_Project_File : aliased constant S := "/PROJECT_FILE=<"               &
                                            "-P>";
   S_Project_Verb : aliased constant S := "/PROJECT_FILE_VERBOSITY="      &
                                            "DEFAULT "                    &
                                               "-vP0 "                    &
                                            "MEDIUM "                     &
                                               "-vP1 "                    &
                                            "HIGH "                       &
                                               "-vP2";

   ----------------------------
   -- Switches for GNAT BIND --
   ----------------------------

   S_Bind_Bind    : aliased constant S := "/BIND_FILE="                    &
                                            "ADA "                         &
                                               "-A "                       &
                                            "C "                           &
                                               "-C";

   S_Bind_Build   : aliased constant S := "/BUILD_LIBRARY=|"               &
                                            "-L|";

   S_Bind_Current : aliased constant S := "/CURRENT_DIRECTORY "            &
                                            "!-I-";

   S_Bind_Debug   : aliased constant S := "/DEBUG="                        &
                                            "TRACEBACK "                   &
                                               "-g2 "                      &
                                            "ALL "                         &
                                               "-g3 "                      &
                                            "NONE "                        &
                                               "-g0 "                      &
                                            "SYMBOLS "                     &
                                               "-g1 "                      &
                                            "NOSYMBOLS "                   &
                                               "!-g1 "                     &
                                            "LINK "                        &
                                               "-g3 "                      &
                                            "NOTRACEBACK "                 &
                                               "!-g2";

   S_Bind_DebugX  : aliased constant S := "/NODEBUG "                      &
                                            "!-g";

   S_Bind_Elab    : aliased constant S := "/ELABORATION_DEPENDENCIES "     &
                                            "-e";

   S_Bind_Error   : aliased constant S := "/ERROR_LIMIT=#"                 &
                                            "-m#";

   S_Bind_Help    : aliased constant S := "/HELP "                         &
                                            "-h";

   S_Bind_Init    : aliased constant S := "/INITIALIZE_SCALARS="           &
                                            "INVALID "                     &
                                               "-Sin "                     &
                                            "LOW "                         &
                                               "-Slo "                     &
                                            "HIGH "                        &
                                               "-Shi";

   S_Bind_Library : aliased constant S := "/LIBRARY_SEARCH=*"              &
                                            "-aO*";

   S_Bind_Linker  : aliased constant S := "/LINKER_OPTION_LIST "           &
                                            "-K";

   S_Bind_List    : aliased constant S := "/LIST_RESTRICTIONS "            &
                                            "-r";

   S_Bind_Main    : aliased constant S := "/MAIN "                         &
                                            "!-n";

   S_Bind_Nostinc : aliased constant S := "/NOSTD_INCLUDES "               &
                                            "-nostdinc";

   S_Bind_Nostlib : aliased constant S := "/NOSTD_LIBRARIES "              &
                                            "-nostdlib";

   S_Bind_No_Time : aliased constant S := "/NO_TIME_STAMP_CHECK "          &
                                            "-t";

   S_Bind_Object  : aliased constant S := "/OBJECT_LIST "                  &
                                            "-O";

   S_Bind_Order   : aliased constant S := "/ORDER_OF_ELABORATION "         &
                                            "-l";

   S_Bind_Output  : aliased constant S := "/OUTPUT=@"                      &
                                            "-o@";

   S_Bind_OutputX : aliased constant S := "/NOOUTPUT "                     &
                                            "-c";

   S_Bind_Pess    : aliased constant S := "/PESSIMISTIC_ELABORATION "      &
                                            "-p";

   S_Bind_Read    : aliased constant S := "/READ_SOURCES="                 &
                                            "ALL "                         &
                                               "-s "                       &
                                            "NONE "                        &
                                               "-x "                       &
                                            "AVAILABLE "                   &
                                               "!-x,!-s";

   S_Bind_ReadX   : aliased constant S := "/NOREAD_SOURCES "               &
                                            "-x";

   S_Bind_Rename  : aliased constant S := "/RENAME_MAIN=<"                  &
                                            "-M>";

   S_Bind_Report  : aliased constant S := "/REPORT_ERRORS="                &
                                            "VERBOSE "                     &
                                               "-v "                       &
                                            "BRIEF "                       &
                                               "-b "                       &
                                            "DEFAULT "                     &
                                               "!-b,!-v";

   S_Bind_ReportX : aliased constant S := "/NOREPORT_ERRORS "              &
                                            "!-b,!-v";

   S_Bind_Restr   : aliased constant S := "/RESTRICTION_LIST "             &
                                            "-r";

   S_Bind_RTS     : aliased constant S := "/RUNTIME_SYSTEM=|"              &
                                            "--RTS=|";

   S_Bind_Search  : aliased constant S := "/SEARCH=*"                      &
                                            "-I*";

   S_Bind_Shared  : aliased constant S := "/SHARED "                       &
                                            "-shared";

   S_Bind_Slice   : aliased constant S := "/TIME_SLICE=#"                  &
                                            "-T#";

   S_Bind_Source  : aliased constant S := "/SOURCE_SEARCH=*"               &
                                            "-aI*";

   S_Bind_Time    : aliased constant S := "/TIME_STAMP_CHECK "             &
                                            "!-t";

   S_Bind_Verbose : aliased constant S := "/VERBOSE "                      &
                                            "-v";

   S_Bind_Warn    : aliased constant S := "/WARNINGS="                     &
                                            "NORMAL "                      &
                                               "!-ws,!-we "                &
                                            "SUPPRESS "                    &
                                               "-ws "                      &
                                            "ERROR "                       &
                                               "-we";

   S_Bind_WarnX   : aliased constant S := "/NOWARNINGS "                   &
     "-ws";

   Bind_Switches : aliased constant Switches :=
     (S_Bind_Bind    'Access,
      S_Bind_Build   'Access,
      S_Bind_Current 'Access,
      S_Bind_Debug   'Access,
      S_Bind_DebugX  'Access,
      S_Bind_Elab    'Access,
      S_Bind_Error   'Access,
      S_Ext_Ref      'Access,
      S_Bind_Help    'Access,
      S_Bind_Init    'Access,
      S_Bind_Library 'Access,
      S_Bind_Linker  'Access,
      S_Bind_List    'Access,
      S_Bind_Main    'Access,
      S_Bind_Nostinc 'Access,
      S_Bind_Nostlib 'Access,
      S_Bind_No_Time 'Access,
      S_Bind_Object  'Access,
      S_Bind_Order   'Access,
      S_Bind_Output  'Access,
      S_Bind_OutputX 'Access,
      S_Bind_Pess    'Access,
      S_Project_File 'Access,
      S_Project_Verb 'Access,
      S_Bind_Read    'Access,
      S_Bind_ReadX   'Access,
      S_Bind_Rename  'Access,
      S_Bind_Report  'Access,
      S_Bind_ReportX 'Access,
      S_Bind_Restr   'Access,
      S_Bind_RTS     'Access,
      S_Bind_Search  'Access,
      S_Bind_Shared  'Access,
      S_Bind_Slice   'Access,
      S_Bind_Source  'Access,
      S_Bind_Time    'Access,
      S_Bind_Verbose 'Access,
      S_Bind_Warn    'Access,
      S_Bind_WarnX   'Access);

   ----------------------------
   -- Switches for GNAT CHOP --
   ----------------------------

   S_Chop_Comp   : aliased constant S := "/COMPILATION "                   &
                                            "-c";

   S_Chop_File   : aliased constant S := "/FILE_NAME_MAX_LENGTH=#"         &
                                            "-k#";

   S_Chop_Help   : aliased constant S := "/HELP "                          &
                                            "-h";

   S_Chop_Over   : aliased constant S := "/OVERWRITE "                     &
                                            "-w";

   S_Chop_Pres   : aliased constant S := "/PRESERVE "                      &
                                            "-p";

   S_Chop_Quiet  : aliased constant S := "/QUIET "                         &
                                            "-q";

   S_Chop_Ref    : aliased constant S := "/REFERENCE "                     &
                                            "-r";

   S_Chop_Verb   : aliased constant S := "/VERBOSE "                       &
                                            "-v";

   Chop_Switches : aliased constant Switches :=
     (S_Chop_Comp   'Access,
      S_Chop_File   'Access,
      S_Chop_Help   'Access,
      S_Chop_Over   'Access,
      S_Chop_Pres   'Access,
      S_Chop_Quiet  'Access,
      S_Chop_Ref    'Access,
      S_Chop_Verb   'Access);

   -------------------------------
   -- Switches for GNAT COMPILE --
   -------------------------------

   S_GCC_Ada_83  : aliased constant S := "/83 "                            &
                                             "-gnat83";

   S_GCC_Ada_95  : aliased constant S := "/95 "                            &
                                             "!-gnat83";

   S_GCC_Asm     : aliased constant S := "/ASM "                           &
                                             "-S,!-c";

   S_GCC_Checks  : aliased constant S := "/CHECKS="                        &
                                             "FULL "                       &
                                                "-gnato,!-gnatE,!-gnatp "  &
                                             "OVERFLOW "                   &
                                                "-gnato "                  &
                                             "ELABORATION "                &
                                                "-gnatE "                  &
                                             "ASSERTIONS "                 &
                                                "-gnata "                  &
                                             "DEFAULT "                    &
                                                "!-gnato,!-gnatp "         &
                                             "SUPPRESS_ALL "               &
                                                "-gnatp";

   S_GCC_ChecksX : aliased constant S := "/NOCHECKS "                      &
                                             "-gnatp,!-gnato,!-gnatE";

   S_GCC_Compres : aliased constant S := "/COMPRESS_NAMES "                &
                                             "-gnatC";

   S_GCC_Config  : aliased constant S := "/CONFIGURATION_PRAGMAS_FILE=<"   &
                                             "-gnatec>";

   S_GCC_Current : aliased constant S := "/CURRENT_DIRECTORY "             &
                                             "!-I-";

   S_GCC_Debug   : aliased constant S := "/DEBUG="                         &
                                            "SYMBOLS "                     &
                                               "-g2 "                      &
                                            "NOSYMBOLS "                   &
                                               "!-g2 "                     &
                                            "TRACEBACK "                   &
                                               "-g1 "                      &
                                            "ALL "                         &
                                               "-g3 "                      &
                                            "NONE "                        &
                                               "-g0 "                      &
                                            "NOTRACEBACK "                 &
                                               "-g0";

   S_GCC_DebugX  : aliased constant S := "/NODEBUG "                       &
                                             "!-g";

   S_GCC_Dist    : aliased constant S := "/DISTRIBUTION_STUBS="            &
                                            "RECEIVER "                    &
                                               "-gnatzr "                  &
                                            "CALLER "                      &
                                              "-gnatzc";

   S_GCC_DistX   : aliased constant S := "/NODISTRIBUTION_STUBS "          &
                                            "!-gnatzr,!-gnatzc";

   S_GCC_Error   : aliased constant S := "/ERROR_LIMIT=#"                  &
                                            "-gnatm#";

   S_GCC_ErrorX  : aliased constant S := "/NOERROR_LIMIT "                 &
                                            "-gnatm999";

   S_GCC_Expand  : aliased constant S := "/EXPAND_SOURCE "                 &
                                            "-gnatG";

   S_GCC_Extend  : aliased constant S := "/EXTENSIONS_ALLOWED "            &
                                            "-gnatX";

   S_GCC_File    : aliased constant S := "/FILE_NAME_MAX_LENGTH=#"         &
                                            "-gnatk#";

   S_GCC_Force   : aliased constant S := "/FORCE_ALI "                     &
                                            "-gnatQ";

   S_GCC_Help    : aliased constant S := "/HELP "                     &
                                            "-gnath";

   S_GCC_Ident   : aliased constant S := "/IDENTIFIER_CHARACTER_SET="      &
                                             "DEFAULT "                    &
                                                "-gnati1 "                 &
                                             "1 "                          &
                                                "-gnati1 "                 &
                                             "2 "                          &
                                                "-gnati2 "                 &
                                             "3 "                          &
                                                "-gnati3 "                 &
                                             "4 "                          &
                                                "-gnati4 "                 &
                                             "5 "                          &
                                                "-gnati5 "                 &
                                             "PC "                         &
                                                "-gnatip "                 &
                                             "PC850 "                      &
                                                "-gnati8 "                 &
                                             "FULL_UPPER "                 &
                                                "-gnatif "                 &
                                             "NO_UPPER "                   &
                                                "-gnatin "                 &
                                             "WIDE "                       &
                                                "-gnatiw";

   S_GCC_IdentX  : aliased constant S := "/NOIDENTIFIER_CHARACTER_SET "    &
                                             "-gnati1";

   S_GCC_Immed   : aliased constant S := "/IMMEDIATE_ERRORS "    &
                                             "-gnatdO";

   S_GCC_Inline  : aliased constant S := "/INLINE="                        &
                                            "PRAGMA "                      &
                                              "-gnatn "                    &
                                            "FULL "                        &
                                              "-gnatN "                    &
                                            "SUPPRESS "                    &
                                              "-fno-inline";

   S_GCC_InlineX : aliased constant S := "/NOINLINE "                      &
                                             "!-gnatn";

   S_GCC_Jumps   : aliased constant S := "/LONGJMP_SETJMP "                &
                                             "-gnatL";

   S_GCC_Length  : aliased constant S := "/MAX_LINE_LENGTH=#"              &
                                             "-gnatyM#";

   S_GCC_List    : aliased constant S := "/LIST "                          &
                                             "-gnatl";

   S_GCC_Noadc   : aliased constant S := "/NO_GNAT_ADC "                   &
                                             "-gnatA";

   S_GCC_Noload  : aliased constant S := "/NOLOAD "                        &
                                             "-gnatc";

   S_GCC_Nostinc : aliased constant S := "/NOSTD_INCLUDES "                &
                                             "-nostdinc";

   S_GCC_Opt     : aliased constant S := "/OPTIMIZE="                      &
                                            "ALL "                         &
                                               "-O2,!-O0,!-O1,!-O3 "       &
                                            "NONE "                        &
                                               "-O0,!-O1,!-O2,!-O3 "       &
                                            "SOME "                        &
                                               "-O1,!-O0,!-O2,!-O3 "       &
                                            "DEVELOPMENT "                 &
                                               "-O1,!-O0,!-O2,!-O3 "       &
                                            "UNROLL_LOOPS "                &
                                               "-funroll-loops "           &
                                            "INLINING "                    &
                                               "-O3,!-O0,!-O1,!-O2";

   S_GCC_OptX    : aliased constant S := "/NOOPTIMIZE "                    &
                                            "-O0,!-O1,!-O2,!-O3";

   S_GCC_Polling : aliased constant S := "/POLLING "                       &
                                            "-gnatP";

   S_GCC_Report  : aliased constant S := "/REPORT_ERRORS="                 &
                                            "VERBOSE "                     &
                                               "-gnatv "                   &
                                            "BRIEF "                       &
                                               "-gnatb "                   &
                                            "FULL "                        &
                                               "-gnatf "                   &
                                            "IMMEDIATE "                   &
                                               "-gnate "                   &
                                            "DEFAULT "                     &
                                               "!-gnatb,!-gnatv";

   S_GCC_ReportX : aliased constant S := "/NOREPORT_ERRORS "               &
                                            "!-gnatb,!-gnatv";

   S_GCC_Repinfo : aliased constant S := "/REPRESENTATION_INFO="           &
                                            "ARRAYS "                      &
                                               "-gnatR1 "                  &
                                            "NONE "                        &
                                               "-gnatR0 "                  &
                                            "OBJECTS "                     &
                                               "-gnatR2 "                  &
                                            "SYMBOLIC "                    &
                                               "-gnatR3 "                  &
                                            "DEFAULT "                     &
                                               "-gnatR";

   S_GCC_RepinfX : aliased constant S := "/NOREPRESENTATION_INFO "         &
                                            "!-gnatR";

   S_GCC_Search  : aliased constant S := "/SEARCH=*"                       &
                                            "-I*";

   S_GCC_Style   : aliased constant S := "/STYLE_CHECKS="                  &
                                            "ALL_BUILTIN "                 &
                                               "-gnaty "                   &
                                            "1 "                           &
                                               "-gnaty1 "                  &
                                            "2 "                           &
                                               "-gnaty2 "                  &
                                            "3 "                           &
                                               "-gnaty3 "                  &
                                            "4 "                           &
                                               "-gnaty4 "                  &
                                            "5 "                           &
                                               "-gnaty5 "                  &
                                            "6 "                           &
                                               "-gnaty6 "                  &
                                            "7 "                           &
                                               "-gnaty7 "                  &
                                            "8 "                           &
                                               "-gnaty8 "                  &
                                            "9 "                           &
                                               "-gnaty9 "                  &
                                            "ATTRIBUTE "                   &
                                               "-gnatya "                  &
                                            "BLANKS "                      &
                                               "-gnatyb "                  &
                                            "COMMENTS "                    &
                                               "-gnatyc "                  &
                                            "END "                         &
                                               "-gnatye "                  &
                                            "VTABS "                       &
                                               "-gnatyf "                  &
                                            "GNAT "                        &
                                               "-gnatg "                   &
                                            "HTABS "                       &
                                               "-gnatyh "                  &
                                            "IF_THEN "                     &
                                               "-gnatyi "                  &
                                            "KEYWORD "                     &
                                               "-gnatyk "                  &
                                            "LAYOUT "                      &
                                               "-gnatyl "                  &
                                            "LINE_LENGTH "                 &
                                               "-gnatym "                  &
                                            "STANDARD_CASING "             &
                                               "-gnatyn "                  &
                                            "ORDERED_SUBPROGRAMS "         &
                                               "-gnatyo "                  &
                                            "NONE "                        &
                                               "!-gnatg,!-gnatr "          &
                                            "PRAGMA "                      &
                                               "-gnatyp "                  &
                                            "RM_COLUMN_LAYOUT "            &
                                               "-gnatr "                   &
                                            "SPECS "                       &
                                               "-gnatys "                  &
                                            "TOKEN "                       &
                                               "-gnatyt ";

   S_GCC_StyleX  : aliased constant S := "/NOSTYLE_CHECKS "                &
                                            "!-gnatg,!-gnatr";

   S_GCC_Syntax  : aliased constant S := "/SYNTAX_ONLY "                   &
                                            "-gnats";

   S_GCC_Trace   : aliased constant S := "/TRACE_UNITS "                   &
                                            "-gnatdc";

   S_GCC_Tree    : aliased constant S := "/TREE_OUTPUT "                   &
                                            "-gnatt";

   S_GCC_Trys    : aliased constant S := "/TRY_SEMANTICS "                 &
                                            "-gnatq";

   S_GCC_Units   : aliased constant S := "/UNITS_LIST "                    &
                                            "-gnatu";

   S_GCC_Unique  : aliased constant S := "/UNIQUE_ERROR_TAG "              &
                                            "-gnatU";

   S_GCC_Upcase  : aliased constant S := "/UPPERCASE_EXTERNALS "           &
                                            "-gnatF";

   S_GCC_Valid   : aliased constant S := "/VALIDITY_CHECKING="             &
                                            "DEFAULT "                     &
                                               "-gnatVd "                  &
                                            "NODEFAULT "                   &
                                               "-gnatVD "                  &
                                            "COPIES "                      &
                                               "-gnatVc "                  &
                                            "NOCOPIES "                    &
                                               "-gnatVC "                  &
                                            "FLOATS "                      &
                                               "-gnatVf "                  &
                                            "NOFLOATS "                    &
                                               "-gnatVF "                  &
                                            "IN_PARAMS "                   &
                                               "-gnatVi "                  &
                                            "NOIN_PARAMS "                 &
                                               "-gnatVI "                  &
                                            "MOD_PARAMS "                  &
                                               "-gnatVm "                  &
                                            "NOMOD_PARAMS "                &
                                               "-gnatVM "                  &
                                            "OPERANDS "                    &
                                               "-gnatVo "                  &
                                            "NOOPERANDS "                  &
                                               "-gnatVO "                  &
                                            "RETURNS "                     &
                                               "-gnatVr "                  &
                                            "NORETURNS "                   &
                                               "-gnatVR "                  &
                                            "SUBSCRIPTS "                  &
                                               "-gnatVs "                  &
                                            "NOSUBSCRIPTS "                &
                                               "-gnatVS "                  &
                                            "TESTS "                       &
                                               "-gnatVt "                  &
                                            "NOTESTS "                     &
                                               "-gnatVT "                  &
                                            "ALL "                         &
                                               "-gnatVa "                  &
                                            "NONE "                        &
                                               "-gnatVn";

   S_GCC_Verbose : aliased constant S := "/VERBOSE "                       &
                                            "-v";

   S_GCC_Warn    : aliased constant S := "/WARNINGS="                      &
                                            "DEFAULT "                     &
                                               "!-gnatws,!-gnatwe "        &
                                            "ALL_GCC "                     &
                                               "-Wall "                    &
                                            "BIASED_ROUNDING "             &
                                               "-gnatwb "                  &
                                            "NOBIASED_ROUNDING "           &
                                               "-gnatwB "                  &
                                            "CONDITIONALS "                &
                                               "-gnatwc "                  &
                                            "NOCONDITIONALS "              &
                                               "-gnatwC "                  &
                                            "IMPLICIT_DEREFERENCE "        &
                                               "-gnatwd "                  &
                                            "NO_IMPLICIT_DEREFERENCE "     &
                                               "-gnatwD "                  &
                                            "ELABORATION "                 &
                                               "-gnatwl "                  &
                                            "NOELABORATION "               &
                                               "-gnatwL "                  &
                                            "ERRORS "                      &
                                               "-gnatwe "                  &
                                            "HIDING "                      &
                                               "-gnatwh "                  &
                                            "NOHIDING "                    &
                                               "-gnatwH "                  &
                                            "IMPLEMENTATION "              &
                                               "-gnatwi "                  &
                                            "NOIMPLEMENTATION "            &
                                               "-gnatwI "                  &
                                            "INEFFECTIVE_INLINE "          &
                                               "-gnatwp "                  &
                                            "NOINEFFECTIVE_INLINE "        &
                                               "-gnatwP "                  &
                                            "OPTIONAL "                    &
                                               "-gnatwa "                  &
                                            "NOOPTIONAL "                  &
                                               "-gnatwA "                  &
                                            "OVERLAYS "                    &
                                               "-gnatwo "                  &
                                            "NOOVERLAYS "                  &
                                               "-gnatwO "                  &
                                            "REDUNDANT "                   &
                                               "-gnatwr "                  &
                                            "NOREDUNDANT "                 &
                                               "-gnatwR "                  &
                                            "SUPPRESS "                    &
                                               "-gnatws "                  &
                                            "UNINITIALIZED "               &
                                               "-Wuninitialized "          &
                                            "UNREFERENCED_FORMALS "        &
                                               "-gnatwf "                  &
                                            "NOUNREFERENCED_FORMALS "      &
                                               "-gnatwF "                  &
                                            "UNUSED "                      &
                                               "-gnatwu "                  &
                                            "NOUNUSED "                    &
                                               "-gnatwU";

   S_GCC_WarnX   : aliased constant S := "/NOWARNINGS "                    &
                                            "-gnatws";

   S_GCC_Wide    : aliased constant S := "/WIDE_CHARACTER_ENCODING="       &
                                             "BRACKETS "                   &
                                                "-gnatWb "                 &
                                             "NONE "                       &
                                                "-gnatWn "                 &
                                             "HEX "                        &
                                                "-gnatWh "                 &
                                             "UPPER "                      &
                                                "-gnatWu "                 &
                                             "SHIFT_JIS "                  &
                                                "-gnatWs "                 &
                                             "UTF8 "                       &
                                                "-gnatW8 "                 &
                                             "EUC "                        &
                                                "-gnatWe";

   S_GCC_WideX   : aliased constant S := "/NOWIDE_CHARACTER_ENCODING "     &
                                             "-gnatWn";

   S_GCC_Xdebug  : aliased constant S := "/XDEBUG "                        &
                                             "-gnatD";

   S_GCC_Xref    : aliased constant S := "/XREF="                          &
                                            "GENERATE "                    &
                                               "!-gnatx "                  &
                                            "SUPPRESS "                    &
                                               "-gnatx";

   GCC_Switches : aliased constant Switches :=
     (S_GCC_Ada_83  'Access,
      S_GCC_Ada_95  'Access,
      S_GCC_Asm     'Access,
      S_GCC_Checks  'Access,
      S_GCC_ChecksX 'Access,
      S_GCC_Compres 'Access,
      S_GCC_Config  'Access,
      S_GCC_Current 'Access,
      S_GCC_Debug   'Access,
      S_GCC_DebugX  'Access,
      S_GCC_Dist    'Access,
      S_GCC_DistX   'Access,
      S_GCC_Error   'Access,
      S_GCC_ErrorX  'Access,
      S_GCC_Expand  'Access,
      S_GCC_Extend  'Access,
      S_Ext_Ref     'Access,
      S_GCC_File    'Access,
      S_GCC_Force   'Access,
      S_GCC_Help    'Access,
      S_GCC_Ident   'Access,
      S_GCC_IdentX  'Access,
      S_GCC_Immed   'Access,
      S_GCC_Inline  'Access,
      S_GCC_InlineX 'Access,
      S_GCC_Jumps   'Access,
      S_GCC_Length  'Access,
      S_GCC_List    'Access,
      S_GCC_Noadc   'Access,
      S_GCC_Noload  'Access,
      S_GCC_Nostinc 'Access,
      S_GCC_Opt     'Access,
      S_GCC_OptX    'Access,
      S_GCC_Polling 'Access,
      S_Project_File'Access,
      S_Project_Verb'Access,
      S_GCC_Report  'Access,
      S_GCC_ReportX 'Access,
      S_GCC_Repinfo 'Access,
      S_GCC_RepinfX 'Access,
      S_GCC_Search  'Access,
      S_GCC_Style   'Access,
      S_GCC_StyleX  'Access,
      S_GCC_Syntax  'Access,
      S_GCC_Trace   'Access,
      S_GCC_Tree    'Access,
      S_GCC_Trys    'Access,
      S_GCC_Units   'Access,
      S_GCC_Unique  'Access,
      S_GCC_Upcase  'Access,
      S_GCC_Valid   'Access,
      S_GCC_Verbose 'Access,
      S_GCC_Warn    'Access,
      S_GCC_WarnX   'Access,
      S_GCC_Wide    'Access,
      S_GCC_WideX   'Access,
      S_GCC_Xdebug  'Access,
      S_GCC_Xref    'Access);

   ----------------------------
   -- Switches for GNAT ELIM --
   ----------------------------

   S_Elim_All    : aliased constant S := "/ALL "                           &
                                            "-a";

   S_Elim_Bind   : aliased constant S := "/BIND_FILE=<"                    &
                                            "-b>";

   S_Elim_Miss   : aliased constant S := "/MISSED "                        &
                                            "-m";

   S_Elim_Quiet  : aliased constant S := "/QUIET "                         &
                                            "-q";

   S_Elim_Tree   : aliased constant S := "/TREE_DIRS=*"                    &
                                            "-T*";

   S_Elim_Verb   : aliased constant S := "/VERBOSE "                       &
                                            "-v";

   Elim_Switches : aliased constant Switches :=
     (S_Elim_All    'Access,
      S_Elim_Bind   'Access,
      S_Elim_Miss   'Access,
      S_Elim_Quiet  'Access,
      S_Elim_Tree   'Access,
      S_Elim_Verb   'Access);

   ----------------------------
   -- Switches for GNAT FIND --
   ----------------------------

   S_Find_All     : aliased constant S := "/ALL_FILES "                    &
                                            "-a";

   S_Find_Deriv   : aliased constant S := "/DERIVED_TYPE_INFORMATION "     &
                                            "-d";

   S_Find_Expr    : aliased constant S := "/EXPRESSIONS "                  &
                                            "-e";

   S_Find_Full    : aliased constant S := "/FULL_PATHNAME "                &
                                            "-f";

   S_Find_Ignore  : aliased constant S := "/IGNORE_LOCALS "                &
                                            "-g";

   S_Find_Nostinc : aliased constant S := "/NOSTD_INCLUDES "               &
                                            "-nostdinc";

   S_Find_Nostlib : aliased constant S := "/NOSTD_LIBRARIES "              &
                                            "-nostdlib";

   S_Find_Object  : aliased constant S := "/OBJECT_SEARCH=*"               &
                                            "-aO*";

   S_Find_Print   : aliased constant S := "/PRINT_LINES "                  &
                                            "-s";

   S_Find_Project : aliased constant S := "/PROJECT=@"                     &
                                            "-p@";

   S_Find_Ref     : aliased constant S := "/REFERENCES "                   &
                                            "-r";

   S_Find_Search  : aliased constant S := "/SEARCH=*"                      &
                                            "-I*";

   S_Find_Source  : aliased constant S := "/SOURCE_SEARCH=*"               &
                                            "-aI*";

   S_Find_Types   : aliased constant S := "/TYPE_HIERARCHY "               &
                                            "-t";

   Find_Switches : aliased constant Switches :=
     (S_Find_All     'Access,
      S_Find_Deriv   'Access,
      S_Find_Expr    'Access,
      S_Ext_Ref      'Access,
      S_Find_Full    'Access,
      S_Find_Ignore  'Access,
      S_Find_Nostinc 'Access,
      S_Find_Nostlib 'Access,
      S_Find_Object  'Access,
      S_Find_Print   'Access,
      S_Find_Project 'Access,
      S_Project_File 'Access,
      S_Project_Verb 'Access,
      S_Find_Ref     'Access,
      S_Find_Search  'Access,
      S_Find_Source  'Access,
      S_Find_Types   'Access);

   ------------------------------
   -- Switches for GNAT KRUNCH --
   ------------------------------

   S_Krunch_Count  : aliased constant S := "/COUNT=#"                      &
                                            "`#";

   Krunch_Switches : aliased constant Switches  :=
     (1 .. 1 => S_Krunch_Count  'Access);

   -------------------------------
   -- Switches for GNAT LIBRARY --
   -------------------------------

   S_Lbr_Config    : aliased constant S := "/CONFIG=@"                     &
                                            "--config=@";

   S_Lbr_Create    : aliased constant S := "/CREATE=%"                     &
                                            "--create=%";

   S_Lbr_Delete    : aliased constant S := "/DELETE=%"                     &
                                            "--delete=%";

   S_Lbr_Set       : aliased constant S := "/SET=%"                        &
                                            "--set=%";

   Lbr_Switches : aliased constant Switches  :=
     (S_Lbr_Config 'Access,
      S_Lbr_Create 'Access,
      S_Lbr_Delete 'Access,
      S_Lbr_Set    'Access);

   ----------------------------
   -- Switches for GNAT LINK --
   ----------------------------

   S_Link_Bind    : aliased constant S := "/BIND_FILE="                    &
                                            "ADA "                         &
                                               "-A "                       &
                                            "C "                           &
                                               "-C";

   S_Link_Debug   : aliased constant S := "/DEBUG="                        &
                                            "ALL "                         &
                                               "-g3 "                      &
                                            "NONE "                        &
                                               "-g0 "                      &
                                            "TRACEBACK "                   &
                                               "-g1 "                      &
                                            "NOTRACEBACK "                 &
                                               "-g0";

   S_Link_Execut  : aliased constant S := "/EXECUTABLE=@"                  &
                                            "-o@";

   S_Link_Force   : aliased constant S := "/FORCE_OBJECT_FILE_LIST "       &
                                            "-f";

   S_Link_Ident   : aliased constant S := "/IDENTIFICATION=" & '"'         &
                                            "--for-linker=IDENT="          &
                                            '"';

   S_Link_Nocomp  : aliased constant S := "/NOCOMPILE "                    &
                                            "-n";

   S_Link_Nofiles : aliased constant S := "/NOSTART_FILES "                &
                                            "-nostartfiles";

   S_Link_Noinhib : aliased constant S := "/NOINHIBIT-EXEC "               &
                                            "--for-linker=--noinhibit-exec";

   S_Link_Static  : aliased constant S := "/STATIC "                       &
                                            "--for-linker=-static";

   S_Link_Verb    : aliased constant S := "/VERBOSE "                      &
                                            "-v";

   S_Link_ZZZZZ   : aliased constant S := "/<other> "                      &
                                            "--for-linker=";

   Link_Switches : aliased constant Switches :=
     (S_Link_Bind    'Access,
      S_Link_Debug   'Access,
      S_Link_Execut  'Access,
      S_Ext_Ref      'Access,
      S_Link_Force   'Access,
      S_Link_Ident   'Access,
      S_Link_Nocomp  'Access,
      S_Link_Nofiles 'Access,
      S_Link_Noinhib 'Access,
      S_Project_File 'Access,
      S_Project_Verb 'Access,
      S_Link_Static  'Access,
      S_Link_Verb    'Access,
      S_Link_ZZZZZ   'Access);

   ----------------------------
   -- Switches for GNAT LIST --
   ----------------------------

   S_List_All     : aliased constant S := "/ALL_UNITS "                    &
                                            "-a";

   S_List_Current : aliased constant S := "/CURRENT_DIRECTORY "            &
                                            "!-I-";

   S_List_Nostinc : aliased constant S := "/NOSTD_INCLUDES "               &
                                            "-nostdinc";

   S_List_Object  : aliased constant S := "/OBJECT_SEARCH=*"               &
                                            "-aO*";

   S_List_Output  : aliased constant S := "/OUTPUT="                       &
                                            "SOURCES "                     &
                                               "-s "                       &
                                            "DEPEND "                      &
                                               "-d "                       &
                                            "OBJECTS "                     &
                                               "-o "                       &
                                            "UNITS "                       &
                                               "-u "                       &
                                            "OPTIONS "                     &
                                               "-h "                       &
                                            "VERBOSE "                     &
                                               "-v ";

   S_List_Search  : aliased constant S := "/SEARCH=*"                      &
                                            "-I*";

   S_List_Source  : aliased constant S := "/SOURCE_SEARCH=*"               &
                                            "-aI*";

   List_Switches : aliased constant Switches :=
     (S_List_All     'Access,
      S_List_Current 'Access,
      S_Ext_Ref      'Access,
      S_List_Nostinc 'Access,
      S_List_Object  'Access,
      S_List_Output  'Access,
      S_Project_File 'Access,
      S_Project_Verb 'Access,
      S_List_Search  'Access,
      S_List_Source  'Access);

   ----------------------------
   -- Switches for GNAT MAKE --
   ----------------------------

   S_Make_Actions : aliased constant S := "/ACTIONS="                      &
                                            "COMPILE "                     &
                                               "-c "                       &
                                            "BIND "                        &
                                               "-b "                       &
                                            "LINK "                        &
                                               "-l ";

   S_Make_All     : aliased constant S := "/ALL_FILES "                    &
                                            "-a";

   S_Make_Bind    : aliased constant S := "/BINDER_QUALIFIERS=?"           &
                                            "-bargs BIND";

   S_Make_Comp    : aliased constant S := "/COMPILER_QUALIFIERS=?"         &
                                            "-cargs COMPILE";

   S_Make_Cond    : aliased constant S := "/CONDITIONAL_SOURCE_SEARCH=*"   &
                                            "-A*";

   S_Make_Cont    : aliased constant S := "/CONTINUE_ON_ERROR "            &
                                            "-k";

   S_Make_Current : aliased constant S := "/CURRENT_DIRECTORY "            &
                                            "!-I-";

   S_Make_Dep     : aliased constant S := "/DEPENDENCIES_LIST "            &
                                            "-M";

   S_Make_Doobj   : aliased constant S := "/DO_OBJECT_CHECK "              &
                                            "-n";

   S_Make_Execut  : aliased constant S := "/EXECUTABLE=@"                  &
                                            "-o@";

   S_Make_Force   : aliased constant S := "/FORCE_COMPILE "                &
                                            "-f";

   S_Make_Inplace : aliased constant S := "/IN_PLACE "                     &
                                            "-i";

   S_Make_Library : aliased constant S := "/LIBRARY_SEARCH=*"              &
                                            "-L*";

   S_Make_Link    : aliased constant S := "/LINKER_QUALIFIERS=?"           &
                                            "-largs LINK";

   S_Make_Mapping : aliased constant S := "/MAPPING "                      &
                                            "-C";

   S_Make_Minimal : aliased constant S := "/MINIMAL_RECOMPILATION "        &
                                            "-m";

   S_Make_Nolink  : aliased constant S := "/NOLINK "                       &
                                            "-c";

   S_Make_Nomain  : aliased constant S := "/NOMAIN "                       &
                                            "-z";

   S_Make_Nostinc : aliased constant S := "/NOSTD_INCLUDES "               &
                                            "-nostdinc";

   S_Make_Nostlib : aliased constant S := "/NOSTD_LIBRARIES "              &
                                            "-nostdlib";

   S_Make_Object  : aliased constant S := "/OBJECT_SEARCH=*"               &
                                            "-aO*";

   S_Make_Proc    : aliased constant S := "/PROCESSES=#"                   &
                                            "-j#";

   S_Make_Nojobs  : aliased constant S := "/NOPROCESSES "                  &
                                            "-j1";

   S_Make_Quiet   : aliased constant S := "/QUIET "                        &
                                            "-q";

   S_Make_Reason  : aliased constant S := "/REASONS "                      &
                                            "-v";

   S_Make_RTS     : aliased constant S := "/RUNTIME_SYSTEM=|"              &
                                            "--RTS=|";

   S_Make_Search  : aliased constant S := "/SEARCH=*"                      &
                                            "-I*";

   S_Make_Skip    : aliased constant S := "/SKIP_MISSING=*"                &
                                            "-aL*";

   S_Make_Source  : aliased constant S := "/SOURCE_SEARCH=*"               &
                                            "-aI*";

   S_Make_Switch  : aliased constant S := "/SWITCH_CHECK "               &
                                            "-s";

   S_Make_Unique  : aliased constant S := "/UNIQUE "                       &
                                            "-u";

   S_Make_Verbose : aliased constant S := "/VERBOSE "                      &
                                            "-v";

   Make_Switches : aliased constant Switches :=
     (S_Make_Actions 'Access,
      S_Make_All     'Access,
      S_Make_Bind    'Access,
      S_Make_Comp    'Access,
      S_Make_Cond    'Access,
      S_Make_Cont    'Access,
      S_Make_Current 'Access,
      S_Make_Dep     'Access,
      S_Make_Doobj   'Access,
      S_Make_Execut  'Access,
      S_Ext_Ref      'Access,
      S_Make_Force   'Access,
      S_Make_Inplace 'Access,
      S_Make_Library 'Access,
      S_Make_Link    'Access,
      S_Make_Mapping 'Access,
      S_Make_Minimal 'Access,
      S_Make_Nolink  'Access,
      S_Make_Nomain  'Access,
      S_Make_Nostinc 'Access,
      S_Make_Nostlib 'Access,
      S_Make_Object  'Access,
      S_Make_Proc    'Access,
      S_Project_File 'Access,
      S_Project_Verb 'Access,
      S_Make_Nojobs  'Access,
      S_Make_Quiet   'Access,
      S_Make_Reason  'Access,
      S_Make_RTS     'Access,
      S_Make_Search  'Access,
      S_Make_Skip    'Access,
      S_Make_Source  'Access,
      S_Make_Switch  'Access,
      S_Make_Unique  'Access,
      S_Make_Verbose 'Access);

   ----------------------------
   -- Switches for GNAT Name --
   ----------------------------

   S_Name_Conf    : aliased constant S := "/CONFIG_FILE=<"                   &
                                            "-c>";

   S_Name_Dirs    : aliased constant S := "/SOURCE_DIRS=*"                   &
                                            "-d*";

   S_Name_Dfile   : aliased constant S := "/DIRS_FILE=<"                     &
                                            "-D>";

   S_Name_Help    : aliased constant S := "/HELP"                            &
                                            " -h";

   S_Name_Proj    : aliased constant S := "/PROJECT_FILE=<"                  &
                                            "-P>";

   S_Name_Verbose : aliased constant S := "/VERBOSE"                         &
                                            " -v";

   Name_Switches : aliased constant Switches :=
     (S_Name_Conf         'Access,
      S_Name_Dirs         'Access,
      S_Name_Dfile        'Access,
      S_Name_Help         'Access,
      S_Name_Proj         'Access,
      S_Name_Verbose      'Access);

   ----------------------------------
   -- Switches for GNAT PREPROCESS --
   ----------------------------------

   S_Prep_Assoc   : aliased constant S := "/ASSOCIATE=" & '"'               &
                                            "-D" & '"';

   S_Prep_Blank   : aliased constant S := "/BLANK_LINES "                   &
                                            "-b";

   S_Prep_Com     : aliased constant S := "/COMMENTS "                      &
                                            "-c";

   S_Prep_Ref     : aliased constant S := "/REFERENCE "                     &
                                            "-r";

   S_Prep_Remove  : aliased constant S := "/REMOVE "                        &
                                            "!-b,!-c";

   S_Prep_Symbols : aliased constant S := "/SYMBOLS "                       &
                                            "-s";

   S_Prep_Undef   : aliased constant S := "/UNDEFINED "                     &
                                            "-u";

   Prep_Switches : aliased constant Switches :=
     (S_Prep_Assoc   'Access,
      S_Prep_Blank   'Access,
      S_Prep_Com     'Access,
      S_Prep_Ref     'Access,
      S_Prep_Remove  'Access,
      S_Prep_Symbols 'Access,
      S_Prep_Undef   'Access);

   ------------------------------
   -- Switches for GNAT SHARED --
   ------------------------------

   S_Shared_Debug   : aliased constant S := "/DEBUG="                      &
                                            "ALL "                         &
                                               "-g3 "                      &
                                            "NONE "                        &
                                               "-g0 "                      &
                                            "TRACEBACK "                   &
                                               "-g1 "                      &
                                            "NOTRACEBACK "                 &
                                               "-g0";

   S_Shared_Image  : aliased constant S := "/IMAGE=@"                      &
                                            "-o@";

   S_Shared_Ident   : aliased constant S := "/IDENTIFICATION=" & '"'       &
                                            "--for-linker=IDENT="          &
                                            '"';

   S_Shared_Nofiles : aliased constant S := "/NOSTART_FILES "              &
                                            "-nostartfiles";

   S_Shared_Noinhib : aliased constant S := "/NOINHIBIT-IMAGE "            &
                                            "--for-linker=--noinhibit-exec";

   S_Shared_Verb    : aliased constant S := "/VERBOSE "                    &
                                            "-v";

   S_Shared_ZZZZZ   : aliased constant S := "/<other> "                    &
                                            "--for-linker=";

   Shared_Switches : aliased constant Switches :=
     (S_Shared_Debug   'Access,
      S_Shared_Image   'Access,
      S_Shared_Ident   'Access,
      S_Shared_Nofiles 'Access,
      S_Shared_Noinhib 'Access,
      S_Shared_Verb    'Access,
      S_Shared_ZZZZZ   'Access);

   --------------------------------
   -- Switches for GNAT STANDARD --
   --------------------------------

   Standard_Switches : aliased constant Switches := (1 .. 0 => null);

   ----------------------------
   -- Switches for GNAT STUB --
   ----------------------------

   S_Stub_Current : aliased constant S := "/CURRENT_DIRECTORY "            &
                                            "!-I-";

   S_Stub_Full    : aliased constant S := "/FULL "                         &
                                            "-f";

   S_Stub_Header  : aliased constant S := "/HEADER="                       &
                                            "GENERAL "                     &
                                               "-hg "                      &
                                            "SPEC "                        &
                                               "-hs";

   S_Stub_Indent  : aliased constant S := "/INDENTATION=#"                 &
                                            "-i#";

   S_Stub_Length  : aliased constant S := "/LINE_LENGTH=#"                 &
                                            "-l#";

   S_Stub_Quiet   : aliased constant S := "/QUIET "                        &
                                            "-q";

   S_Stub_Search  : aliased constant S := "/SEARCH=*"                      &
                                            "-I*";

   S_Stub_Tree    : aliased constant S := "/TREE_FILE="                    &
                                            "OVERWRITE "                   &
                                               "-t "                       &
                                            "SAVE "                        &
                                               "-k "                       &
                                            "REUSE "                       &
                                               "-r";

   S_Stub_Verbose : aliased constant S := "/VERBOSE "                      &
                                            "-v";

   Stub_Switches : aliased constant Switches :=
     (S_Stub_Current 'Access,
      S_Stub_Full    'Access,
      S_Stub_Header  'Access,
      S_Stub_Indent  'Access,
      S_Stub_Length  'Access,
      S_Stub_Quiet   'Access,
      S_Stub_Search  'Access,
      S_Stub_Tree    'Access,
      S_Stub_Verbose 'Access);

   ----------------------------
   -- Switches for GNAT XREF --
   ----------------------------

   S_Xref_All     : aliased constant S := "/ALL_FILES "                    &
                                            "-a";

   S_Xref_Deriv   : aliased constant S := "/DERIVED_TYPES "                &
                                            "-d";

   S_Xref_Full    : aliased constant S := "/FULL_PATHNAME "                &
                                            "-f";

   S_Xref_Global  : aliased constant S := "/IGNORE_LOCALS "                &
                                            "-g";

   S_Xref_Nostinc : aliased constant S := "/NOSTD_INCLUDES "               &
                                            "-nostdinc";

   S_Xref_Nostlib : aliased constant S := "/NOSTD_LIBRARIES "              &
                                            "-nostdlib";

   S_Xref_Object  : aliased constant S := "/OBJECT_SEARCH=*"               &
                                            "-aO*";

   S_Xref_Project : aliased constant S := "/PROJECT=@"                     &
                                            "-p@";

   S_Xref_Search  : aliased constant S := "/SEARCH=*"                      &
                                            "-I*";

   S_Xref_Source  : aliased constant S := "/SOURCE_SEARCH=*"               &
                                            "-aI*";

   S_Xref_Output  : aliased constant S := "/UNUSED "                       &
                                            "-u";

   S_Xref_Tags    : aliased constant S := "/TAGS "                         &
                                            "-v";

   Xref_Switches : aliased constant Switches :=
     (S_Xref_All     'Access,
      S_Xref_Deriv   'Access,
      S_Ext_Ref      'Access,
      S_Xref_Full    'Access,
      S_Xref_Global  'Access,
      S_Xref_Nostinc 'Access,
      S_Xref_Nostlib 'Access,
      S_Xref_Object  'Access,
      S_Xref_Project 'Access,
      S_Project_File 'Access,
      S_Project_Verb 'Access,
      S_Xref_Search  'Access,
      S_Xref_Source  'Access,
      S_Xref_Output  'Access,
      S_Xref_Tags    'Access);

   -------------------
   -- COMMAND TABLE --
   -------------------

   --  The command table contains an entry for each command recognized by
   --  GNATCmd. The entries are represented by an array of records.

   type Parameter_Type is
   --  A parameter is defined as a whitespace bounded string, not begining
   --   with a slash. (But see note under FILES_OR_WILDCARD).
     (File,
      --  A required file or directory parameter.

      Optional_File,
      --  An optional file or directory parameter.

      Other_As_Is,
      --  A parameter that's passed through as is (not canonicalized)

      Unlimited_Files,
      --  An unlimited number of whitespace separate file or directory
      --  parameters including wildcard specifications.

      Unlimited_As_Is,
      --  Un unlimited number of whitespace separated paameters that are
      --  passed through as is (not canonicalized).

      Files_Or_Wildcard);
      --  A comma separated list of files and/or wildcard file specifications.
      --  A comma preceded by or followed by whitespace is considered as a
      --  single comma character w/o whitespace.

   type Parameter_Array is array (Natural range <>) of Parameter_Type;
   type Parameter_Ref is access all Parameter_Array;

   type Command_Type is
     (Bind, Chop, Compile, Elim, Find, Krunch, Library, Link, List,
      Make, Name, Preprocess, Shared, Standard, Stub, Xref, Undefined);

   type Alternate_Command is (Comp, Ls, Kr, Prep, Psta);
   --  Alternate command libel for non VMS system

   Corresponding_To : constant array (Alternate_Command) of Command_Type :=
     (Comp  => Compile,
      Ls    => List,
      Kr    => Krunch,
      Prep  => Preprocess,
      Psta  => Standard);
   --  Mapping of alternate commands to commands

   subtype Real_Command_Type is Command_Type range Bind .. Xref;

   type Command_Entry is record
      Cname : String_Ptr;
      --  Command name for GNAT xxx command

      Usage : String_Ptr;
      --  A usage string, used for error messages

      Unixcmd : String_Ptr;
      --  Corresponding Unix command

      Unixsws : Argument_List_Access;
      --  Switches for the Unix command

      VMS_Only : Boolean;
      --  When True, the command can only be used on VMS

      Switches : Switches_Ptr;
      --  Pointer to array of switch strings

      Params : Parameter_Ref;
      --  Describes the allowable types of parameters.
      --  Params (1) is the type of the first parameter, etc.
      --  An empty parameter array means this command takes no parameters.

      Defext : String (1 .. 3);
      --  Default extension. If non-blank, then this extension is supplied by
      --  default as the extension for any file parameter which does not have
      --  an extension already.
   end record;

   -------------------------
   -- INTERNAL STRUCTURES --
   -------------------------

   --  The switches and commands are defined by strings in the previous
   --  section so that they are easy to modify, but internally, they are
   --  kept in a more conveniently accessible form described in this
   --  section.

   --  Commands, command qualifers and options have a similar common format
   --  so that searching for matching names can be done in a common manner.

   type Item_Id is (Id_Command, Id_Switch, Id_Option);

   type Translation_Type is
     (
      T_Direct,
      --  A qualifier with no options.
      --  Example: GNAT MAKE /VERBOSE

      T_Directories,
      --  A qualifier followed by a list of directories
      --  Example: GNAT COMPILE /SEARCH=([], [.FOO], [.BAR])

      T_Directory,
      --  A qualifier followed by one directory
      --  Example: GNAT LIBRARY /SET=[.VAXFLOATLIB]

      T_File,
      --  A qualifier followed by a filename
      --  Example: GNAT LINK /EXECUTABLE=FOO.EXE

      T_No_Space_File,
      --  A qualifier followed by a filename
      --  Example: GNAT MAKE /PROJECT_FILE=PRJ.GPR

      T_Numeric,
      --  A qualifier followed by a numeric value.
      --  Example: GNAT CHOP /FILE_NAME_MAX_LENGTH=39

      T_String,
      --  A qualifier followed by a quoted string. Only used by
      --  /IDENTIFICATION qualfier.
      --  Example: GNAT LINK /IDENTIFICATION="3.14a1 version"

      T_Options,
      --  A qualifier followed by a list of options.
      --  Example: GNAT COMPILE /REPRESENTATION_INFO=(ARRAYS,OBJECTS)

      T_Commands,
      --  A qualifier followed by a list. Only used for
      --  MAKE /COMPILER_QUALIFIERS /BINDER_QUALIFIERS /LINKER_QUALIFIERS
      --  (gnatmake -cargs -bargs -largs )
      --  Example: GNAT MAKE ... /LINKER_QUALIFIERS /VERBOSE FOOBAR.OBJ

      T_Other,
      --  A qualifier passed directly to the linker. Only used
      --  for LINK and SHARED if no other match is found.
      --  Example: GNAT LINK FOO.ALI /SYSSHR

      T_Alphanumplus
      --  A qualifier followed by a legal linker symbol prefix. Only used
      --  for BIND /BUILD_LIBRARY (gnatbind -Lxyz).
      --  Example: GNAT BIND /BUILD_LIBRARY=foobar
      );

   type Item (Id : Item_Id);
   type Item_Ptr is access all Item;

   type Item (Id : Item_Id) is record
      Name : String_Ptr;
      --  Name of the command, switch (with slash) or option

      Next : Item_Ptr;
      --  Pointer to next item on list, always has the same Id value

      Command : Command_Type := Undefined;

      Unix_String : String_Ptr := null;
      --  Corresponding Unix string. For a command, this is the unix command
      --  name and possible default switches. For a switch or option it is
      --  the unix switch string.

      case Id is

         when Id_Command =>

            Switches : Item_Ptr;
            --  Pointer to list of switch items for the command, linked
            --  through the Next fields with null terminating the list.

            Usage : String_Ptr;
            --  Usage information, used only for errors and the default
            --  list of commands output.

            Params : Parameter_Ref;
            --  Array of parameters

            Defext : String (1 .. 3);
            --  Default extension. If non-blank, then this extension is
            --  supplied by default as the extension for any file parameter
            --  which does not have an extension already.

         when Id_Switch =>

            Translation : Translation_Type;
            --  Type of switch translation. For all cases, except Options,
            --  this is the only field needed, since the Unix translation
            --  is found in Unix_String.

            Options : Item_Ptr;
            --  For the Options case, this field is set to point to a list
            --  of options item (for this case Unix_String is null in the
            --  main switch item). The end of the list is marked by null.

         when Id_Option =>

            null;
            --  No special fields needed, since Name and Unix_String are
            --  sufficient to completely described an option.

      end case;
   end record;

   subtype Command_Item is Item (Id_Command);
   subtype Switch_Item  is Item (Id_Switch);
   subtype Option_Item  is Item (Id_Option);

   ----------------------------------
   -- Declarations for GNATCMD use --
   ----------------------------------

   Commands : Item_Ptr;
   --  Pointer to head of list of command items, one for each command, with
   --  the end of the list marked by a null pointer.

   Last_Command : Item_Ptr;
   --  Pointer to last item in Commands list

   Normal_Exit : exception;
   --  Raise this exception for normal program termination

   Error_Exit : exception;
   --  Raise this exception if error detected

   Errors : Natural := 0;
   --  Count errors detected

   Command_Arg : Positive := 1;

   Command : Item_Ptr;
   --  Pointer to command item for current command

   Make_Commands_Active : Item_Ptr := null;
   --  Set to point to Command entry for COMPILE, BIND, or LINK as appropriate
   --  if a COMMANDS_TRANSLATION switch has been encountered while processing
   --  a MAKE Command.

   My_Exit_Status : Exit_Status := Success;

   package Buffer is new Table.Table
     (Table_Component_Type => Character,
      Table_Index_Type     => Integer,
      Table_Low_Bound      => 1,
      Table_Initial        => 4096,
      Table_Increment      => 2,
      Table_Name           => "Buffer");

   Param_Count : Natural := 0;
   --  Number of parameter arguments so far

   Arg_Num : Natural;
   --  Argument number

   Display_Command : Boolean := False;
   --  Set true if /? switch causes display of generated command (on VMS)

   The_Command : Command_Type;
   --  The command used

   -----------------------
   -- Local Subprograms --
   -----------------------

   function Index (Char : Character; Str : String) return Natural;
   --  Returns the first occurrence of Char in Str.
   --  Returns 0 if Char is not in Str.

   function Init_Object_Dirs return Argument_List;

   function Invert_Sense (S : String) return String_Ptr;
   --  Given a unix switch string S, computes the inverse (adding or
   --  removing ! characters as required), and returns a pointer to
   --  the allocated result on the heap.

   function Is_Extensionless (F : String) return Boolean;
   --  Returns true if the filename has no extension.

   function Match (S1, S2 : String) return Boolean;
   --  Determines whether S1 and S2 match. This is a case insensitive match.

   function Match_Prefix (S1, S2 : String) return Boolean;
   --  Determines whether S1 matches a prefix of S2. This is also a case
   --  insensitive match (for example Match ("AB","abc") is True).

   function Matching_Name
     (S     : String;
      Itm   : Item_Ptr;
      Quiet : Boolean := False)
      return  Item_Ptr;
   --  Determines if the item list headed by Itm and threaded through the
   --  Next fields (with null marking the end of the list), contains an
   --  entry that uniquely matches the given string. The match is case
   --  insensitive and permits unique abbreviation. If the match succeeds,
   --  then a pointer to the matching item is returned. Otherwise, an
   --  appropriate error message is written. Note that the discriminant
   --  of Itm is used to determine the appropriate form of this message.
   --  Quiet is normally False as shown, if it is set to True, then no
   --  error message is generated in a not found situation (null is still
   --  returned to indicate the not-found situation).

   procedure Non_VMS_Usage;
   --  Display usage for platforms other than VMS

   function OK_Alphanumerplus (S : String) return Boolean;
   --  Checks that S is a string of alphanumeric characters,
   --  returning True if all alphanumeric characters,
   --  False if empty or a non-alphanumeric character is present.

   function OK_Integer (S : String) return Boolean;
   --  Checks that S is a string of digits, returning True if all digits,
   --  False if empty or a non-digit is present.

   procedure Output_Version;
   --  Output the version of this program

   procedure Place (C : Character);
   --  Place a single character in the buffer, updating Ptr

   procedure Place (S : String);
   --  Place a string character in the buffer, updating Ptr

   procedure Place_Lower (S : String);
   --  Place string in buffer, forcing letters to lower case, updating Ptr

   procedure Place_Unix_Switches (S : String_Ptr);
   --  Given a unix switch string, place corresponding switches in Buffer,
   --  updating Ptr appropriatelly. Note that in the case of use of ! the
   --  result may be to remove a previously placed switch.

   procedure Set_Library_For
     (Project             : Project_Id;
      There_Are_Libraries : in out Boolean);
   --  If Project is a library project, add the correct
   --  -L and -l switches to the linker invocation.

   procedure Set_Libraries is
      new For_Every_Project_Imported (Boolean, Set_Library_For);
   --  Add the -L and -l switches to the linker for all
   --  of the library projects.

   procedure Validate_Command_Or_Option (N : String_Ptr);
   --  Check that N is a valid command or option name, i.e. that it is of the
   --  form of an Ada identifier with upper case letters and underscores.

   procedure Validate_Unix_Switch (S : String_Ptr);
   --  Check that S is a valid switch string as described in the syntax for
   --  the switch table item UNIX_SWITCH or else begins with a backquote.

   procedure VMS_Conversion (The_Command : out Command_Type);
   --  Converts VMS command line to equivalent Unix command line

   -----------
   -- Index --
   -----------

   function Index (Char : Character; Str : String) return Natural is
   begin
      for Index in Str'Range loop
         if Str (Index) = Char then
            return Index;
         end if;
      end loop;

      return 0;
   end Index;

   ----------------------
   -- Init_Object_Dirs --
   ----------------------

   function Init_Object_Dirs return Argument_List is
      Object_Dirs     : Integer;
      Object_Dir      : Argument_List (1 .. 256);
      Object_Dir_Name : String_Access;

   begin
      Object_Dirs := 0;
      Object_Dir_Name := String_Access (Object_Dir_Default_Name);
      Get_Next_Dir_In_Path_Init (Object_Dir_Name);

      loop
         declare
            Dir : String_Access := String_Access
              (Get_Next_Dir_In_Path (Object_Dir_Name));
         begin
            exit when Dir = null;
            Object_Dirs := Object_Dirs + 1;
            Object_Dir (Object_Dirs) :=
              new String'("-L" &
                          To_Canonical_Dir_Spec
                          (To_Host_Dir_Spec
                           (Normalize_Directory_Name (Dir.all).all,
                            True).all, True).all);
         end;
      end loop;

      Object_Dirs := Object_Dirs + 1;
      Object_Dir (Object_Dirs) := new String'("-lgnat");

      if Hostparm.OpenVMS then
         Object_Dirs := Object_Dirs + 1;
         Object_Dir (Object_Dirs) := new String'("-ldecgnat");
      end if;

      return Object_Dir (1 .. Object_Dirs);
   end Init_Object_Dirs;

   ------------------
   -- Invert_Sense --
   ------------------

   function Invert_Sense (S : String) return String_Ptr is
      Sinv : String (1 .. S'Length * 2);
      --  Result (for sure long enough)

      Sinvp : Natural := 0;
      --  Pointer to output string

   begin
      for Sp in S'Range loop
         if Sp = S'First or else S (Sp - 1) = ',' then
            if S (Sp) = '!' then
               null;
            else
               Sinv (Sinvp + 1) := '!';
               Sinv (Sinvp + 2) := S (Sp);
               Sinvp := Sinvp + 2;
            end if;

         else
            Sinv (Sinvp + 1) := S (Sp);
            Sinvp := Sinvp + 1;
         end if;
      end loop;

      return new String'(Sinv (1 .. Sinvp));
   end Invert_Sense;

   ----------------------
   -- Is_Extensionless --
   ----------------------

   function Is_Extensionless (F : String) return Boolean is
   begin
      for J in reverse F'Range loop
         if F (J) = '.' then
            return False;
         elsif F (J) = '/' or else F (J) = ']' or else F (J) = ':' then
            return True;
         end if;
      end loop;

      return True;
   end Is_Extensionless;

   -----------
   -- Match --
   -----------

   function Match (S1, S2 : String) return Boolean is
      Dif : constant Integer := S2'First - S1'First;

   begin

      if S1'Length /= S2'Length then
         return False;

      else
         for J in S1'Range loop
            if To_Lower (S1 (J)) /= To_Lower (S2 (J + Dif)) then
               return False;
            end if;
         end loop;

         return True;
      end if;
   end Match;

   ------------------
   -- Match_Prefix --
   ------------------

   function Match_Prefix (S1, S2 : String) return Boolean is
   begin
      if S1'Length > S2'Length then
         return False;
      else
         return Match (S1, S2 (S2'First .. S2'First + S1'Length - 1));
      end if;
   end Match_Prefix;

   -------------------
   -- Matching_Name --
   -------------------

   function Matching_Name
     (S     : String;
      Itm   : Item_Ptr;
      Quiet : Boolean := False)
     return  Item_Ptr
   is
      P1, P2 : Item_Ptr;

      procedure Err;
      --  Little procedure to output command/qualifier/option as appropriate
      --  and bump error count.

      ---------
      -- Err --
      ---------

      procedure Err is
      begin
         if Quiet then
            return;
         end if;

         Errors := Errors + 1;

         if Itm /= null then
            case Itm.Id is
               when Id_Command =>
                  Put (Standard_Error, "command");

               when Id_Switch =>
                  if OpenVMS then
                     Put (Standard_Error, "qualifier");
                  else
                     Put (Standard_Error, "switch");
                  end if;

               when Id_Option =>
                  Put (Standard_Error, "option");

            end case;
         else
            Put (Standard_Error, "input");

         end if;

         Put (Standard_Error, ": ");
         Put (Standard_Error, S);
      end Err;

   --  Start of processing for Matching_Name

   begin
      --  If exact match, that's the one we want

      P1 := Itm;
      while P1 /= null loop
         if Match (S, P1.Name.all) then
            return P1;
         else
            P1 := P1.Next;
         end if;
      end loop;

      --  Now check for prefix matches

      P1 := Itm;
      while P1 /= null loop
         if P1.Name.all = "/<other>" then
            return P1;

         elsif not Match_Prefix (S, P1.Name.all) then
            P1 := P1.Next;

         else
            --  Here we have found one matching prefix, so see if there is
            --  another one (which is an ambiguity)

            P2 := P1.Next;
            while P2 /= null loop
               if Match_Prefix (S, P2.Name.all) then
                  if not Quiet then
                     Put (Standard_Error, "ambiguous ");
                     Err;
                     Put (Standard_Error, " (matches ");
                     Put (Standard_Error, P1.Name.all);

                     while P2 /= null loop
                        if Match_Prefix (S, P2.Name.all) then
                           Put (Standard_Error, ',');
                           Put (Standard_Error, P2.Name.all);
                        end if;

                        P2 := P2.Next;
                     end loop;

                     Put_Line (Standard_Error, ")");
                  end if;

                  return null;
               end if;

               P2 := P2.Next;
            end loop;

            --  If we fall through that loop, then there was only one match

            return P1;
         end if;
      end loop;

      --  If we fall through outer loop, there was no match

      if not Quiet then
         Put (Standard_Error, "unrecognized ");
         Err;
         New_Line (Standard_Error);
      end if;

      return null;
   end Matching_Name;

   -----------------------
   -- OK_Alphanumerplus --
   -----------------------

   function OK_Alphanumerplus (S : String) return Boolean is
   begin
      if S'Length = 0 then
         return False;

      else
         for J in S'Range loop
            if not (Is_Alphanumeric (S (J)) or else
                    S (J) = '_' or else S (J) = '$')
            then
               return False;
            end if;
         end loop;

         return True;
      end if;
   end OK_Alphanumerplus;

   ----------------
   -- OK_Integer --
   ----------------

   function OK_Integer (S : String) return Boolean is
   begin
      if S'Length = 0 then
         return False;

      else
         for J in S'Range loop
            if not Is_Digit (S (J)) then
               return False;
            end if;
         end loop;

         return True;
      end if;
   end OK_Integer;

   --------------------
   -- Output_Version --
   --------------------

   procedure Output_Version is
   begin
      Put ("GNAT ");
      Put (Gnatvsn.Gnat_Version_String);
      Put_Line (" Copyright 1996-2002 Free Software Foundation, Inc.");
   end Output_Version;

   -----------
   -- Place --
   -----------

   procedure Place (C : Character) is
   begin
      Buffer.Increment_Last;
      Buffer.Table (Buffer.Last) := C;

      --  Do not put a space as the first character in the buffer
      if C = ' ' and then Buffer.Last = 1 then
         Buffer.Decrement_Last;
      end if;
   end Place;

   procedure Place (S : String) is
   begin
      for J in S'Range loop
         Place (S (J));
      end loop;
   end Place;

   -----------------
   -- Place_Lower --
   -----------------

   procedure Place_Lower (S : String) is
   begin
      for J in S'Range loop
         Place (To_Lower (S (J)));
      end loop;
   end Place_Lower;

   -------------------------
   -- Place_Unix_Switches --
   -------------------------

   procedure Place_Unix_Switches (S : String_Ptr) is
      P1, P2, P3 : Natural;
      Remove     : Boolean;
      Slen       : Natural;

   begin
      P1 := S'First;
      while P1 <= S'Last loop
         if S (P1) = '!' then
            P1 := P1 + 1;
            Remove := True;
         else
            Remove := False;
         end if;

         P2 := P1;
         pragma Assert (S (P1) = '-' or else S (P1) = '`');

         while P2 < S'Last and then S (P2 + 1) /= ',' loop
            P2 := P2 + 1;
         end loop;

         --  Switch is now in S (P1 .. P2)

         Slen := P2 - P1 + 1;

         if Remove then
            P3 := 2;
            while P3 <= Buffer.Last - Slen loop
               if Buffer.Table (P3) = ' '
                 and then String (Buffer.Table (P3 + 1 .. P3 + Slen)) =
                                                             S (P1 .. P2)
                 and then (P3 + Slen = Buffer.Last
                             or else
                           Buffer.Table (P3 + Slen + 1) = ' ')
               then
                  Buffer.Table (P3 .. Buffer.Last - Slen - 1) :=
                    Buffer.Table (P3 + Slen + 1 .. Buffer.Last);
                  Buffer.Set_Last (Buffer.Last - Slen - 1);

               else
                  P3 := P3 + 1;
               end if;
            end loop;

         else
            Place (' ');

            if S (P1) = '`' then
               P1 := P1 + 1;
            end if;

            Place (S (P1 .. P2));
         end if;

         P1 := P2 + 2;
      end loop;
   end Place_Unix_Switches;

   ---------------------
   -- Set_Library_For --
   ---------------------

   procedure Set_Library_For
     (Project             : Project_Id;
      There_Are_Libraries : in out Boolean)
   is
   begin
      --  Case of library project

      if Projects.Table (Project).Library then
         There_Are_Libraries := True;

         --  Add the -L switch

         Last_Switches.Increment_Last;
         Last_Switches.Table (Last_Switches.Last) :=
           new String'("-L" &
                       Get_Name_String
                       (Projects.Table (Project).Library_Dir));

         --  Add the -l switch

         Last_Switches.Increment_Last;
         Last_Switches.Table (Last_Switches.Last) :=
           new String'("-l" &
                       Get_Name_String
                       (Projects.Table (Project).Library_Name));

         --  Add the Wl,-rpath switch if library non static

         if Projects.Table (Project).Library_Kind /= Static then
            declare
               Option : constant String_Access :=
                          MLib.Tgt.Linker_Library_Path_Option
                            (Get_Name_String
                              (Projects.Table (Project).Library_Dir));

            begin
               if Option /= null then
                  Last_Switches.Increment_Last;
                  Last_Switches.Table (Last_Switches.Last) :=
                    Option;
               end if;

            end;

         end if;

      end if;
   end Set_Library_For;

   --------------------------------
   -- Validate_Command_Or_Option --
   --------------------------------

   procedure Validate_Command_Or_Option (N : String_Ptr) is
   begin
      pragma Assert (N'Length > 0);

      for J in N'Range loop
         if N (J) = '_' then
            pragma Assert (N (J - 1) /= '_');
            null;
         else
            pragma Assert (Is_Upper (N (J)) or else Is_Digit (N (J)));
            null;
         end if;
      end loop;
   end Validate_Command_Or_Option;

   --------------------------
   -- Validate_Unix_Switch --
   --------------------------

   procedure Validate_Unix_Switch (S : String_Ptr) is
   begin
      if S (S'First) = '`' then
         return;
      end if;

      pragma Assert (S (S'First) = '-' or else S (S'First) = '!');

      for J in S'First + 1 .. S'Last loop
         pragma Assert (S (J) /= ' ');

         if S (J) = '!' then
            pragma Assert (S (J - 1) = ',' and then S (J + 1) = '-');
            null;
         end if;
      end loop;
   end Validate_Unix_Switch;

   ----------------------
   -- List of Commands --
   ----------------------

   --  Note that we put this after all the local bodies (except Non_VMS_Usage
   --  and VMS_Conversion that use Command_List) to avoid some access before
   --  elaboration problems.

   Command_List : constant array (Real_Command_Type) of Command_Entry :=
     (Bind =>
       (Cname    => new S'("BIND"),
        Usage    => new S'("GNAT BIND file[.ali] /qualifiers"),
        VMS_Only => False,
        Unixcmd  => new S'("gnatbind"),
        Unixsws  => null,
        Switches => Bind_Switches'Access,
        Params   => new Parameter_Array'(1 => File),
        Defext   => "ali"),

      Chop =>
        (Cname    => new S'("CHOP"),
         Usage    => new S'("GNAT CHOP file [directory] /qualifiers"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatchop"),
         Unixsws  => null,
         Switches => Chop_Switches'Access,
         Params   => new Parameter_Array'(1 => File, 2 => Optional_File),
         Defext   => "   "),

      Compile =>
        (Cname    => new S'("COMPILE"),
         Usage    => new S'("GNAT COMPILE filespec[,...] /qualifiers"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatmake"),
         Unixsws  => new Argument_List' (1 => new String'("-f"),
                                         2 => new String'("-u"),
                                         3 => new String'("-c")),
         Switches => GCC_Switches'Access,
         Params   => new Parameter_Array'(1 => Files_Or_Wildcard),
         Defext   => "   "),

      Elim =>
        (Cname    => new S'("ELIM"),
         Usage    => new S'("GNAT ELIM name /qualifiers"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatelim"),
         Unixsws  => null,
         Switches => Elim_Switches'Access,
         Params   => new Parameter_Array'(1 => Other_As_Is),
         Defext   => "ali"),

      Find =>
        (Cname    => new S'("FIND"),
         Usage    => new S'("GNAT FIND pattern[:sourcefile[:line"
                            & "[:column]]] filespec[,...] /qualifiers"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatfind"),
         Unixsws  => null,
         Switches => Find_Switches'Access,
         Params   => new Parameter_Array'(1 => Other_As_Is,
                                          2 => Files_Or_Wildcard),
         Defext   => "ali"),

      Krunch =>
        (Cname    => new S'("KRUNCH"),
         Usage    => new S'("GNAT KRUNCH file [/COUNT=nnn]"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatkr"),
         Unixsws  => null,
         Switches => Krunch_Switches'Access,
         Params   => new Parameter_Array'(1 => File),
         Defext   => "   "),

      Library =>
        (Cname    => new S'("LIBRARY"),
         Usage    => new S'("GNAT LIBRARY /[CREATE | SET | DELETE]"
                            & "=directory [/CONFIG=file]"),
         VMS_Only => True,
         Unixcmd  => new S'("gnatlbr"),
         Unixsws  => null,
         Switches => Lbr_Switches'Access,
         Params   => new Parameter_Array'(1 .. 0 => File),
         Defext   => "   "),

      Link =>
        (Cname    => new S'("LINK"),
         Usage    => new S'("GNAT LINK file[.ali]"
                            & " [extra obj_&_lib_&_exe_&_opt files]"
                            & " /qualifiers"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatlink"),
         Unixsws  => null,
         Switches => Link_Switches'Access,
         Params   => new Parameter_Array'(1 => Unlimited_Files),
         Defext   => "ali"),

      List =>
        (Cname    => new S'("LIST"),
         Usage    => new S'("GNAT LIST /qualifiers object_or_ali_file"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatls"),
         Unixsws  => null,
         Switches => List_Switches'Access,
         Params   => new Parameter_Array'(1 => File),
         Defext   => "ali"),

      Make =>
        (Cname    => new S'("MAKE"),
         Usage    => new S'("GNAT MAKE file /qualifiers (includes "
                            & "COMPILE /qualifiers)"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatmake"),
         Unixsws  => null,
         Switches => Make_Switches'Access,
         Params   => new Parameter_Array'(1 => File),
         Defext   => "   "),

      Name =>
        (Cname    => new S'("NAME"),
         Usage    => new S'("GNAT NAME /qualifiers naming-pattern "
                            & "[naming-patterns]"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatname"),
         Unixsws  => null,
         Switches => Name_Switches'Access,
         Params   => new Parameter_Array'(1 => Unlimited_As_Is),
         Defext   => "   "),

      Preprocess =>
        (Cname    => new S'("PREPROCESS"),
         Usage    => new S'("GNAT PREPROCESS ifile ofile dfile /qualifiers"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatprep"),
         Unixsws  => null,
         Switches => Prep_Switches'Access,
         Params   => new Parameter_Array'(1 .. 3 => File),
         Defext   => "   "),

      Shared =>
        (Cname    => new S'("SHARED"),
         Usage    => new S'("GNAT SHARED [obj_&_lib_&_exe_&_opt"
                            & "files] /qualifiers"),
         VMS_Only => True,
         Unixcmd  => new S'("gcc"),
         Unixsws  => new Argument_List'(new String'("-shared")
                                        & Init_Object_Dirs),
         Switches => Shared_Switches'Access,
         Params   => new Parameter_Array'(1 => Unlimited_Files),
         Defext   => "   "),

      Standard =>
        (Cname    => new S'("STANDARD"),
         Usage    => new S'("GNAT STANDARD"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatpsta"),
         Unixsws  => null,
         Switches => Standard_Switches'Access,
         Params   => new Parameter_Array'(1 .. 0 => File),
         Defext   => "   "),

      Stub =>
        (Cname    => new S'("STUB"),
         Usage    => new S'("GNAT STUB file [directory]/qualifiers"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatstub"),
         Unixsws  => null,
         Switches => Stub_Switches'Access,
         Params   => new Parameter_Array'(1 => File, 2 => Optional_File),
         Defext   => "   "),

      Xref =>
        (Cname    => new S'("XREF"),
         Usage    => new S'("GNAT XREF filespec[,...] /qualifiers"),
         VMS_Only => False,
         Unixcmd  => new S'("gnatxref"),
         Unixsws  => null,
         Switches => Xref_Switches'Access,
         Params   => new Parameter_Array'(1 => Files_Or_Wildcard),
         Defext   => "ali")
      );

   -------------------
   -- Non_VMS_Usage --
   -------------------

   procedure Non_VMS_Usage is
   begin
      Output_Version;
      New_Line;
      Put_Line ("List of available commands");
      New_Line;

      for C in Command_List'Range loop
         if not Command_List (C).VMS_Only then
            Put ("GNAT " & Command_List (C).Cname.all);
            Set_Col (25);
            Put (Command_List (C).Unixcmd.all);

            declare
               Sws : Argument_List_Access renames Command_List (C).Unixsws;
            begin
               if Sws /= null then
                  for J in Sws'Range loop
                     Put (' ');
                     Put (Sws (J).all);
                  end loop;
               end if;
            end;

            New_Line;
         end if;
      end loop;

      New_Line;
      Put_Line ("Commands FIND, LIST and XREF accept project file " &
                "switches -vPx, -Pprj and -Xnam=val");
      New_Line;
   end Non_VMS_Usage;

   --------------------
   -- VMS_Conversion --
   --------------------

   procedure VMS_Conversion (The_Command : out Command_Type) is
   begin
      Buffer.Init;

      --  First we must preprocess the string form of the command and options
      --  list into the internal form that we use.

      for C in Real_Command_Type loop

         declare
            Command : Item_Ptr := new Command_Item;

            Last_Switch : Item_Ptr;
            --  Last switch in list

         begin
            --  Link new command item into list of commands

            if Last_Command = null then
               Commands := Command;
            else
               Last_Command.Next := Command;
            end if;

            Last_Command := Command;

            --  Fill in fields of new command item

            Command.Name    := Command_List (C).Cname;
            Command.Usage   := Command_List (C).Usage;
            Command.Command := C;

            if Command_List (C).Unixsws = null then
               Command.Unix_String := Command_List (C).Unixcmd;
            else
               declare
                  Cmd  : String (1 .. 5_000);
                  Last : Natural := 0;
                  Sws  : Argument_List_Access := Command_List (C).Unixsws;

               begin
                  Cmd (1 .. Command_List (C).Unixcmd'Length) :=
                    Command_List (C).Unixcmd.all;
                  Last := Command_List (C).Unixcmd'Length;

                  for J in Sws'Range loop
                     Last := Last + 1;
                     Cmd (Last) := ' ';
                     Cmd (Last + 1 .. Last + Sws (J)'Length) :=
                       Sws (J).all;
                     Last := Last + Sws (J)'Length;
                  end loop;

                  Command.Unix_String := new String'(Cmd (1 .. Last));
               end;
            end if;

            Command.Params := Command_List (C).Params;
            Command.Defext := Command_List (C).Defext;

            Validate_Command_Or_Option (Command.Name);

            --  Process the switch list

            for S in Command_List (C).Switches'Range loop
               declare
                  SS : constant String_Ptr := Command_List (C).Switches (S);

                  P  : Natural := SS'First;
                  Sw : Item_Ptr := new Switch_Item;

                  Last_Opt : Item_Ptr;
                  --  Pointer to last option

               begin
                  --  Link new switch item into list of switches

                  if Last_Switch = null then
                     Command.Switches := Sw;
                  else
                     Last_Switch.Next := Sw;
                  end if;

                  Last_Switch := Sw;

                  --  Process switch string, first get name

                  while SS (P) /= ' ' and SS (P) /= '=' loop
                     P := P + 1;
                  end loop;

                  Sw.Name := new String'(SS (SS'First .. P - 1));

                  --  Direct translation case

                  if SS (P) = ' ' then
                     Sw.Translation := T_Direct;
                     Sw.Unix_String := new String'(SS (P + 1 .. SS'Last));
                     Validate_Unix_Switch (Sw.Unix_String);

                     if SS (P - 1) = '>' then
                        Sw.Translation := T_Other;

                     elsif SS (P + 1) = '`' then
                        null;

                        --  Create the inverted case (/NO ..)

                     elsif SS (SS'First + 1 .. SS'First + 2) /= "NO" then
                        Sw := new Switch_Item;
                        Last_Switch.Next := Sw;
                        Last_Switch := Sw;

                        Sw.Name :=
                          new String'("/NO" & SS (SS'First + 1 .. P - 1));
                        Sw.Translation := T_Direct;
                        Sw.Unix_String := Invert_Sense (SS (P + 1 .. SS'Last));
                        Validate_Unix_Switch (Sw.Unix_String);
                     end if;

                     --  Directories translation case

                  elsif SS (P + 1) = '*' then
                     pragma Assert (SS (SS'Last) = '*');
                     Sw.Translation := T_Directories;
                     Sw.Unix_String := new String'(SS (P + 2 .. SS'Last - 1));
                     Validate_Unix_Switch (Sw.Unix_String);

                     --  Directory translation case

                  elsif SS (P + 1) = '%' then
                     pragma Assert (SS (SS'Last) = '%');
                     Sw.Translation := T_Directory;
                     Sw.Unix_String := new String'(SS (P + 2 .. SS'Last - 1));
                     Validate_Unix_Switch (Sw.Unix_String);

                     --  File translation case

                  elsif SS (P + 1) = '@' then
                     pragma Assert (SS (SS'Last) = '@');
                     Sw.Translation := T_File;
                     Sw.Unix_String := new String'(SS (P + 2 .. SS'Last - 1));
                     Validate_Unix_Switch (Sw.Unix_String);

                     --  No space file translation case

                  elsif SS (P + 1) = '<' then
                     pragma Assert (SS (SS'Last) = '>');
                     Sw.Translation := T_No_Space_File;
                     Sw.Unix_String := new String'(SS (P + 2 .. SS'Last - 1));
                     Validate_Unix_Switch (Sw.Unix_String);

                     --  Numeric translation case

                  elsif SS (P + 1) = '#' then
                     pragma Assert (SS (SS'Last) = '#');
                     Sw.Translation := T_Numeric;
                     Sw.Unix_String := new String'(SS (P + 2 .. SS'Last - 1));
                     Validate_Unix_Switch (Sw.Unix_String);

                     --  Alphanumerplus translation case

                  elsif SS (P + 1) = '|' then
                     pragma Assert (SS (SS'Last) = '|');
                     Sw.Translation := T_Alphanumplus;
                     Sw.Unix_String := new String'(SS (P + 2 .. SS'Last - 1));
                     Validate_Unix_Switch (Sw.Unix_String);

                     --  String translation case

                  elsif SS (P + 1) = '"' then
                     pragma Assert (SS (SS'Last) = '"');
                     Sw.Translation := T_String;
                     Sw.Unix_String := new String'(SS (P + 2 .. SS'Last - 1));
                     Validate_Unix_Switch (Sw.Unix_String);

                     --  Commands translation case

                  elsif SS (P + 1) = '?' then
                     Sw.Translation := T_Commands;
                     Sw.Unix_String := new String'(SS (P + 2 .. SS'Last));

                     --  Options translation case

                  else
                     Sw.Translation := T_Options;
                     Sw.Unix_String := new String'("");

                     P := P + 1; -- bump past =
                     while P <= SS'Last loop
                        declare
                           Opt : Item_Ptr := new Option_Item;
                           Q   : Natural;

                        begin
                           --  Link new option item into options list

                           if Last_Opt = null then
                              Sw.Options := Opt;
                           else
                              Last_Opt.Next := Opt;
                           end if;

                           Last_Opt := Opt;

                           --  Fill in fields of new option item

                           Q := P;
                           while SS (Q) /= ' ' loop
                              Q := Q + 1;
                           end loop;

                           Opt.Name := new String'(SS (P .. Q - 1));
                           Validate_Command_Or_Option (Opt.Name);

                           P := Q + 1;
                           Q := P;

                           while Q <= SS'Last and then SS (Q) /= ' ' loop
                              Q := Q + 1;
                           end loop;

                           Opt.Unix_String := new String'(SS (P .. Q - 1));
                           Validate_Unix_Switch (Opt.Unix_String);
                           P := Q + 1;
                        end;
                     end loop;
                  end if;
               end;
            end loop;
         end;
      end loop;

      --  If no parameters, give complete list of commands

      if Argument_Count = 0 then
         Output_Version;
         New_Line;
         Put_Line ("List of available commands");
         New_Line;

         while Commands /= null loop
            Put (Commands.Usage.all);
            Set_Col (53);
            Put_Line (Commands.Unix_String.all);
            Commands := Commands.Next;
         end loop;

         raise Normal_Exit;
      end if;

      Arg_Num := 1;

      --  Loop through arguments

      while Arg_Num <= Argument_Count loop

         Process_Argument : declare
            Argv    : String_Access;
            Arg_Idx : Integer;

            function Get_Arg_End
              (Argv    : String;
               Arg_Idx : Integer)
               return    Integer;
            --  Begins looking at Arg_Idx + 1 and returns the index of the
            --  last character before a slash or else the index of the last
            --  character in the string Argv.

            -----------------
            -- Get_Arg_End --
            -----------------

            function Get_Arg_End
              (Argv    : String;
               Arg_Idx : Integer)
              return    Integer
            is
            begin
               for J in Arg_Idx + 1 .. Argv'Last loop
                  if Argv (J) = '/' then
                     return J - 1;
                  end if;
               end loop;

               return Argv'Last;
            end Get_Arg_End;

         --  Start of processing for Process_Argument

         begin
            Argv := new String'(Argument (Arg_Num));
            Arg_Idx := Argv'First;

            <<Tryagain_After_Coalesce>>
               loop
                  declare
                     Next_Arg_Idx : Integer;
                     Arg          : String_Access;

                  begin
                     Next_Arg_Idx := Get_Arg_End (Argv.all, Arg_Idx);
                     Arg := new String'(Argv (Arg_Idx .. Next_Arg_Idx));

                     --  The first one must be a command name

                     if Arg_Num = 1 and then Arg_Idx = Argv'First then

                        Command := Matching_Name (Arg.all, Commands);

                        if Command = null then
                           raise Error_Exit;
                        end if;

                        The_Command := Command.Command;

                        --  Give usage information if only command given

                        if Argument_Count = 1 and then Next_Arg_Idx = Argv'Last
                          and then Command.Command /= Standard
                        then
                           Output_Version;
                           New_Line;
                           Put_Line
                             ("List of available qualifiers and options");
                           New_Line;

                           Put (Command.Usage.all);
                           Set_Col (53);
                           Put_Line (Command.Unix_String.all);

                           declare
                              Sw : Item_Ptr := Command.Switches;

                           begin
                              while Sw /= null loop
                                 Put ("   ");
                                 Put (Sw.Name.all);

                                 case Sw.Translation is

                                    when T_Other =>
                                       Set_Col (53);
                                       Put_Line (Sw.Unix_String.all &
                                                 "/<other>");

                                    when T_Direct =>
                                       Set_Col (53);
                                       Put_Line (Sw.Unix_String.all);

                                    when T_Directories =>
                                       Put ("=(direc,direc,..direc)");
                                       Set_Col (53);
                                       Put (Sw.Unix_String.all);
                                       Put (" direc ");
                                       Put (Sw.Unix_String.all);
                                       Put_Line (" direc ...");

                                    when T_Directory =>
                                       Put ("=directory");
                                       Set_Col (53);
                                       Put (Sw.Unix_String.all);

                                       if Sw.Unix_String (Sw.Unix_String'Last)
                                         /= '='
                                       then
                                          Put (' ');
                                       end if;

                                       Put_Line ("directory ");

                                    when T_File | T_No_Space_File =>
                                       Put ("=file");
                                       Set_Col (53);
                                       Put (Sw.Unix_String.all);

                                       if Sw.Translation = T_File
                                         and then Sw.Unix_String
                                                   (Sw.Unix_String'Last)
                                                     /= '='
                                       then
                                          Put (' ');
                                       end if;

                                       Put_Line ("file ");

                                    when T_Numeric =>
                                       Put ("=nnn");
                                       Set_Col (53);

                                       if Sw.Unix_String (Sw.Unix_String'First)
                                         = '`'
                                       then
                                          Put (Sw.Unix_String
                                               (Sw.Unix_String'First + 1
                                                .. Sw.Unix_String'Last));
                                       else
                                          Put (Sw.Unix_String.all);
                                       end if;

                                       Put_Line ("nnn");

                                    when T_Alphanumplus =>
                                       Put ("=xyz");
                                       Set_Col (53);

                                       if Sw.Unix_String (Sw.Unix_String'First)
                                         = '`'
                                       then
                                          Put (Sw.Unix_String
                                               (Sw.Unix_String'First + 1
                                                .. Sw.Unix_String'Last));
                                       else
                                          Put (Sw.Unix_String.all);
                                       end if;

                                       Put_Line ("xyz");

                                    when T_String =>
                                       Put ("=");
                                       Put ('"');
                                       Put ("<string>");
                                       Put ('"');
                                       Set_Col (53);

                                       Put (Sw.Unix_String.all);

                                       if Sw.Unix_String (Sw.Unix_String'Last)
                                         /= '='
                                       then
                                          Put (' ');
                                       end if;

                                       Put ("<string>");
                                       New_Line;

                                    when T_Commands =>
                                       Put (" (switches for ");
                                       Put (Sw.Unix_String
                                            (Sw.Unix_String'First + 7
                                             .. Sw.Unix_String'Last));
                                       Put (')');
                                       Set_Col (53);
                                       Put (Sw.Unix_String
                                            (Sw.Unix_String'First
                                             .. Sw.Unix_String'First + 5));
                                       Put_Line (" switches");

                                    when T_Options =>
                                       declare
                                          Opt : Item_Ptr := Sw.Options;

                                       begin
                                          Put_Line ("=(option,option..)");

                                          while Opt /= null loop
                                             Put ("      ");
                                             Put (Opt.Name.all);

                                             if Opt = Sw.Options then
                                                Put (" (D)");
                                             end if;

                                             Set_Col (53);
                                             Put_Line (Opt.Unix_String.all);
                                             Opt := Opt.Next;
                                          end loop;
                                       end;

                                 end case;

                                 Sw := Sw.Next;
                              end loop;
                           end;

                           raise Normal_Exit;
                        end if;

                        --  Place (Command.Unix_String.all);

                        --  Special handling for internal debugging switch /?

                     elsif Arg.all = "/?" then
                        Display_Command := True;

                        --  Copy -switch unchanged

                     elsif Arg (Arg'First) = '-' then
                        Place (' ');
                        Place (Arg.all);

                        --  Copy quoted switch with quotes stripped

                     elsif Arg (Arg'First) = '"' then
                        if Arg (Arg'Last) /= '"' then
                           Put (Standard_Error, "misquoted argument: ");
                           Put_Line (Standard_Error, Arg.all);
                           Errors := Errors + 1;

                        else
                           Place (' ');
                           Place (Arg (Arg'First + 1 .. Arg'Last - 1));
                        end if;

                        --  Parameter Argument

                     elsif Arg (Arg'First) /= '/'
                       and then Make_Commands_Active = null
                     then
                        Param_Count := Param_Count + 1;

                        if Param_Count <= Command.Params'Length then

                           case Command.Params (Param_Count) is

                              when File | Optional_File =>
                                 declare
                                    Normal_File : String_Access
                                      := To_Canonical_File_Spec (Arg.all);
                                 begin
                                    Place (' ');
                                    Place_Lower (Normal_File.all);

                                    if Is_Extensionless (Normal_File.all)
                                      and then Command.Defext /= "   "
                                    then
                                       Place ('.');
                                       Place (Command.Defext);
                                    end if;
                                 end;

                              when Unlimited_Files =>
                                 declare
                                    Normal_File : String_Access
                                      := To_Canonical_File_Spec (Arg.all);

                                    File_Is_Wild  : Boolean := False;
                                    File_List     : String_Access_List_Access;
                                 begin
                                    for I in Arg'Range loop
                                       if Arg (I) = '*'
                                         or else Arg (I) = '%'
                                       then
                                          File_Is_Wild := True;
                                       end if;
                                    end loop;

                                    if File_Is_Wild then
                                       File_List := To_Canonical_File_List
                                         (Arg.all, False);

                                       for I in File_List.all'Range loop
                                          Place (' ');
                                          Place_Lower (File_List.all (I).all);
                                       end loop;
                                    else
                                       Place (' ');
                                       Place_Lower (Normal_File.all);

                                       if Is_Extensionless (Normal_File.all)
                                         and then Command.Defext /= "   "
                                       then
                                          Place ('.');
                                          Place (Command.Defext);
                                       end if;
                                    end if;

                                    Param_Count := Param_Count - 1;
                                 end;

                              when Other_As_Is =>
                                 Place (' ');
                                 Place (Arg.all);

                              when Unlimited_As_Is =>
                                 Place (' ');
                                 Place (Arg.all);
                                 Param_Count := Param_Count - 1;

                              when Files_Or_Wildcard =>

                                 --  Remove spaces from a comma separated list
                                 --  of file names and adjust control variables
                                 --  accordingly.

                                 while Arg_Num < Argument_Count and then
                                   (Argv (Argv'Last) = ',' xor
                                    Argument (Arg_Num + 1)
                                      (Argument (Arg_Num + 1)'First) = ',')
                                 loop
                                    Argv := new String'
                                           (Argv.all & Argument (Arg_Num + 1));
                                    Arg_Num := Arg_Num + 1;
                                    Arg_Idx := Argv'First;
                                    Next_Arg_Idx :=
                                      Get_Arg_End (Argv.all, Arg_Idx);
                                    Arg := new String'
                                            (Argv (Arg_Idx .. Next_Arg_Idx));
                                 end loop;

                                 --  Parse the comma separated list of VMS
                                 --  filenames and place them on the command
                                 --  line as space separated Unix style
                                 --  filenames. Lower case and add default
                                 --  extension as appropriate.

                                 declare
                                    Arg1_Idx : Integer := Arg'First;

                                    function Get_Arg1_End
                                      (Arg : String; Arg_Idx : Integer)
                                      return Integer;
                                    --  Begins looking at Arg_Idx + 1 and
                                    --  returns the index of the last character
                                    --  before a comma or else the index of the
                                    --  last character in the string Arg.

                                    function Get_Arg1_End
                                      (Arg : String; Arg_Idx : Integer)
                                      return Integer
                                    is
                                    begin
                                       for I in Arg_Idx + 1 .. Arg'Last loop
                                          if Arg (I) = ',' then
                                             return I - 1;
                                          end if;
                                       end loop;

                                       return Arg'Last;
                                    end Get_Arg1_End;

                                 begin
                                    loop
                                       declare
                                          Next_Arg1_Idx : Integer :=
                                            Get_Arg1_End (Arg.all, Arg1_Idx);

                                          Arg1 : String :=
                                            Arg (Arg1_Idx .. Next_Arg1_Idx);

                                          Normal_File : String_Access :=
                                            To_Canonical_File_Spec (Arg1);

                                       begin
                                          Place (' ');
                                          Place_Lower (Normal_File.all);

                                          if Is_Extensionless (Normal_File.all)
                                            and then Command.Defext /= "   "
                                          then
                                             Place ('.');
                                             Place (Command.Defext);
                                          end if;

                                          Arg1_Idx := Next_Arg1_Idx + 1;
                                       end;

                                       exit when Arg1_Idx > Arg'Last;

                                       --  Don't allow two or more commas in
                                       --  a row

                                       if Arg (Arg1_Idx) = ',' then
                                          Arg1_Idx := Arg1_Idx + 1;
                                          if Arg1_Idx > Arg'Last or else
                                            Arg (Arg1_Idx) = ','
                                          then
                                             Put_Line
                                               (Standard_Error,
                                                "Malformed Parameter: " &
                                                Arg.all);
                                             Put (Standard_Error, "usage: ");
                                             Put_Line (Standard_Error,
                                                       Command.Usage.all);
                                             raise Error_Exit;
                                          end if;
                                       end if;

                                    end loop;
                                 end;
                           end case;
                        end if;

                        --  Qualifier argument

                     else
                        declare
                           Sw   : Item_Ptr;
                           SwP  : Natural;
                           P2   : Natural;
                           Endp : Natural := 0; -- avoid warning!
                           Opt  : Item_Ptr;

                        begin
                           SwP := Arg'First;
                           while SwP < Arg'Last
                             and then Arg (SwP + 1) /= '='
                           loop
                              SwP := SwP + 1;
                           end loop;

                           --  At this point, the switch name is in
                           --  Arg (Arg'First..SwP) and if that is not the
                           --  whole switch, then there is an equal sign at
                           --  Arg (SwP + 1) and the rest of Arg is what comes
                           --  after the equal sign.

                           --  If make commands are active, see if we have
                           --  another COMMANDS_TRANSLATION switch belonging
                           --  to gnatmake.

                           if Make_Commands_Active /= null then
                              Sw :=
                                Matching_Name
                                (Arg (Arg'First .. SwP),
                                 Command.Switches,
                                 Quiet => True);

                              if Sw /= null
                                and then Sw.Translation = T_Commands
                              then
                                 null;

                              else
                                 Sw :=
                                   Matching_Name
                                   (Arg (Arg'First .. SwP),
                                    Make_Commands_Active.Switches,
                                    Quiet => False);
                              end if;

                              --  For case of GNAT MAKE or CHOP, if we cannot
                              --  find the switch, then see if it is a
                              --  recognized compiler switch instead, and if
                              --  so process the compiler switch.

                           elsif Command.Name.all = "MAKE"
                             or else Command.Name.all = "CHOP" then
                              Sw :=
                                Matching_Name
                                (Arg (Arg'First .. SwP),
                                 Command.Switches,
                                 Quiet => True);

                              if Sw = null then
                                 Sw :=
                                   Matching_Name
                                   (Arg (Arg'First .. SwP),
                                    Matching_Name
                                      ("COMPILE", Commands).Switches,
                                    Quiet => False);
                              end if;

                              --  For all other cases, just search the relevant
                              --  command.

                           else
                              Sw :=
                                Matching_Name
                                (Arg (Arg'First .. SwP),
                                 Command.Switches,
                                 Quiet => False);
                           end if;

                           if Sw /= null then
                              case Sw.Translation is

                                 when T_Direct =>
                                    Place_Unix_Switches (Sw.Unix_String);
                                    if SwP < Arg'Last
                                      and then Arg (SwP + 1) = '='
                                    then
                                       Put (Standard_Error,
                                            "qualifier options ignored: ");
                                       Put_Line (Standard_Error, Arg.all);
                                    end if;

                                 when T_Directories =>
                                    if SwP + 1 > Arg'Last then
                                       Put (Standard_Error,
                                            "missing directories for: ");
                                       Put_Line (Standard_Error, Arg.all);
                                       Errors := Errors + 1;

                                    elsif Arg (SwP + 2) /= '(' then
                                       SwP := SwP + 2;
                                       Endp := Arg'Last;

                                    elsif Arg (Arg'Last) /= ')' then

                                       --  Remove spaces from a comma separated
                                       --  list of file names and adjust
                                       --  control variables accordingly.

                                       if Arg_Num < Argument_Count and then
                                         (Argv (Argv'Last) = ',' xor
                                          Argument (Arg_Num + 1)
                                          (Argument (Arg_Num + 1)'First) = ',')
                                       then
                                          Argv :=
                                            new String'(Argv.all
                                                        & Argument
                                                           (Arg_Num + 1));
                                          Arg_Num := Arg_Num + 1;
                                          Arg_Idx := Argv'First;
                                          Next_Arg_Idx
                                            := Get_Arg_End (Argv.all, Arg_Idx);
                                          Arg := new String'
                                            (Argv (Arg_Idx .. Next_Arg_Idx));
                                          goto Tryagain_After_Coalesce;
                                       end if;

                                       Put (Standard_Error,
                                            "incorrectly parenthesized " &
                                            "or malformed argument: ");
                                       Put_Line (Standard_Error, Arg.all);
                                       Errors := Errors + 1;

                                    else
                                       SwP := SwP + 3;
                                       Endp := Arg'Last - 1;
                                    end if;

                                    while SwP <= Endp loop
                                       declare
                                          Dir_Is_Wild       : Boolean := False;
                                          Dir_Maybe_Is_Wild : Boolean := False;
                                          Dir_List : String_Access_List_Access;
                                       begin
                                          P2 := SwP;

                                          while P2 < Endp
                                            and then Arg (P2 + 1) /= ','
                                          loop

                                             --  A wildcard directory spec on
                                             --  VMS will contain either * or
                                             --  % or ...

                                             if Arg (P2) = '*' then
                                                Dir_Is_Wild := True;

                                             elsif Arg (P2) = '%' then
                                                Dir_Is_Wild := True;

                                             elsif Dir_Maybe_Is_Wild
                                               and then Arg (P2) = '.'
                                               and then Arg (P2 + 1) = '.'
                                             then
                                                Dir_Is_Wild := True;
                                                Dir_Maybe_Is_Wild := False;

                                             elsif Dir_Maybe_Is_Wild then
                                                Dir_Maybe_Is_Wild := False;

                                             elsif Arg (P2) = '.'
                                               and then Arg (P2 + 1) = '.'
                                             then
                                                Dir_Maybe_Is_Wild := True;

                                             end if;

                                             P2 := P2 + 1;
                                          end loop;

                                          if (Dir_Is_Wild) then
                                             Dir_List := To_Canonical_File_List
                                               (Arg (SwP .. P2), True);

                                             for I in Dir_List.all'Range loop
                                                Place_Unix_Switches
                                                  (Sw.Unix_String);
                                                Place_Lower
                                                  (Dir_List.all (I).all);
                                             end loop;
                                          else
                                             Place_Unix_Switches
                                               (Sw.Unix_String);
                                             Place_Lower
                                               (To_Canonical_Dir_Spec
                                                (Arg (SwP .. P2), False).all);
                                          end if;

                                          SwP := P2 + 2;
                                       end;
                                    end loop;

                                 when T_Directory =>
                                    if SwP + 1 > Arg'Last then
                                       Put (Standard_Error,
                                            "missing directory for: ");
                                       Put_Line (Standard_Error, Arg.all);
                                       Errors := Errors + 1;

                                    else
                                       Place_Unix_Switches (Sw.Unix_String);

                                       --  Some switches end in "=". No space
                                       --  here

                                       if Sw.Unix_String
                                         (Sw.Unix_String'Last) /= '='
                                       then
                                          Place (' ');
                                       end if;

                                       Place_Lower
                                         (To_Canonical_Dir_Spec
                                          (Arg (SwP + 2 .. Arg'Last),
                                           False).all);
                                    end if;

                                 when T_File | T_No_Space_File =>
                                    if SwP + 1 > Arg'Last then
                                       Put (Standard_Error,
                                            "missing file for: ");
                                       Put_Line (Standard_Error, Arg.all);
                                       Errors := Errors + 1;

                                    else
                                       Place_Unix_Switches (Sw.Unix_String);

                                       --  Some switches end in "=". No space
                                       --  here.

                                       if Sw.Translation = T_File
                                         and then Sw.Unix_String
                                                   (Sw.Unix_String'Last) /= '='
                                       then
                                          Place (' ');
                                       end if;

                                       Place_Lower
                                         (To_Canonical_File_Spec
                                          (Arg (SwP + 2 .. Arg'Last)).all);
                                    end if;

                                 when T_Numeric =>
                                    if
                                      OK_Integer (Arg (SwP + 2 .. Arg'Last))
                                    then
                                       Place_Unix_Switches (Sw.Unix_String);
                                       Place (Arg (SwP + 2 .. Arg'Last));

                                    else
                                       Put (Standard_Error, "argument for ");
                                       Put (Standard_Error, Sw.Name.all);
                                       Put_Line
                                         (Standard_Error, " must be numeric");
                                       Errors := Errors + 1;
                                    end if;

                                 when T_Alphanumplus =>
                                    if
                                      OK_Alphanumerplus
                                        (Arg (SwP + 2 .. Arg'Last))
                                    then
                                       Place_Unix_Switches (Sw.Unix_String);
                                       Place (Arg (SwP + 2 .. Arg'Last));

                                    else
                                       Put (Standard_Error, "argument for ");
                                       Put (Standard_Error, Sw.Name.all);
                                       Put_Line (Standard_Error,
                                                 " must be alphanumeric");
                                       Errors := Errors + 1;
                                    end if;

                                 when T_String =>

                                    --  A String value must be extended to the
                                    --  end of the Argv, otherwise strings like
                                    --  "foo/bar" get split at the slash.
                                    --
                                    --  The begining and ending of the string
                                    --  are flagged with embedded nulls which
                                    --  are removed when building the Spawn
                                    --  call. Nulls are use because they won't
                                    --  show up in a /? output. Quotes aren't
                                    --  used because that would make it
                                    --  difficult to embed them.

                                    Place_Unix_Switches (Sw.Unix_String);
                                    if Next_Arg_Idx /= Argv'Last then
                                       Next_Arg_Idx := Argv'Last;
                                       Arg := new String'
                                         (Argv (Arg_Idx .. Next_Arg_Idx));

                                       SwP := Arg'First;
                                       while SwP < Arg'Last and then
                                         Arg (SwP + 1) /= '=' loop
                                          SwP := SwP + 1;
                                       end loop;
                                    end if;
                                    Place (ASCII.NUL);
                                    Place (Arg (SwP + 2 .. Arg'Last));
                                    Place (ASCII.NUL);

                                 when T_Commands =>

                                    --  Output -largs/-bargs/-cargs

                                    Place (' ');
                                    Place (Sw.Unix_String
                                           (Sw.Unix_String'First ..
                                            Sw.Unix_String'First + 5));

                                    --  Set source of new commands, also
                                    --  setting this non-null indicates that
                                    --  we are in the special commands mode
                                    --  for processing the -xargs case.

                                    Make_Commands_Active :=
                                      Matching_Name
                                      (Sw.Unix_String
                                       (Sw.Unix_String'First + 7 ..
                                        Sw.Unix_String'Last),
                                       Commands);

                                 when T_Options =>
                                    if SwP + 1 > Arg'Last then
                                       Place_Unix_Switches
                                         (Sw.Options.Unix_String);
                                       SwP := Endp + 1;

                                    elsif Arg (SwP + 2) /= '(' then
                                       SwP := SwP + 2;
                                       Endp := Arg'Last;

                                    elsif Arg (Arg'Last) /= ')' then
                                       Put
                                         (Standard_Error,
                                          "incorrectly parenthesized " &
                                          "argument: ");
                                       Put_Line (Standard_Error, Arg.all);
                                       Errors := Errors + 1;
                                       SwP := Endp + 1;

                                    else
                                       SwP := SwP + 3;
                                       Endp := Arg'Last - 1;
                                    end if;

                                    while SwP <= Endp loop
                                       P2 := SwP;

                                       while P2 < Endp
                                         and then Arg (P2 + 1) /= ','
                                       loop
                                          P2 := P2 + 1;
                                       end loop;

                                       --  Option name is in Arg (SwP .. P2)

                                       Opt := Matching_Name (Arg (SwP .. P2),
                                                             Sw.Options);

                                       if Opt /= null then
                                          Place_Unix_Switches
                                            (Opt.Unix_String);
                                       end if;

                                       SwP := P2 + 2;
                                    end loop;

                                 when T_Other =>
                                    Place_Unix_Switches
                                      (new String'(Sw.Unix_String.all &
                                                   Arg.all));

                              end case;
                           end if;
                        end;
                     end if;

                     Arg_Idx := Next_Arg_Idx + 1;
                  end;

                  exit when Arg_Idx > Argv'Last;

               end loop;
         end Process_Argument;

         Arg_Num := Arg_Num + 1;
      end loop;

      if Display_Command then
         Put (Standard_Error, "generated command -->");
         Put (Standard_Error, Command_List (The_Command).Unixcmd.all);

         if Command_List (The_Command).Unixsws /= null then
            for J in Command_List (The_Command).Unixsws'Range loop
               Put (Standard_Error, " ");
               Put (Standard_Error,
                    Command_List (The_Command).Unixsws (J).all);
            end loop;
         end if;

         Put (Standard_Error, " ");
         Put (Standard_Error, String (Buffer.Table (1 .. Buffer.Last)));
         Put (Standard_Error, "<--");
         New_Line (Standard_Error);
         raise Normal_Exit;
      end if;

      --  Gross error checking that the number of parameters is correct.
      --  Not applicable to Unlimited_Files parameters.

      if (Param_Count = Command.Params'Length - 1
            and then Command.Params (Param_Count + 1) = Unlimited_Files)
        or else Param_Count <= Command.Params'Length
      then
         null;

      else
         Put_Line (Standard_Error,
                   "Parameter count of "
                   & Integer'Image (Param_Count)
                   & " not equal to expected "
                   & Integer'Image (Command.Params'Length));
         Put (Standard_Error, "usage: ");
         Put_Line (Standard_Error, Command.Usage.all);
         Errors := Errors + 1;
      end if;

      if Errors > 0 then
         raise Error_Exit;
      else
         --  Prepare arguments for a call to spawn, filtering out
         --  embedded nulls place there to delineate strings.

         declare
            P1, P2     : Natural;
            Inside_Nul : Boolean := False;
            Arg        : String (1 .. 1024);
            Arg_Ctr    : Natural;

         begin
            P1 := 1;

            while P1 <= Buffer.Last and then Buffer.Table (P1) = ' ' loop
               P1 := P1 + 1;
            end loop;

            Arg_Ctr := 1;
            Arg (Arg_Ctr) := Buffer.Table (P1);

            while P1 <= Buffer.Last loop

               if Buffer.Table (P1) = ASCII.NUL then
                  if Inside_Nul then
                     Inside_Nul := False;
                  else
                     Inside_Nul := True;
                  end if;
               end if;

               if Buffer.Table (P1) = ' ' and then not Inside_Nul then
                  P1 := P1 + 1;
                  Arg_Ctr := Arg_Ctr + 1;
                  Arg (Arg_Ctr) := Buffer.Table (P1);

               else
                  Last_Switches.Increment_Last;
                  P2 := P1;

                  while P2 < Buffer.Last
                    and then (Buffer.Table (P2 + 1) /= ' ' or else
                              Inside_Nul)
                  loop
                     P2 := P2 + 1;
                     Arg_Ctr := Arg_Ctr + 1;
                     Arg (Arg_Ctr) := Buffer.Table (P2);
                     if Buffer.Table (P2) = ASCII.NUL then
                        Arg_Ctr := Arg_Ctr - 1;
                        if Inside_Nul then
                           Inside_Nul := False;
                        else
                           Inside_Nul := True;
                        end if;
                     end if;
                  end loop;

                  Last_Switches.Table (Last_Switches.Last) :=
                    new String'(String (Arg (1 .. Arg_Ctr)));
                  P1 := P2 + 2;
                  Arg_Ctr := 1;
                  Arg (Arg_Ctr) := Buffer.Table (P1);
               end if;
            end loop;
         end;
      end if;
   end VMS_Conversion;

   -------------------------------------
   -- Start of processing for GNATCmd --
   -------------------------------------

begin
   --  Initializations

   Namet.Initialize;
   Csets.Initialize;

   Snames.Initialize;

   Prj.Initialize;

   Last_Switches.Init;
   Last_Switches.Set_Last (0);

   First_Switches.Init;
   First_Switches.Set_Last (0);

   --  If on VMS, or if VMS emulation is on, convert VMS style /qualifiers,
   --  filenames and pathnames to Unix style.

   if Hostparm.OpenVMS
     or else To_Lower (Getenv ("EMULATE_VMS").all) = "true"
   then
      VMS_Conversion (The_Command);

   --  If not on VMS, scan the command line directly

   else
      if Argument_Count = 0 then
         Non_VMS_Usage;
         return;
      else
         begin
            if Argument_Count > 1 and then Argument (1) = "-v" then
               Opt.Verbose_Mode := True;
               Command_Arg := 2;
            end if;

            The_Command := Real_Command_Type'Value (Argument (Command_Arg));

            if Command_List (The_Command).VMS_Only then
               Non_VMS_Usage;
               Fail ("Command """ & Command_List (The_Command).Cname.all &
                     """ can only be used on VMS");
            end if;
         exception
            when Constraint_Error =>

               --  Check if it is an alternate command
               declare
                  Alternate : Alternate_Command;

               begin
                  Alternate := Alternate_Command'Value
                                              (Argument (Command_Arg));
                  The_Command := Corresponding_To (Alternate);

               exception
                  when Constraint_Error =>
                     Non_VMS_Usage;
                     Fail ("Unknown command: " & Argument (Command_Arg));
               end;
         end;

         for Arg in Command_Arg + 1 .. Argument_Count loop
            Last_Switches.Increment_Last;
            Last_Switches.Table (Last_Switches.Last) :=
              new String'(Argument (Arg));
         end loop;
      end if;
   end if;

   declare
      Program : constant String :=
        Program_Name (Command_List (The_Command).Unixcmd.all).all;

      Exec_Path : String_Access;

   begin
      --  Locate the executable for the command

      Exec_Path := Locate_Exec_On_Path (Program);

      if Exec_Path = null then
         Put_Line (Standard_Error, "Couldn't locate " & Program);
         raise Error_Exit;
      end if;

      --  If there are switches for the executable, put them as first switches

      if Command_List (The_Command).Unixsws /= null then
         for J in Command_List (The_Command).Unixsws'Range loop
            First_Switches.Increment_Last;
            First_Switches.Table (First_Switches.Last) :=
              Command_List (The_Command).Unixsws (J);
         end loop;
      end if;

      --  For BIND, FIND, LINK, LIST and XREF, look for project file related
      --  switches.

      if The_Command = Bind
        or else The_Command = Find
        or else The_Command = Link
        or else The_Command = List
        or else The_Command = Xref
      then
         case The_Command is
            when Bind =>
               Tool_Package_Name := Name_Binder;
            when Find =>
               Tool_Package_Name := Name_Finder;
            when Link =>
               Tool_Package_Name := Name_Linker;
            when List =>
               Tool_Package_Name := Name_Gnatls;
            when Xref =>
               Tool_Package_Name := Name_Cross_Reference;
            when others =>
               null;
         end case;

         declare
            Arg_Num : Positive := 1;
            Argv    : String_Access;

            procedure Remove_Switch (Num : Positive);
            --  Remove a project related switch from table Last_Switches

            -------------------
            -- Remove_Switch --
            -------------------

            procedure Remove_Switch (Num : Positive) is
            begin
               Last_Switches.Table (Num .. Last_Switches.Last - 1) :=
                 Last_Switches.Table (Num + 1 .. Last_Switches.Last);
               Last_Switches.Decrement_Last;
            end Remove_Switch;

         --  Start of processing for ??? (need block name here)

         begin
            while Arg_Num <= Last_Switches.Last loop
               Argv := Last_Switches.Table (Arg_Num);

               if Argv (Argv'First) = '-' then
                  if Argv'Length = 1 then
                     Fail ("switch character cannot be followed by a blank");
                  end if;

                  --  The two style project files (-p and -P) cannot be used
                  --  together

                  if (The_Command = Find or else The_Command = Xref)
                    and then Argv (2) = 'p'
                  then
                     Old_Project_File_Used := True;
                     if Project_File /= null then
                        Fail ("-P and -p cannot be used together");
                     end if;
                  end if;

                  --  -vPx  Specify verbosity while parsing project files

                  if Argv'Length = 4
                    and then Argv (Argv'First + 1 .. Argv'First + 2) = "vP"
                  then
                     case Argv (Argv'Last) is
                        when '0' =>
                           Current_Verbosity := Prj.Default;
                        when '1' =>
                           Current_Verbosity := Prj.Medium;
                        when '2' =>
                           Current_Verbosity := Prj.High;
                        when others =>
                           Fail ("Invalid switch: " & Argv.all);
                     end case;

                     Remove_Switch (Arg_Num);

                  --  -Pproject_file  Specify project file to be used

                  elsif Argv'Length >= 3
                    and then Argv (Argv'First + 1) = 'P'
                  then

                     --  Only one -P switch can be used

                     if Project_File /= null then
                        Fail (Argv.all &
                              ": second project file forbidden (first is """ &
                              Project_File.all & """)");

                     --  The two style project files (-p and -P) cannot be
                     --  used together.

                     elsif Old_Project_File_Used then
                        Fail ("-p and -P cannot be used together");

                     else
                        Project_File :=
                          new String'(Argv (Argv'First + 2 .. Argv'Last));
                     end if;

                     Remove_Switch (Arg_Num);

                  --  -Xexternal=value Specify an external reference to be
                  --                   used in project files

                  elsif Argv'Length >= 5
                    and then Argv (Argv'First + 1) = 'X'
                  then
                     declare
                        Equal_Pos : constant Natural :=
                          Index ('=', Argv (Argv'First + 2 .. Argv'Last));
                     begin
                        if Equal_Pos >= Argv'First + 3 and then
                          Equal_Pos /= Argv'Last then
                           Add (External_Name =>
                                  Argv (Argv'First + 2 .. Equal_Pos - 1),
                                Value => Argv (Equal_Pos + 1 .. Argv'Last));
                        else
                           Fail (Argv.all &
                                 " is not a valid external assignment.");
                        end if;
                     end;

                     Remove_Switch (Arg_Num);

                  else
                     Arg_Num := Arg_Num + 1;
                  end if;

               else
                  Arg_Num := Arg_Num + 1;
               end if;
            end loop;
         end;
      end if;

      --  If there is a project file specified, parse it, get the switches
      --  for the tool and setup PATH environment variables.

      if Project_File /= null then
         Prj.Pars.Set_Verbosity (To => Current_Verbosity);

         Prj.Pars.Parse
           (Project           => Project,
            Project_File_Name => Project_File.all);

         if Project = Prj.No_Project then
            Fail ("""" & Project_File.all & """ processing failed");
         end if;

         --  Check if a package with the name of the tool is in the project
         --  file and if there is one, get the switches, if any, and scan them.

         declare
            Data : Prj.Project_Data := Prj.Projects.Table (Project);
            Pkg  : Prj.Package_Id :=
                     Prj.Util.Value_Of
                       (Name        => Tool_Package_Name,
                        In_Packages => Data.Decl.Packages);

            Element : Package_Element;

            Default_Switches_Array : Array_Element_Id;

            The_Switches : Prj.Variable_Value;
            Current      : Prj.String_List_Id;
            The_String   : String_Element;

         begin
            if Pkg /= No_Package then
               Element := Packages.Table (Pkg);

               --  Packages Gnatls has a single attribute Switches, that is
               --  not an associative array.

               if The_Command = List then
                  The_Switches :=
                    Prj.Util.Value_Of
                    (Variable_Name => Snames.Name_Switches,
                     In_Variables => Element.Decl.Attributes);

               --  Packages Binder (for gnatbind), Cross_Reference (for
               --  gnatxref), Linker (for gnatlink) and Finder
               --  (for gnatfind) have an attributed Default_Switches,
               --  an associative array, indexed by the name of the
               --  programming language.
               else
                  Default_Switches_Array :=
                    Prj.Util.Value_Of
                    (Name => Name_Default_Switches,
                     In_Arrays => Packages.Table (Pkg).Decl.Arrays);
                  The_Switches := Prj.Util.Value_Of
                    (Index => Name_Ada,
                     In_Array => Default_Switches_Array);

               end if;

               --  If there are switches specified in the package of the
               --  project file corresponding to the tool, scan them.

               case The_Switches.Kind is
                  when Prj.Undefined =>
                     null;

                  when Prj.Single =>
                     if String_Length (The_Switches.Value) > 0 then
                        String_To_Name_Buffer (The_Switches.Value);
                        First_Switches.Increment_Last;
                        First_Switches.Table (First_Switches.Last) :=
                          new String'(Name_Buffer (1 .. Name_Len));
                     end if;

                  when Prj.List =>
                     Current := The_Switches.Values;
                     while Current /= Prj.Nil_String loop
                        The_String := String_Elements.Table (Current);

                        if String_Length (The_String.Value) > 0 then
                           String_To_Name_Buffer (The_String.Value);
                           First_Switches.Increment_Last;
                           First_Switches.Table (First_Switches.Last) :=
                             new String'(Name_Buffer (1 .. Name_Len));
                        end if;

                        Current := The_String.Next;
                     end loop;
               end case;
            end if;
         end;

         --  Set up the environment variables ADA_INCLUDE_PATH and
         --  ADA_OBJECTS_PATH.

         Setenv
           (Name  => Ada_Include_Path,
            Value => Prj.Env.Ada_Include_Path (Project).all);
         Setenv
           (Name  => Ada_Objects_Path,
            Value => Prj.Env.Ada_Objects_Path
            (Project, Including_Libraries => False).all);

         if The_Command = Bind or else The_Command = Link then
            Change_Dir
              (Get_Name_String
                 (Projects.Table (Project).Object_Directory));
         end if;

         if The_Command = Link then

            --  Add the default search directories, to be able to find
            --  libgnat in call to MLib.Utl.Lib_Directory.

            Add_Default_Search_Dirs;

            declare
               There_Are_Libraries  : Boolean := False;

            begin
               --  Check if there are library project files

               if MLib.Tgt.Libraries_Are_Supported then
                  Set_Libraries (Project, There_Are_Libraries);
               end if;

               --  If there are, add the necessary additional switches

               if There_Are_Libraries then

                  --  Add -L<lib_dir> -lgnarl -lgnat -Wl,-rpath,<lib_dir>

                  Last_Switches.Increment_Last;
                  Last_Switches.Table (Last_Switches.Last) :=
                    new String'("-L" & MLib.Utl.Lib_Directory);
                  Last_Switches.Increment_Last;
                  Last_Switches.Table (Last_Switches.Last) :=
                    new String'("-lgnarl");
                  Last_Switches.Increment_Last;
                  Last_Switches.Table (Last_Switches.Last) :=
                    new String'("-lgnat");

                  declare
                     Option : constant String_Access :=
                       MLib.Tgt.Linker_Library_Path_Option
                       (MLib.Utl.Lib_Directory);

                  begin
                     if Option /= null then
                        Last_Switches.Increment_Last;
                        Last_Switches.Table (Last_Switches.Last) :=
                          Option;
                     end if;
                  end;
               end if;
            end;
         end if;
      end if;

      --  Gather all the arguments and invoke the executable

      declare
         The_Args : Argument_List
           (1 .. First_Switches.Last + Last_Switches.Last);
         Arg_Num : Natural := 0;
      begin
         for J in 1 .. First_Switches.Last loop
            Arg_Num := Arg_Num + 1;
            The_Args (Arg_Num) := First_Switches.Table (J);
         end loop;

         for J in 1 .. Last_Switches.Last loop
            Arg_Num := Arg_Num + 1;
            The_Args (Arg_Num) := Last_Switches.Table (J);
         end loop;

         if Opt.Verbose_Mode then
            Output.Write_Str (Exec_Path.all);

            for Arg in The_Args'Range loop
               Output.Write_Char (' ');
               Output.Write_Str (The_Args (Arg).all);
            end loop;

            Output.Write_Eol;
         end if;

         My_Exit_Status
           := Exit_Status (Spawn (Exec_Path.all, The_Args));
         raise Normal_Exit;
      end;
   end;

exception
   when Error_Exit =>
      Set_Exit_Status (Failure);

   when Normal_Exit =>
      Set_Exit_Status (My_Exit_Status);

end GNATCmd;
