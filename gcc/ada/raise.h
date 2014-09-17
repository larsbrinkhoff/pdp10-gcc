/****************************************************************************
 *                                                                          *
 *                         GNAT COMPILER COMPONENTS                         *
 *                                                                          *
 *                                R A I S E                                 *
 *                                                                          *
 *                              C Header File                               *
 *                                                                          *
 *                                                                          *
 *          Copyright (C) 1992-2001, Free Software Foundation, Inc.         *
 *                                                                          *
 * GNAT is free software;  you can  redistribute it  and/or modify it under *
 * terms of the  GNU General Public License as published  by the Free Soft- *
 * ware  Foundation;  either version 2,  or (at your option) any later ver- *
 * sion.  GNAT is distributed in the hope that it will be useful, but WITH- *
 * OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License *
 * for  more details.  You should have  received  a copy of the GNU General *
 * Public License  distributed with GNAT;  see file COPYING.  If not, write *
 * to  the Free Software Foundation,  59 Temple Place - Suite 330,  Boston, *
 * MA 02111-1307, USA.                                                      *
 *                                                                          *
 * As a  special  exception,  if you  link  this file  with other  files to *
 * produce an executable,  this file does not by itself cause the resulting *
 * executable to be covered by the GNU General Public License. This except- *
 * ion does not  however invalidate  any other reasons  why the  executable *
 * file might be covered by the  GNU Public License.                        *
 *                                                                          *
 * GNAT was originally developed  by the GNAT team at  New York University. *
 * It is now maintained by Ada Core Technologies Inc (http://www.gnat.com). *
 *                                                                          *
 ****************************************************************************/

struct Exception_Data
{ 
  char  Handled_By_Others;
  char Lang;
  int Name_Length;
  char *Full_Name, Htable_Ptr;
  int Import_Code;
};

typedef struct Exception_Data *Exception_Id;

struct Exception_Occurrence
{
  int Max_Length;
  Exception_Id Id;
  int Msg_Length;
  char Msg[0];
};

typedef struct Exception_Occurrence *Exception_Occurrence_Access;

extern void _gnat_builtin_longjmp	PARAMS ((void *, int));
extern void __gnat_unhandled_terminate	PARAMS ((void));
extern void *__gnat_malloc		PARAMS ((__SIZE_TYPE__));
extern void __gnat_free			PARAMS ((void *));
extern void *__gnat_realloc		PARAMS ((void *, __SIZE_TYPE__));
extern void __gnat_finalize		PARAMS ((void));
extern void set_gnat_exit_status	PARAMS ((int));
extern void __gnat_set_globals		PARAMS ((int, int, 
						 char, char, char, char,
						 char *, int, int, int));
extern void __gnat_initialize		PARAMS ((void));
extern void __gnat_init_float		PARAMS ((void));
extern void __gnat_install_handler	PARAMS ((void));

extern int gnat_exit_status;
