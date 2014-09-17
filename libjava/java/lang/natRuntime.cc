// natRuntime.cc - Implementation of native side of Runtime class.

/* Copyright (C) 1998, 1999, 2000, 2001, 2002  Free Software Foundation

   This file is part of libgcj.

This software is copyrighted work licensed under the terms of the
Libgcj License.  Please consult the file "LIBGCJ_LICENSE" for
details.  */

#include <config.h>

#include <stdlib.h>

#include <gcj/cni.h>
#include <jvm.h>
#include <java/lang/Runtime.h>
#include <java/lang/UnknownError.h>
#include <java/lang/UnsatisfiedLinkError.h>
#include <gnu/gcj/runtime/FileDeleter.h>
#include <gnu/gcj/runtime/FinalizerThread.h>

#include <jni.h>

#ifdef USE_LTDL
#include <ltdl.h>

/* FIXME: we don't always need this.  The next libtool will let us use
   AC_LTDL_PREOPEN to see if we do.  */
extern const lt_dlsymlist lt_preloaded_symbols[1] = { { 0, 0 } };

// We keep track of all the libraries loaded by this application.  For
// now we use them to look up symbols for JNI.  `libraries_size' holds
// the total size of the buffer.  `libraries_count' is the number of
// items which are in use.
static int libraries_size;
static int libraries_count;
static lt_dlhandle *libraries;

static void
add_library (lt_dlhandle lib)
{
  if (libraries_count == libraries_size)
    {
      int ns = libraries_size * 2;
      if (ns == 0)
	ns = 10;
      lt_dlhandle *n = (lt_dlhandle *) _Jv_Malloc (ns * sizeof (lt_dlhandle));
      if (libraries)
	{
	  memcpy (n, libraries, libraries_size * sizeof (lt_dlhandle));
	  _Jv_Free (libraries);
	}
      libraries = n;
      libraries_size = ns;
      for (int i = libraries_count; i < libraries_size; ++i)
	libraries[i] = NULL;
    }

  libraries[libraries_count++] = lib;
}

void *
_Jv_FindSymbolInExecutable (const char *symname)
{
  for (int i = 0; i < libraries_count; ++i)
    {
      void *r = lt_dlsym (libraries[i], symname);
      if (r)
	return r;
    }

  return NULL;
}

#else

void *
_Jv_FindSymbolInExecutable (const char *symname)
{
  return NULL;
}

#endif /* USE_LTDL */

void
java::lang::Runtime::exit (jint status)
{
  checkExit (status);
  _exit (status);
}

void
java::lang::Runtime::_exit (jint status)
{
  // Make status right for Unix.  This is perhaps strange.
  if (status < 0 || status > 255)
    status = 255;

  if (finalize_on_exit)
    _Jv_RunAllFinalizers ();

  // Delete all files registered with File.deleteOnExit()
  gnu::gcj::runtime::FileDeleter::deleteOnExitNow ();

  ::exit (status);
}

jlong
java::lang::Runtime::freeMemory (void)
{
  return _Jv_GCFreeMemory ();
}

void
java::lang::Runtime::gc (void)
{
  _Jv_RunGC ();
}

void
java::lang::Runtime::_load (jstring path, jboolean do_search)
{
  JvSynchronize sync (this);
  checkLink (path);
  using namespace java::lang;
#ifdef USE_LTDL
  jint len = _Jv_GetStringUTFLength (path);
  char buf[len + 1 + 3];
  int offset = 0;
#ifndef WIN32
  // On Unix boxes, prefix library name with `lib', for loadLibrary.
  if (do_search)
    {
      strcpy (buf, "lib");
      offset = 3;
    }
#endif
  jsize total = JvGetStringUTFRegion (path, 0, path->length(), &buf[offset]);
  buf[offset + total] = '\0';
  lt_dlhandle h;
  // FIXME: make sure path is absolute.
  {
    // Synchronize on java.lang.Class. This is to protect the class chain from
    // concurrent modification by class registration calls which may be run
    // during the dlopen().
    JvSynchronize sync (&java::lang::Class::class$);
    h = do_search ? lt_dlopenext (buf) : lt_dlopen (buf);
  }
  if (h == NULL)
    {
      const char *msg = lt_dlerror ();
      jstring str = path->concat (JvNewStringLatin1 (": "));
      str = str->concat (JvNewStringLatin1 (msg));
      throw new UnsatisfiedLinkError (str);
    }

  add_library (h);

  void *onload = lt_dlsym (h, "JNI_OnLoad");
  if (onload != NULL)
    {
      JavaVM *vm = _Jv_GetJavaVM ();
      if (vm == NULL)
	{
	  // FIXME: what?
	  return;
	}
      jint vers = ((jint (*) (JavaVM *, void *)) onload) (vm, NULL);
      if (vers != JNI_VERSION_1_1 && vers != JNI_VERSION_1_2
	  && vers != JNI_VERSION_1_4)
	{
	  // FIXME: unload the library.
	  throw new UnsatisfiedLinkError (JvNewStringLatin1 ("unrecognized version from JNI_OnLoad"));
	}
    }
#else
  throw new UnknownError
    (JvNewStringLatin1 (do_search
			? "Runtime.loadLibrary not implemented"
			: "Runtime.load not implemented"));
#endif /* USE_LTDL */
}

jboolean
java::lang::Runtime::loadLibraryInternal (jstring lib)
{
  JvSynchronize sync (this);
  using namespace java::lang;
#ifdef USE_LTDL
  jint len = _Jv_GetStringUTFLength (lib);
  char buf[len + 1];
  jsize total = JvGetStringUTFRegion (lib, 0, lib->length(), buf);
  buf[total] = '\0';
  // FIXME: make sure path is absolute.
  lt_dlhandle h = lt_dlopenext (buf);
  if (h != NULL)
    add_library (h);
  return h != NULL;
#else
  return false;
#endif /* USE_LTDL */
}

void
java::lang::Runtime::init (void)
{
  finalize_on_exit = false;
#ifdef USE_LTDL
  lt_dlinit ();
  lt_dlhandle self = lt_dlopen (NULL);
  if (self != NULL)
    add_library (self);
#endif
}

void
java::lang::Runtime::runFinalization (void)
{
  gnu::gcj::runtime::FinalizerThread::finalizerReady ();
}

jlong
java::lang::Runtime::totalMemory (void)
{
  return _Jv_GCTotalMemory ();
}

void
java::lang::Runtime::traceInstructions (jboolean)
{
  // Do nothing.
}

void
java::lang::Runtime::traceMethodCalls (jboolean)
{
  // Do nothing.
}
