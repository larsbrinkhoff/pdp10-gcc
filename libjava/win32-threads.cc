// win32-threads.cc - interface between libjava and Win32 threads.

/* Copyright (C) 1998, 1999  Free Software Foundation, Inc.

   This file is part of libgcj.

This software is copyrighted work licensed under the terms of the
Libgcj License.  Please consult the file "LIBGCJ_LICENSE" for
details.  */

#include <config.h>

// If we're using the Boehm GC, then we need to override some of the
// thread primitives.  This is fairly gross.
#ifdef HAVE_BOEHM_GC
extern "C"
{
#include <gc.h>
// <windows.h> #define's STRICT, which conflicts with Modifier.h
#undef STRICT
};
#endif /* HAVE_BOEHM_GC */

#include <gcj/cni.h>
#include <jvm.h>
#include <java/lang/Thread.h>
#include <java/lang/System.h>

#include <errno.h>

#ifndef ETIMEDOUT
#define ETIMEDOUT 116
#endif

// This is used to implement thread startup.
struct starter
{
  _Jv_ThreadStartFunc *method;
  _Jv_Thread_t *data;
};

// Controls access to the variable below
static HANDLE daemon_mutex;
static HANDLE daemon_cond;
// Number of non-daemon threads - _Jv_ThreadWait returns when this is 0
static int non_daemon_count;

// TLS key get Java object representing the thread
DWORD _Jv_ThreadKey;
// TLS key to get _Jv_Thread_t* representing the thread
DWORD _Jv_ThreadDataKey;

//
// These are the flags that can appear in _Jv_Thread_t.
//

// Thread started.
#define FLAG_START   0x01
// Thread is daemon.
#define FLAG_DAEMON  0x02

//
// Condition variables.
//

// we do lazy creation of Events since CreateEvent() is insanely
// expensive, and because the rest of libgcj will call _Jv_CondInit
// when only a mutex is needed.

inline void
ensure_condvar_initialized(_Jv_ConditionVariable_t *cv)
{
  if (cv->ev[0] == 0) {
    cv->ev[0] = CreateEvent (NULL, 0, 0, NULL);
    if (cv->ev[0] == 0) JvFail("CreateEvent() failed");
    cv->ev[1] = CreateEvent (NULL, 1, 0, NULL);
    if (cv->ev[1] == 0) JvFail("CreateEvent() failed");
  }
}

// Reimplementation of the general algorithm described at
// http://www.cs.wustl.edu/~schmidt/win32-cv-1.html (isomorphic to
// 3.2, not a cut-and-paste).

int
_Jv_CondWait(_Jv_ConditionVariable_t *cv, _Jv_Mutex_t *mu, jlong millis, jint nanos)
{

  EnterCriticalSection(&cv->count_mutex);
  ensure_condvar_initialized(cv);
  cv->blocked_count++;
  LeaveCriticalSection(&cv->count_mutex);

  DWORD time;
  if ((millis == 0) && (nanos > 0)) time = 1;
  else if (millis == 0) time = INFINITE;
  else time = millis;

  _Jv_MutexUnlock (mu);

  DWORD rval = WaitForMultipleObjects (2, &(cv->ev[0]), 0, time);

  EnterCriticalSection(&cv->count_mutex);
  cv->blocked_count--;
  // If we were unblocked by the second event (the broadcast one) and nobody is
  // left, then reset the signal.
  int last_waiter = rval == WAIT_OBJECT_0 + 1 && cv->blocked_count == 0;
  LeaveCriticalSection(&cv->count_mutex);

  if (last_waiter) ResetEvent(&cv->ev[1]);

  _Jv_MutexLock (mu);

  if (rval == WAIT_FAILED) return GetLastError();
  else if (rval == WAIT_TIMEOUT) return ETIMEDOUT;
  else return 0;
}

void
_Jv_CondInit (_Jv_ConditionVariable_t *cv)
{
  // we do lazy creation of Events since CreateEvent() is insanely expensive
  cv->ev[0] = 0;
  InitializeCriticalSection(&cv->count_mutex);
  cv->blocked_count = 0;
}

void
_Jv_CondDestroy (_Jv_ConditionVariable_t *cv)
{
  if (cv->ev[0] != 0) CloseHandle(cv->ev[0]);
  cv = NULL;
}

int
_Jv_CondNotify (_Jv_ConditionVariable_t *cv, _Jv_Mutex_t *)
{
  EnterCriticalSection(&cv->count_mutex);
  ensure_condvar_initialized(cv);
  int somebody_is_blocked = cv->blocked_count > 0;
  LeaveCriticalSection(&cv->count_mutex);

  if (somebody_is_blocked) return SetEvent (cv->ev[0]) ? 0 : GetLastError();
  else return 0;
}

int
_Jv_CondNotifyAll (_Jv_ConditionVariable_t *cv, _Jv_Mutex_t *)
{
  EnterCriticalSection(&cv->count_mutex);
  ensure_condvar_initialized(cv);
  int somebody_is_blocked = cv->blocked_count > 0;
  LeaveCriticalSection(&cv->count_mutex);

  if (somebody_is_blocked) return SetEvent (cv->ev[1]) ? 0 : GetLastError();
  else return 0;
}

//
// Threads.
//

void
_Jv_InitThreads (void)
{
  _Jv_ThreadKey = TlsAlloc();
  _Jv_ThreadDataKey = TlsAlloc();
  daemon_mutex = CreateMutex(NULL, 0, NULL);
  daemon_cond = CreateEvent(NULL, 0, 0, NULL);
  non_daemon_count = 0;
}

_Jv_Thread_t *
_Jv_ThreadInitData (java::lang::Thread* obj)
{
  _Jv_Thread_t *data = (_Jv_Thread_t*)_Jv_Malloc(sizeof(_Jv_Thread_t));
  data->flags = 0;
  data->thread_obj = obj;

  return data;
}

void
_Jv_ThreadDestroyData (_Jv_Thread_t *data)
{
  _Jv_Free(data);
}

void
_Jv_ThreadSetPriority (_Jv_Thread_t *data, jint prio)
{
  int actual = THREAD_PRIORITY_NORMAL;

  if (data->flags & FLAG_START)
    {
      switch (prio)
        {
          case 10:
            actual = THREAD_PRIORITY_TIME_CRITICAL;
            break;
          case 9:
            actual = THREAD_PRIORITY_HIGHEST;
            break;
          case 8:
          case 7:
            actual = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
          case 6:
          case 5:
            actual = THREAD_PRIORITY_NORMAL;
            break;
          case 4:
          case 3:
            actual = THREAD_PRIORITY_BELOW_NORMAL;
            break;
          case 2:
            actual = THREAD_PRIORITY_LOWEST;
            break;
          case 1:
            actual = THREAD_PRIORITY_IDLE;
            break;
        }
      SetThreadPriority(data->handle, actual);
    }
}

void
_Jv_ThreadRegister (_Jv_Thread_t *data)
{
  TlsSetValue (_Jv_ThreadKey, data->thread_obj);
  TlsSetValue (_Jv_ThreadDataKey, data);
}

void
_Jv_ThreadUnRegister ()
{
  TlsSetValue (_Jv_ThreadKey, NULL);
  TlsSetValue (_Jv_ThreadDataKey, NULL);
}

// This function is called when a thread is started.  We don't arrange
// to call the `run' method directly, because this function must
// return a value.
static DWORD WINAPI
really_start (void* x)
{
  struct starter *info = (struct starter *) x;

  _Jv_ThreadRegister (info->data);

  info->method (info->data->thread_obj);

  if (! (info->data->flags & FLAG_DAEMON))
    {
      WaitForSingleObject (daemon_mutex, INFINITE);
      non_daemon_count--;
      if (! non_daemon_count)
          PulseEvent (daemon_cond);
      ReleaseMutex (daemon_mutex);
    }

  return 0;
}

void
_Jv_ThreadStart (java::lang::Thread *thread, _Jv_Thread_t *data, _Jv_ThreadStartFunc *meth)
{
  DWORD id;
  struct starter *info;

  // Do nothing if thread has already started
  if (data->flags & FLAG_START)
    return;
  data->flags |= FLAG_START;

  // FIXME: handle marking the info object for GC.
  info = (struct starter *) _Jv_AllocBytes (sizeof (struct starter));
  info->method = meth;
  info->data = data;

  if (! thread->isDaemon ())
    {
      WaitForSingleObject (daemon_mutex, INFINITE);
      non_daemon_count++;
      ReleaseMutex (daemon_mutex);
    }
  else
    data->flags |= FLAG_DAEMON;

  HANDLE h = GC_CreateThread(NULL, 0, really_start, info, 0, &id);
  _Jv_ThreadSetPriority(data, thread->getPriority());

  //if (!h)
    //JvThrow ();
}

void
_Jv_ThreadWait (void)
{
  WaitForSingleObject(daemon_mutex, INFINITE);
  if(non_daemon_count)
      SignalObjectAndWait(daemon_mutex, daemon_cond, INFINITE, 0);
  ReleaseMutex(daemon_mutex);
}

void
_Jv_ThreadInterrupt (_Jv_Thread_t *data)
{
  MessageBox(NULL, "Unimplemented", "win32-threads.cc:_Jv_ThreadInterrupt", MB_OK);
  // FIXME:
}
