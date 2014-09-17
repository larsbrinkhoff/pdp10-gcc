------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                    G N A T . S O C K E T S . T H I N                     --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--                                                                          --
--              Copyright (C) 2001 Ada Core Technologies, Inc.              --
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
-- GNAT is maintained by Ada Core Technologies Inc (http://www.gnat.com).   --
--                                                                          --
------------------------------------------------------------------------------

with Interfaces.C.Pointers;

with Interfaces.C.Strings;
with GNAT.Sockets.Constants;
with GNAT.OS_Lib;

with System;

package GNAT.Sockets.Thin is

   --  ??? more comments needed ???

   --  This package is intended for hosts implementing BSD sockets with a
   --  standard interface. It will be used as a default for all the platforms
   --  that do not have a specific version of this file.

   package C renames Interfaces.C;

   use type C.int;
   --  This is so we can declare the Failure constant below

   Success : constant C.int :=  0;
   Failure : constant C.int := -1;

   function Socket_Errno return Integer renames GNAT.OS_Lib.Errno;
   --  Returns last socket error number.

   function Socket_Error_Message (Errno : Integer) return String;
   --  Returns the error message string for the error number Errno. If
   --  Errno is not known it returns "Unknown system error".

   type Fd_Set is mod 2 ** 32;
   pragma Convention (C, Fd_Set);

   Null_Fd_Set : constant Fd_Set := 0;

   type Fd_Set_Access is access all Fd_Set;
   pragma Convention (C, Fd_Set_Access);

   type Timeval_Unit is new C.int;
   pragma Convention (C, Timeval_Unit);

   type Timeval is record
      Tv_Sec  : Timeval_Unit;
      Tv_Usec : Timeval_Unit;
   end record;
   pragma Convention (C, Timeval);

   type Timeval_Access is access all Timeval;
   pragma Convention (C, Timeval_Access);

   Immediat : constant Timeval := (0, 0);

   type Int_Access is access all C.int;
   pragma Convention (C, Int_Access);
   --  Access to C integers

   type Chars_Ptr_Array is array (C.size_t range <>) of
     aliased C.Strings.chars_ptr;

   package Chars_Ptr_Pointers is
      new C.Pointers (C.size_t, C.Strings.chars_ptr, Chars_Ptr_Array,
                      C.Strings.Null_Ptr);
   --  Arrays of C (char *)

   type In_Addr is record
      S_B1, S_B2, S_B3, S_B4 : C.unsigned_char;
   end record;
   pragma Convention (C, In_Addr);
   --  Internet address

   type In_Addr_Access is access all In_Addr;
   pragma Convention (C, In_Addr_Access);
   --  Access to internet address

   Inaddr_Any : aliased constant In_Addr := (others => 0);
   --  Any internet address (all the interfaces)

   type In_Addr_Access_Array is array (C.size_t range <>)
     of aliased In_Addr_Access;
   pragma Convention (C, In_Addr_Access_Array);

   package In_Addr_Access_Pointers is
     new C.Pointers (C.size_t, In_Addr_Access, In_Addr_Access_Array, null);
   --  Array of internet addresses

   type Sockaddr is record
      Sa_Family : C.unsigned_short;
      Sa_Data   : C.char_array (1 .. 14);
   end record;
   pragma Convention (C, Sockaddr);
   --  Socket address

   type Sockaddr_Access is access all Sockaddr;
   pragma Convention (C, Sockaddr_Access);
   --  Access to socket address

   type Sockaddr_In is record
      Sin_Family : C.unsigned_short      := Constants.AF_INET;
      Sin_Port   : C.unsigned_short      := 0;
      Sin_Addr   : In_Addr               := Inaddr_Any;
      Sin_Zero   : C.char_array (1 .. 8) := (others => C.char'Val (0));
   end record;
   pragma Convention (C, Sockaddr_In);
   --  Internet socket address

   type Sockaddr_In_Access is access all Sockaddr_In;
   pragma Convention (C, Sockaddr_In_Access);
   --  Access to internet socket address

   type Hostent is record
      H_Name      : C.Strings.chars_ptr;
      H_Aliases   : Chars_Ptr_Pointers.Pointer;
      H_Addrtype  : C.int;
      H_Length    : C.int;
      H_Addr_List : In_Addr_Access_Pointers.Pointer;
   end record;
   pragma Convention (C, Hostent);
   --  Host entry

   type Hostent_Access is access all Hostent;
   pragma Convention (C, Hostent_Access);
   --  Access to host entry

   type Two_Int is array (0 .. 1) of C.int;
   pragma Convention (C, Two_Int);
   --  Used with pipe()

   function C_Accept
     (S       : C.int;
      Addr    : System.Address;
      Addrlen : access C.int)
      return    C.int;

   function C_Bind
     (S       : C.int;
      Name    : System.Address;
      Namelen : C.int)
      return    C.int;

   function C_Close
     (Fd   : C.int)
      return C.int;

   function C_Connect
     (S       : C.int;
      Name    : System.Address;
      Namelen : C.int)
      return    C.int;

   function C_Gethostbyaddr
     (Addr : System.Address;
      Len  : C.int;
      Typ  : C.int)
      return Hostent_Access;

   function C_Gethostbyname
     (Name : C.char_array)
      return Hostent_Access;

   function C_Gethostname
     (Name    : System.Address;
      Namelen : C.int)
      return    C.int;

   function C_Getpeername
     (S       : C.int;
      Name    : System.Address;
      Namelen : access C.int)
      return    C.int;

   function C_Getsockname
     (S       : C.int;
      Name    : System.Address;
      Namelen : access C.int)
      return    C.int;

   function C_Getsockopt
     (S       : C.int;
      Level   : C.int;
      Optname : C.int;
      Optval  : System.Address;
      Optlen  : access C.int)
      return    C.int;

   function C_Inet_Addr
     (Cp   : C.Strings.chars_ptr)
      return C.int;

   function C_Ioctl
     (S    : C.int;
      Req  : C.int;
      Arg  : Int_Access)
      return C.int;

   function C_Listen (S, Backlog : C.int) return C.int;

   function C_Read
     (Fd    : C.int;
      Buf   : System.Address;
      Count : C.int)
      return  C.int;

   function C_Recv
     (S     : C.int;
      Msg   : System.Address;
      Len   : C.int;
      Flags : C.int)
      return  C.int;

   function C_Recvfrom
     (S       : C.int;
      Msg     : System.Address;
      Len     : C.int;
      Flags   : C.int;
      From    : Sockaddr_In_Access;
      Fromlen : access C.int)
      return    C.int;

   function C_Select
     (Nfds      : C.int;
      Readfds   : Fd_Set_Access;
      Writefds  : Fd_Set_Access;
      Exceptfds : Fd_Set_Access;
      Timeout   : Timeval_Access)
      return      C.int;

   function C_Send
     (S     : C.int;
      Msg   : System.Address;
      Len   : C.int;
      Flags : C.int)
      return  C.int;

   function C_Sendto
     (S     : C.int;
      Msg   : System.Address;
      Len   : C.int;
      Flags : C.int;
      To    : Sockaddr_In_Access;
      Tolen : C.int)
      return  C.int;

   function C_Setsockopt
     (S       : C.int;
      Level   : C.int;
      Optname : C.int;
      Optval  : System.Address;
      Optlen  : C.int)
      return    C.int;

   function C_Shutdown
     (S    : C.int;
      How  : C.int)
      return C.int;

   function C_Socket
     (Domain   : C.int;
      Typ      : C.int;
      Protocol : C.int)
      return     C.int;

   function C_Strerror
     (Errnum : C.int)
      return   C.Strings.chars_ptr;

   function C_System
     (Command : System.Address)
      return    C.int;

   function C_Write
     (Fd    : C.int;
      Buf   : System.Address;
      Count : C.int)
      return  C.int;

   --  Return highest numbered socket (what does this refer to???)

   procedure Clear    (Item : in out Fd_Set; Socket : in C.int);
   procedure Empty    (Item : in out Fd_Set);
   function  Is_Empty (Item : Fd_Set) return Boolean;
   function  Is_Set   (Item : Fd_Set; Socket : C.int) return Boolean;
   function  Max      (Item : Fd_Set) return C.int;
   procedure Set      (Item : in out Fd_Set; Socket : in C.int);

   procedure Finalize;
   procedure Initialize (Process_Blocking_IO : Boolean);

private

   pragma Import (C, C_Bind, "bind");
   pragma Import (C, C_Close, "close");
   pragma Import (C, C_Gethostbyaddr, "gethostbyaddr");
   pragma Import (C, C_Gethostbyname, "gethostbyname");
   pragma Import (C, C_Gethostname, "gethostname");
   pragma Import (C, C_Getpeername, "getpeername");
   pragma Import (C, C_Getsockname, "getsockname");
   pragma Import (C, C_Getsockopt, "getsockopt");
   pragma Import (C, C_Inet_Addr, "inet_addr");
   pragma Import (C, C_Listen, "listen");
   pragma Import (C, C_Read, "read");
   pragma Import (C, C_Select, "select");
   pragma Import (C, C_Setsockopt, "setsockopt");
   pragma Import (C, C_Shutdown, "shutdown");
   pragma Import (C, C_Strerror, "strerror");
   pragma Import (C, C_System, "system");
   pragma Import (C, C_Write, "write");

end GNAT.Sockets.Thin;
