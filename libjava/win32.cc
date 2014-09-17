// win32.cc - Helper functions for Microsoft-flavored OSs.

/* Copyright (C) 2002  Free Software Foundation

   This file is part of libgcj.

This software is copyrighted work licensed under the terms of the
Libgcj License.  Please consult the file "LIBGCJ_LICENSE" for
details.  */

#include <config.h>
#include <jvm.h>
#include <sys/timeb.h>
#include <stdlib.h>

#include "platform.h"
#include <java/lang/ArithmeticException.h>
#include <java/util/Properties.h>

static LONG CALLBACK
win32_exception_handler (LPEXCEPTION_POINTERS e)
{
  if (e->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    _Jv_ThrowNullPointerException();
  else if (e->ExceptionRecord->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO)
    throw new java::lang::ArithmeticException;
  else
    return EXCEPTION_CONTINUE_SEARCH;
}

// Platform-specific VM initialization.
void
_Jv_platform_initialize (void)
{
  // Initialise winsock for networking
  WSADATA data;
  if (WSAStartup (MAKEWORD (1, 1), &data))
    MessageBox (NULL, "Error initialising winsock library.", "Error",
		MB_OK | MB_ICONEXCLAMATION);
  // Install exception handler
  SetUnhandledExceptionFilter (win32_exception_handler);
}

// gettimeofday implementation.
jlong
_Jv_platform_gettimeofday ()
{
  struct timeb t;
  ftime (&t);
  return t.time * 1000LL + t.millitm;
}

// The following definitions "fake out" mingw to think that -mthreads
// was enabled and that mingwthr.dll was linked. GCJ-compiled
// applications don't need this helper library because we can safely
// detect thread death (return from Thread.run()).

int _CRT_MT = 1;

extern "C" int
__mingwthr_key_dtor (DWORD, void (*) (void *))
{
  // FIXME: for now we do nothing; this causes a memory leak of
  //        approximately 24 bytes per thread created.
  return 0;
}

// Set platform-specific System properties.
void
_Jv_platform_initProperties (java::util::Properties* newprops)
{
  // A convenience define.
#define SET(Prop,Val) \
  newprops->put(JvNewStringLatin1 (Prop), JvNewStringLatin1 (Val))

  SET ("file.separator", "\\");
  SET ("path.separator", ";");
  SET ("line.separator", "\r\n");

  // Use GetCurrentDirectory to set 'user.dir'.
  DWORD buflen = MAX_PATH;
  char *buffer = (char *) _Jv_MallocUnchecked (buflen);
  if (buffer != NULL)
    {
      if (GetCurrentDirectory (buflen, buffer))
	SET ("user.dir", buffer);

      if (GetTempPath (buflen, buffer))
	SET ("java.io.tmpdir", buffer);

      _Jv_Free (buffer);
    }
  
  // Use GetUserName to set 'user.name'.
  buflen = 257;  // UNLEN + 1
  buffer = (char *) _Jv_MallocUnchecked (buflen);
  if (buffer != NULL)
    {
      if (GetUserName (buffer, &buflen))
        SET ("user.name", buffer);
      _Jv_Free (buffer);
    }

  // According to the api documentation for 'GetWindowsDirectory()', the 
  // environmental variable HOMEPATH always specifies the user's home 
  // directory or a default directory.  On the 3 windows machines I checked
  // only 1 had it set.  If it's not set, JDK1.3.1 seems to set it to
  // the windows directory, so we'll do the same.
  char *userHome = NULL;
  if ((userHome = ::getenv ("HOMEPATH")) == NULL )
    {
      // Check HOME since it's what I use.
      if ((userHome = ::getenv ("HOME")) == NULL )
        {
          // Not found - use the windows directory like JDK1.3.1 does.
          char *winHome = (char *) _Jv_MallocUnchecked (MAX_PATH);
          if (winHome != NULL)
            {
              if (GetWindowsDirectory (winHome, MAX_PATH))
		SET ("user.home", winHome);
              _Jv_Free (winHome);
            }
        }
     }
  if (userHome != NULL)
    SET ("user.home", userHome);

  // Get and set some OS info.
  OSVERSIONINFO osvi;
  ZeroMemory (&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (GetVersionEx (&osvi))
    {
      char *buffer = (char *) _Jv_MallocUnchecked (30);
      if (buffer != NULL)
        {
          sprintf (buffer, "%d.%d", (int) osvi.dwMajorVersion,
		   (int) osvi.dwMinorVersion);
          SET ("os.version", buffer);
          _Jv_Free (buffer);
        }

      switch (osvi.dwPlatformId)
        {
          case VER_PLATFORM_WIN32_WINDOWS:
            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
              SET ("os.name", "Windows 95");
            else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
              SET ("os.name", "Windows 98");
            else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
              SET ("os.name", "Windows Me");
            else
              SET ("os.name", "Windows ??"); 
            break;

          case VER_PLATFORM_WIN32_NT:
            if (osvi.dwMajorVersion <= 4 )
              SET ("os.name", "Windows NT");
            else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
              SET ("os.name", "Windows 2000");
            else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
              SET ("os.name", "Windows XP");
            else
              SET ("os.name", "Windows NT ??");
            break;

          default:
            SET ("os.name", "Windows UNKNOWN");
            break;
       }
  }

  // Set the OS architecture.
  SYSTEM_INFO si;
  GetSystemInfo (&si);
  switch (si.dwProcessorType)
    {
      case PROCESSOR_INTEL_386:
        SET ("os.arch", "i386");
        break;
      case PROCESSOR_INTEL_486:
        SET ("os.arch", "i486");
        break;
      case PROCESSOR_INTEL_PENTIUM:
        SET ("os.arch", "i586");
        break;
      case PROCESSOR_MIPS_R4000:	
        SET ("os.arch", "MIPS4000");
        break;
      case PROCESSOR_ALPHA_21064:
        SET ("os.arch", "ALPHA");
        break;
      default:
        SET ("os.arch", "unknown");
        break;
    }
}
