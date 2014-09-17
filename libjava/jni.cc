// jni.cc - JNI implementation, including the jump table.

/* Copyright (C) 1998, 1999, 2000, 2001, 2002  Free Software Foundation

   This file is part of libgcj.

This software is copyrighted work licensed under the terms of the
Libgcj License.  Please consult the file "LIBGCJ_LICENSE" for
details.  */

#include <config.h>

#include <stddef.h>
#include <string.h>

#include <gcj/cni.h>
#include <jvm.h>
#include <java-assert.h>
#include <jni.h>
#ifdef ENABLE_JVMPI
#include <jvmpi.h>
#endif

#include <java/lang/Class.h>
#include <java/lang/ClassLoader.h>
#include <java/lang/Throwable.h>
#include <java/lang/ArrayIndexOutOfBoundsException.h>
#include <java/lang/StringIndexOutOfBoundsException.h>
#include <java/lang/UnsatisfiedLinkError.h>
#include <java/lang/InstantiationException.h>
#include <java/lang/NoSuchFieldError.h>
#include <java/lang/NoSuchMethodError.h>
#include <java/lang/reflect/Constructor.h>
#include <java/lang/reflect/Method.h>
#include <java/lang/reflect/Modifier.h>
#include <java/lang/OutOfMemoryError.h>
#include <java/util/IdentityHashMap.h>
#include <java/lang/Integer.h>
#include <java/lang/ThreadGroup.h>
#include <java/lang/Thread.h>

#include <gcj/method.h>
#include <gcj/field.h>

#include <java-interp.h>
#include <java-threads.h>

using namespace gcj;

// This enum is used to select different template instantiations in
// the invocation code.
enum invocation_type
{
  normal,
  nonvirtual,
  static_type,
  constructor
};

// Forward declarations.
extern struct JNINativeInterface _Jv_JNIFunctions;
extern struct JNIInvokeInterface _Jv_JNI_InvokeFunctions;

// Number of slots in the default frame.  The VM must allow at least
// 16.
#define FRAME_SIZE 32

// Mark value indicating this is an overflow frame.
#define MARK_NONE    0
// Mark value indicating this is a user frame.
#define MARK_USER    1
// Mark value indicating this is a system frame.
#define MARK_SYSTEM  2

// This structure is used to keep track of local references.
struct _Jv_JNI_LocalFrame
{
  // This is true if this frame object represents a pushed frame (eg
  // from PushLocalFrame).
  int marker :  2;

  // Number of elements in frame.
  int size   : 30;

  // Next frame in chain.
  _Jv_JNI_LocalFrame *next;

  // The elements.  These are allocated using the C "struct hack".
  jobject vec[0];
};

// This holds a reference count for all local references.
static java::util::IdentityHashMap *local_ref_table;
// This holds a reference count for all global references.
static java::util::IdentityHashMap *global_ref_table;

// The only VM.
static JavaVM *the_vm;

#ifdef ENABLE_JVMPI
// The only JVMPI interface description.
static JVMPI_Interface _Jv_JVMPI_Interface;

static jint
jvmpiEnableEvent (jint event_type, void *)
{
  switch (event_type)
    {
    case JVMPI_EVENT_OBJECT_ALLOC:
      _Jv_JVMPI_Notify_OBJECT_ALLOC = _Jv_JVMPI_Interface.NotifyEvent;
      break;

    case JVMPI_EVENT_THREAD_START:
      _Jv_JVMPI_Notify_THREAD_START = _Jv_JVMPI_Interface.NotifyEvent;
      break;

    case JVMPI_EVENT_THREAD_END:
      _Jv_JVMPI_Notify_THREAD_END = _Jv_JVMPI_Interface.NotifyEvent;
      break;

    default:
      return JVMPI_NOT_AVAILABLE;
    }

  return JVMPI_SUCCESS;
}

static jint
jvmpiDisableEvent (jint event_type, void *)
{
  switch (event_type)
    {
    case JVMPI_EVENT_OBJECT_ALLOC:
      _Jv_JVMPI_Notify_OBJECT_ALLOC = NULL;
      break;

    default:
      return JVMPI_NOT_AVAILABLE;
    }

  return JVMPI_SUCCESS;
}
#endif



void
_Jv_JNI_Init (void)
{
  local_ref_table = new java::util::IdentityHashMap;
  global_ref_table = new java::util::IdentityHashMap;

#ifdef ENABLE_JVMPI
  _Jv_JVMPI_Interface.version = 1;
  _Jv_JVMPI_Interface.EnableEvent = &jvmpiEnableEvent;
  _Jv_JVMPI_Interface.DisableEvent = &jvmpiDisableEvent;
  _Jv_JVMPI_Interface.EnableGC = &_Jv_EnableGC;
  _Jv_JVMPI_Interface.DisableGC = &_Jv_DisableGC;
  _Jv_JVMPI_Interface.RunGC = &_Jv_RunGC;
#endif
}

// Tell the GC that a certain pointer is live.
static void
mark_for_gc (jobject obj, java::util::IdentityHashMap *ref_table)
{
  JvSynchronize sync (ref_table);

  using namespace java::lang;
  Integer *refcount = (Integer *) ref_table->get (obj);
  jint val = (refcount == NULL) ? 0 : refcount->intValue ();
  // FIXME: what about out of memory error?
  ref_table->put (obj, new Integer (val + 1));
}

// Unmark a pointer.
static void
unmark_for_gc (jobject obj, java::util::IdentityHashMap *ref_table)
{
  JvSynchronize sync (ref_table);

  using namespace java::lang;
  Integer *refcount = (Integer *) ref_table->get (obj);
  JvAssert (refcount);
  jint val = refcount->intValue () - 1;
  JvAssert (val >= 0);
  if (val == 0)
    ref_table->remove (obj);
  else
    // FIXME: what about out of memory error?
    ref_table->put (obj, new Integer (val));
}

// "Unwrap" some random non-reference type.  This exists to simplify
// other template functions.
template<typename T>
static T
unwrap (T val)
{
  return val;
}

// Unwrap a weak reference, if required.
template<typename T>
static T *
unwrap (T *obj)
{
  using namespace gnu::gcj::runtime;
  // We can compare the class directly because JNIWeakRef is `final'.
  // Doing it this way is much faster.
  if (obj == NULL || obj->getClass () != &JNIWeakRef::class$)
    return obj;
  JNIWeakRef *wr = reinterpret_cast<JNIWeakRef *> (obj);
  return reinterpret_cast<T *> (wr->get ());
}



static jobject
_Jv_JNI_NewGlobalRef (JNIEnv *, jobject obj)
{
  // This seems weird but I think it is correct.
  obj = unwrap (obj);
  mark_for_gc (obj, global_ref_table);
  return obj;
}

static void
_Jv_JNI_DeleteGlobalRef (JNIEnv *, jobject obj)
{
  // This seems weird but I think it is correct.
  obj = unwrap (obj);
  unmark_for_gc (obj, global_ref_table);
}

static void
_Jv_JNI_DeleteLocalRef (JNIEnv *env, jobject obj)
{
  _Jv_JNI_LocalFrame *frame;

  // This seems weird but I think it is correct.
  obj = unwrap (obj);

  for (frame = env->locals; frame != NULL; frame = frame->next)
    {
      for (int i = 0; i < frame->size; ++i)
	{
	  if (frame->vec[i] == obj)
	    {
	      frame->vec[i] = NULL;
	      unmark_for_gc (obj, local_ref_table);
	      return;
	    }
	}

      // Don't go past a marked frame.
      JvAssert (frame->marker == MARK_NONE);
    }

  JvAssert (0);
}

static jint
_Jv_JNI_EnsureLocalCapacity (JNIEnv *env, jint size)
{
  // It is easier to just always allocate a new frame of the requested
  // size.  This isn't the most efficient thing, but for now we don't
  // care.  Note that _Jv_JNI_PushLocalFrame relies on this right now.

  _Jv_JNI_LocalFrame *frame;
  try
    {
      frame = (_Jv_JNI_LocalFrame *) _Jv_Malloc (sizeof (_Jv_JNI_LocalFrame)
						 + size * sizeof (jobject));
    }
  catch (jthrowable t)
    {
      env->ex = t;
      return JNI_ERR;
    }

  frame->marker = MARK_NONE;
  frame->size = size;
  memset (&frame->vec[0], 0, size * sizeof (jobject));
  frame->next = env->locals;
  env->locals = frame;

  return 0;
}

static jint
_Jv_JNI_PushLocalFrame (JNIEnv *env, jint size)
{
  jint r = _Jv_JNI_EnsureLocalCapacity (env, size);
  if (r < 0)
    return r;

  // The new frame is on top.
  env->locals->marker = MARK_USER;

  return 0;
}

static jobject
_Jv_JNI_NewLocalRef (JNIEnv *env, jobject obj)
{
  // This seems weird but I think it is correct.
  obj = unwrap (obj);

  // Try to find an open slot somewhere in the topmost frame.
  _Jv_JNI_LocalFrame *frame = env->locals;
  bool done = false, set = false;
  for (; frame != NULL && ! done; frame = frame->next)
    {
      for (int i = 0; i < frame->size; ++i)
	{
	  if (frame->vec[i] == NULL)
	    {
	      set = true;
	      done = true;
	      frame->vec[i] = obj;
	      break;
	    }
	}

      // If we found a slot, or if the frame we just searched is the
      // mark frame, then we are done.
      if (done || frame == NULL || frame->marker != MARK_NONE)
	break;
    }

  if (! set)
    {
      // No slots, so we allocate a new frame.  According to the spec
      // we could just die here.  FIXME: return value.
      _Jv_JNI_EnsureLocalCapacity (env, 16);
      // We know the first element of the new frame will be ok.
      env->locals->vec[0] = obj;
    }

  mark_for_gc (obj, local_ref_table);
  return obj;
}

static jobject
_Jv_JNI_PopLocalFrame (JNIEnv *env, jobject result, int stop)
{
  _Jv_JNI_LocalFrame *rf = env->locals;

  bool done = false;
  while (rf != NULL && ! done)
    {
      for (int i = 0; i < rf->size; ++i)
	if (rf->vec[i] != NULL)
	  unmark_for_gc (rf->vec[i], local_ref_table);

      // If the frame we just freed is the marker frame, we are done.
      done = (rf->marker == stop);

      _Jv_JNI_LocalFrame *n = rf->next;
      // When N==NULL, we've reached the stack-allocated frame, and we
      // must not free it.  However, we must be sure to clear all its
      // elements, since we might conceivably reuse it.
      if (n == NULL)
	{
	  memset (&rf->vec[0], 0, rf->size * sizeof (jobject));
	  break;
	}

      _Jv_Free (rf);
      rf = n;
    }

  // Update the local frame information.
  env->locals = rf;

  return result == NULL ? NULL : _Jv_JNI_NewLocalRef (env, result);
}

static jobject
_Jv_JNI_PopLocalFrame (JNIEnv *env, jobject result)
{
  return _Jv_JNI_PopLocalFrame (env, result, MARK_USER);
}

// Pop a `system' frame from the stack.  This is `extern "C"' as it is
// used by the compiler.
extern "C" void
_Jv_JNI_PopSystemFrame (JNIEnv *env)
{
  _Jv_JNI_PopLocalFrame (env, NULL, MARK_SYSTEM);

  if (env->ex)
    {
      jthrowable t = env->ex;
      env->ex = NULL;
      throw t;
    }
}

// This function is used from other template functions.  It wraps the
// return value appropriately; we specialize it so that object returns
// are turned into local references.
template<typename T>
static T
wrap_value (JNIEnv *, T value)
{
  return value;
}

// This specialization is used for jobject, jclass, jstring, jarray,
// etc.
template<typename T>
static T *
wrap_value (JNIEnv *env, T *value)
{
  return (value == NULL
	  ? value
	  : (T *) _Jv_JNI_NewLocalRef (env, (jobject) value));
}



static jint
_Jv_JNI_GetVersion (JNIEnv *)
{
  return JNI_VERSION_1_4;
}

static jclass
_Jv_JNI_DefineClass (JNIEnv *env, jobject loader,
		     const jbyte *buf, jsize bufLen)
{
  try
    {
      loader = unwrap (loader);

      jbyteArray bytes = JvNewByteArray (bufLen);

      jbyte *elts = elements (bytes);
      memcpy (elts, buf, bufLen * sizeof (jbyte));

      java::lang::ClassLoader *l
	= reinterpret_cast<java::lang::ClassLoader *> (loader);

      jclass result = l->defineClass (bytes, 0, bufLen);
      return (jclass) wrap_value (env, result);
    }
  catch (jthrowable t)
    {
      env->ex = t;
      return NULL;
    }
}

static jclass
_Jv_JNI_FindClass (JNIEnv *env, const char *name)
{
  // FIXME: assume that NAME isn't too long.
  int len = strlen (name);
  char s[len + 1];
  for (int i = 0; i <= len; ++i)
    s[i] = (name[i] == '/') ? '.' : name[i];

  jclass r = NULL;
  try
    {
      // This might throw an out of memory exception.
      jstring n = JvNewStringUTF (s);

      java::lang::ClassLoader *loader = NULL;
      if (env->klass != NULL)
	loader = env->klass->getClassLoader ();

      if (loader == NULL)
	{
	  // FIXME: should use getBaseClassLoader, but we don't have that
	  // yet.
	  loader = java::lang::ClassLoader::getSystemClassLoader ();
	}

      r = loader->loadClass (n);
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }

  return (jclass) wrap_value (env, r);
}

static jclass
_Jv_JNI_GetSuperclass (JNIEnv *env, jclass clazz)
{
  return (jclass) wrap_value (env, unwrap (clazz)->getSuperclass ());
}

static jboolean
_Jv_JNI_IsAssignableFrom(JNIEnv *, jclass clazz1, jclass clazz2)
{
  return unwrap (clazz1)->isAssignableFrom (unwrap (clazz2));
}

static jint
_Jv_JNI_Throw (JNIEnv *env, jthrowable obj)
{
  // We check in case the user did some funky cast.
  obj = unwrap (obj);
  JvAssert (obj != NULL && java::lang::Throwable::class$.isInstance (obj));
  env->ex = obj;
  return 0;
}

static jint
_Jv_JNI_ThrowNew (JNIEnv *env, jclass clazz, const char *message)
{
  using namespace java::lang::reflect;

  clazz = unwrap (clazz);
  JvAssert (java::lang::Throwable::class$.isAssignableFrom (clazz));

  int r = JNI_OK;
  try
    {
      JArray<jclass> *argtypes
	= (JArray<jclass> *) JvNewObjectArray (1, &java::lang::Class::class$,
					       NULL);

      jclass *elts = elements (argtypes);
      elts[0] = &StringClass;

      Constructor *cons = clazz->getConstructor (argtypes);

      jobjectArray values = JvNewObjectArray (1, &StringClass, NULL);
      jobject *velts = elements (values);
      velts[0] = JvNewStringUTF (message);

      jobject obj = cons->newInstance (values);

      env->ex = reinterpret_cast<jthrowable> (obj);
    }
  catch (jthrowable t)
    {
      env->ex = t;
      r = JNI_ERR;
    }

  return r;
}

static jthrowable
_Jv_JNI_ExceptionOccurred (JNIEnv *env)
{
  return (jthrowable) wrap_value (env, env->ex);
}

static void
_Jv_JNI_ExceptionDescribe (JNIEnv *env)
{
  if (env->ex != NULL)
    env->ex->printStackTrace();
}

static void
_Jv_JNI_ExceptionClear (JNIEnv *env)
{
  env->ex = NULL;
}

static jboolean
_Jv_JNI_ExceptionCheck (JNIEnv *env)
{
  return env->ex != NULL;
}

static void
_Jv_JNI_FatalError (JNIEnv *, const char *message)
{
  JvFail (message);
}



static jboolean
_Jv_JNI_IsSameObject (JNIEnv *, jobject obj1, jobject obj2)
{
  return unwrap (obj1) == unwrap (obj2);
}

static jobject
_Jv_JNI_AllocObject (JNIEnv *env, jclass clazz)
{
  jobject obj = NULL;
  using namespace java::lang::reflect;

  try
    {
      clazz = unwrap (clazz);
      JvAssert (clazz && ! clazz->isArray ());
      if (clazz->isInterface() || Modifier::isAbstract(clazz->getModifiers()))
	env->ex = new java::lang::InstantiationException ();
      else
	{
	  // FIXME: will this work for String?
	  obj = JvAllocObject (clazz);
	}
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }

  return wrap_value (env, obj);
}

static jclass
_Jv_JNI_GetObjectClass (JNIEnv *env, jobject obj)
{
  obj = unwrap (obj);
  JvAssert (obj);
  return (jclass) wrap_value (env, obj->getClass());
}

static jboolean
_Jv_JNI_IsInstanceOf (JNIEnv *, jobject obj, jclass clazz)
{
  return unwrap (clazz)->isInstance(unwrap (obj));
}



//
// This section concerns method invocation.
//

template<jboolean is_static>
static jmethodID
_Jv_JNI_GetAnyMethodID (JNIEnv *env, jclass clazz,
			const char *name, const char *sig)
{
  try
    {
      clazz = unwrap (clazz);
      _Jv_InitClass (clazz);

      _Jv_Utf8Const *name_u = _Jv_makeUtf8Const ((char *) name, -1);

      // FIXME: assume that SIG isn't too long.
      int len = strlen (sig);
      char s[len + 1];
      for (int i = 0; i <= len; ++i)
	s[i] = (sig[i] == '/') ? '.' : sig[i];
      _Jv_Utf8Const *sig_u = _Jv_makeUtf8Const ((char *) s, -1);

      JvAssert (! clazz->isPrimitive());

      using namespace java::lang::reflect;

      while (clazz != NULL)
	{
	  jint count = JvNumMethods (clazz);
	  jmethodID meth = JvGetFirstMethod (clazz);

	  for (jint i = 0; i < count; ++i)
	    {
	      if (((is_static && Modifier::isStatic (meth->accflags))
		   || (! is_static && ! Modifier::isStatic (meth->accflags)))
		  && _Jv_equalUtf8Consts (meth->name, name_u)
		  && _Jv_equalUtf8Consts (meth->signature, sig_u))
		return meth;

	      meth = meth->getNextMethod();
	    }

	  clazz = clazz->getSuperclass ();
	}

      env->ex = new java::lang::NoSuchMethodError ();
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }

  return NULL;
}

// This is a helper function which turns a va_list into an array of
// `jvalue's.  It needs signature information in order to do its work.
// The array of values must already be allocated.
static void
array_from_valist (jvalue *values, JArray<jclass> *arg_types, va_list vargs)
{
  jclass *arg_elts = elements (arg_types);
  for (int i = 0; i < arg_types->length; ++i)
    {
      if (arg_elts[i] == JvPrimClass (byte))
	values[i].b = (jbyte) va_arg (vargs, int);
      else if (arg_elts[i] == JvPrimClass (short))
	values[i].s = (jshort) va_arg (vargs, int);
      else if (arg_elts[i] == JvPrimClass (int))
	values[i].i = va_arg (vargs, jint);
      else if (arg_elts[i] == JvPrimClass (long))
	values[i].j = va_arg (vargs, jlong);
      else if (arg_elts[i] == JvPrimClass (float))
	values[i].f = va_arg (vargs, jfloat);
      else if (arg_elts[i] == JvPrimClass (double))
	values[i].d = va_arg (vargs, jdouble);
      else if (arg_elts[i] == JvPrimClass (boolean))
	values[i].z = (jboolean) va_arg (vargs, int);
      else if (arg_elts[i] == JvPrimClass (char))
	values[i].c = (jchar) va_arg (vargs, int);
      else
	{
	  // An object.
	  values[i].l = unwrap (va_arg (vargs, jobject));
	}
    }
}

// This can call any sort of method: virtual, "nonvirtual", static, or
// constructor.
template<typename T, invocation_type style>
static T
_Jv_JNI_CallAnyMethodV (JNIEnv *env, jobject obj, jclass klass,
			jmethodID id, va_list vargs)
{
  obj = unwrap (obj);
  klass = unwrap (klass);

  if (style == normal)
    id = _Jv_LookupDeclaredMethod (obj->getClass (), id->name, id->signature);

  jclass decl_class = klass ? klass : obj->getClass ();
  JvAssert (decl_class != NULL);

  jclass return_type;
  JArray<jclass> *arg_types;

  try
    {
      _Jv_GetTypesFromSignature (id, decl_class,
				 &arg_types, &return_type);

      jvalue args[arg_types->length];
      array_from_valist (args, arg_types, vargs);

      // For constructors we need to pass the Class we are instantiating.
      if (style == constructor)
	return_type = klass;

      jvalue result;
      jthrowable ex = _Jv_CallAnyMethodA (obj, return_type, id,
					  style == constructor,
					  arg_types, args, &result);

      if (ex != NULL)
	env->ex = ex;

      // We cheat a little here.  FIXME.
      return wrap_value (env, * (T *) &result);
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }

  return wrap_value (env, (T) 0);
}

template<typename T, invocation_type style>
static T
_Jv_JNI_CallAnyMethod (JNIEnv *env, jobject obj, jclass klass,
		       jmethodID method, ...)
{
  va_list args;
  T result;

  va_start (args, method);
  result = _Jv_JNI_CallAnyMethodV<T, style> (env, obj, klass, method, args);
  va_end (args);

  return result;
}

template<typename T, invocation_type style>
static T
_Jv_JNI_CallAnyMethodA (JNIEnv *env, jobject obj, jclass klass,
			jmethodID id, jvalue *args)
{
  obj = unwrap (obj);
  klass = unwrap (klass);

  if (style == normal)
    id = _Jv_LookupDeclaredMethod (obj->getClass (), id->name, id->signature);

  jclass decl_class = klass ? klass : obj->getClass ();
  JvAssert (decl_class != NULL);

  jclass return_type;
  JArray<jclass> *arg_types;
  try
    {
      _Jv_GetTypesFromSignature (id, decl_class,
				 &arg_types, &return_type);

      // For constructors we need to pass the Class we are instantiating.
      if (style == constructor)
	return_type = klass;

      // Unwrap arguments as required.  Eww.
      jclass *type_elts = elements (arg_types);
      jvalue arg_copy[arg_types->length];
      for (int i = 0; i < arg_types->length; ++i)
	{
	  if (type_elts[i]->isPrimitive ())
	    arg_copy[i] = args[i];
	  else
	    arg_copy[i].l = unwrap (args[i].l);
	}

      jvalue result;
      jthrowable ex = _Jv_CallAnyMethodA (obj, return_type, id,
					  style == constructor,
					  arg_types, arg_copy, &result);

      if (ex != NULL)
	env->ex = ex;

      // We cheat a little here.  FIXME.
      return wrap_value (env, * (T *) &result);
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }

  return wrap_value (env, (T) 0);
}

template<invocation_type style>
static void
_Jv_JNI_CallAnyVoidMethodV (JNIEnv *env, jobject obj, jclass klass,
			    jmethodID id, va_list vargs)
{
  obj = unwrap (obj);
  klass = unwrap (klass);

  if (style == normal)
    id = _Jv_LookupDeclaredMethod (obj->getClass (), id->name, id->signature);

  jclass decl_class = klass ? klass : obj->getClass ();
  JvAssert (decl_class != NULL);

  jclass return_type;
  JArray<jclass> *arg_types;
  try
    {
      _Jv_GetTypesFromSignature (id, decl_class,
				 &arg_types, &return_type);

      jvalue args[arg_types->length];
      array_from_valist (args, arg_types, vargs);

      // For constructors we need to pass the Class we are instantiating.
      if (style == constructor)
	return_type = klass;

      jthrowable ex = _Jv_CallAnyMethodA (obj, return_type, id,
					  style == constructor,
					  arg_types, args, NULL);

      if (ex != NULL)
	env->ex = ex;
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }
}

template<invocation_type style>
static void
_Jv_JNI_CallAnyVoidMethod (JNIEnv *env, jobject obj, jclass klass,
			   jmethodID method, ...)
{
  va_list args;

  va_start (args, method);
  _Jv_JNI_CallAnyVoidMethodV<style> (env, obj, klass, method, args);
  va_end (args);
}

template<invocation_type style>
static void
_Jv_JNI_CallAnyVoidMethodA (JNIEnv *env, jobject obj, jclass klass,
			    jmethodID id, jvalue *args)
{
  if (style == normal)
    id = _Jv_LookupDeclaredMethod (obj->getClass (), id->name, id->signature);

  jclass decl_class = klass ? klass : obj->getClass ();
  JvAssert (decl_class != NULL);

  jclass return_type;
  JArray<jclass> *arg_types;
  try
    {
      _Jv_GetTypesFromSignature (id, decl_class,
				 &arg_types, &return_type);

      // Unwrap arguments as required.  Eww.
      jclass *type_elts = elements (arg_types);
      jvalue arg_copy[arg_types->length];
      for (int i = 0; i < arg_types->length; ++i)
	{
	  if (type_elts[i]->isPrimitive ())
	    arg_copy[i] = args[i];
	  else
	    arg_copy[i].l = unwrap (args[i].l);
	}

      jthrowable ex = _Jv_CallAnyMethodA (obj, return_type, id,
					  style == constructor,
					  arg_types, args, NULL);

      if (ex != NULL)
	env->ex = ex;
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }
}

// Functions with this signature are used to implement functions in
// the CallMethod family.
template<typename T>
static T
_Jv_JNI_CallMethodV (JNIEnv *env, jobject obj, jmethodID id, va_list args)
{
  return _Jv_JNI_CallAnyMethodV<T, normal> (env, obj, NULL, id, args);
}

// Functions with this signature are used to implement functions in
// the CallMethod family.
template<typename T>
static T
_Jv_JNI_CallMethod (JNIEnv *env, jobject obj, jmethodID id, ...)
{
  va_list args;
  T result;

  va_start (args, id);
  result = _Jv_JNI_CallAnyMethodV<T, normal> (env, obj, NULL, id, args);
  va_end (args);

  return result;
}

// Functions with this signature are used to implement functions in
// the CallMethod family.
template<typename T>
static T
_Jv_JNI_CallMethodA (JNIEnv *env, jobject obj, jmethodID id, jvalue *args)
{
  return _Jv_JNI_CallAnyMethodA<T, normal> (env, obj, NULL, id, args);
}

static void
_Jv_JNI_CallVoidMethodV (JNIEnv *env, jobject obj, jmethodID id, va_list args)
{
  _Jv_JNI_CallAnyVoidMethodV<normal> (env, obj, NULL, id, args);
}

static void
_Jv_JNI_CallVoidMethod (JNIEnv *env, jobject obj, jmethodID id, ...)
{
  va_list args;

  va_start (args, id);
  _Jv_JNI_CallAnyVoidMethodV<normal> (env, obj, NULL, id, args);
  va_end (args);
}

static void
_Jv_JNI_CallVoidMethodA (JNIEnv *env, jobject obj, jmethodID id, jvalue *args)
{
  _Jv_JNI_CallAnyVoidMethodA<normal> (env, obj, NULL, id, args);
}

// Functions with this signature are used to implement functions in
// the CallStaticMethod family.
template<typename T>
static T
_Jv_JNI_CallStaticMethodV (JNIEnv *env, jclass klass,
			   jmethodID id, va_list args)
{
  JvAssert (((id->accflags) & java::lang::reflect::Modifier::STATIC));
  JvAssert (java::lang::Class::class$.isInstance (unwrap (klass)));

  return _Jv_JNI_CallAnyMethodV<T, static_type> (env, NULL, klass, id, args);
}

// Functions with this signature are used to implement functions in
// the CallStaticMethod family.
template<typename T>
static T
_Jv_JNI_CallStaticMethod (JNIEnv *env, jclass klass, jmethodID id, ...)
{
  va_list args;
  T result;

  JvAssert (((id->accflags) & java::lang::reflect::Modifier::STATIC));
  JvAssert (java::lang::Class::class$.isInstance (unwrap (klass)));

  va_start (args, id);
  result = _Jv_JNI_CallAnyMethodV<T, static_type> (env, NULL, klass,
						   id, args);
  va_end (args);

  return result;
}

// Functions with this signature are used to implement functions in
// the CallStaticMethod family.
template<typename T>
static T
_Jv_JNI_CallStaticMethodA (JNIEnv *env, jclass klass, jmethodID id,
			   jvalue *args)
{
  JvAssert (((id->accflags) & java::lang::reflect::Modifier::STATIC));
  JvAssert (java::lang::Class::class$.isInstance (unwrap (klass)));

  return _Jv_JNI_CallAnyMethodA<T, static_type> (env, NULL, klass, id, args);
}

static void
_Jv_JNI_CallStaticVoidMethodV (JNIEnv *env, jclass klass, jmethodID id,
			       va_list args)
{
  _Jv_JNI_CallAnyVoidMethodV<static_type> (env, NULL, klass, id, args);
}

static void
_Jv_JNI_CallStaticVoidMethod (JNIEnv *env, jclass klass, jmethodID id, ...)
{
  va_list args;

  va_start (args, id);
  _Jv_JNI_CallAnyVoidMethodV<static_type> (env, NULL, klass, id, args);
  va_end (args);
}

static void
_Jv_JNI_CallStaticVoidMethodA (JNIEnv *env, jclass klass, jmethodID id,
			       jvalue *args)
{
  _Jv_JNI_CallAnyVoidMethodA<static_type> (env, NULL, klass, id, args);
}

static jobject
_Jv_JNI_NewObjectV (JNIEnv *env, jclass klass,
		    jmethodID id, va_list args)
{
  JvAssert (klass && ! klass->isArray ());
  JvAssert (! strcmp (id->name->data, "<init>")
	    && id->signature->length > 2
	    && id->signature->data[0] == '('
	    && ! strcmp (&id->signature->data[id->signature->length - 2],
			 ")V"));

  return _Jv_JNI_CallAnyMethodV<jobject, constructor> (env, NULL, klass,
						       id, args);
}

static jobject
_Jv_JNI_NewObject (JNIEnv *env, jclass klass, jmethodID id, ...)
{
  JvAssert (klass && ! klass->isArray ());
  JvAssert (! strcmp (id->name->data, "<init>")
	    && id->signature->length > 2
	    && id->signature->data[0] == '('
	    && ! strcmp (&id->signature->data[id->signature->length - 2],
			 ")V"));

  va_list args;
  jobject result;

  va_start (args, id);
  result = _Jv_JNI_CallAnyMethodV<jobject, constructor> (env, NULL, klass,
							 id, args);
  va_end (args);

  return result;
}

static jobject
_Jv_JNI_NewObjectA (JNIEnv *env, jclass klass, jmethodID id,
		    jvalue *args)
{
  JvAssert (klass && ! klass->isArray ());
  JvAssert (! strcmp (id->name->data, "<init>")
	    && id->signature->length > 2
	    && id->signature->data[0] == '('
	    && ! strcmp (&id->signature->data[id->signature->length - 2],
			 ")V"));

  return _Jv_JNI_CallAnyMethodA<jobject, constructor> (env, NULL, klass,
						       id, args);
}



template<typename T>
static T
_Jv_JNI_GetField (JNIEnv *env, jobject obj, jfieldID field)
{
  obj = unwrap (obj);
  JvAssert (obj);
  T *ptr = (T *) ((char *) obj + field->getOffset ());
  return wrap_value (env, *ptr);
}

template<typename T>
static void
_Jv_JNI_SetField (JNIEnv *, jobject obj, jfieldID field, T value)
{
  obj = unwrap (obj);
  value = unwrap (value);

  JvAssert (obj);
  T *ptr = (T *) ((char *) obj + field->getOffset ());
  *ptr = value;
}

template<jboolean is_static>
static jfieldID
_Jv_JNI_GetAnyFieldID (JNIEnv *env, jclass clazz,
		       const char *name, const char *sig)
{
  try
    {
      clazz = unwrap (clazz);

      _Jv_InitClass (clazz);

      _Jv_Utf8Const *a_name = _Jv_makeUtf8Const ((char *) name, -1);

      // FIXME: assume that SIG isn't too long.
      int len = strlen (sig);
      char s[len + 1];
      for (int i = 0; i <= len; ++i)
	s[i] = (sig[i] == '/') ? '.' : sig[i];
      jclass field_class = _Jv_FindClassFromSignature ((char *) s, NULL);

      // FIXME: what if field_class == NULL?

      java::lang::ClassLoader *loader = clazz->getClassLoader ();
      while (clazz != NULL)
	{
	  // We acquire the class lock so that fields aren't resolved
	  // while we are running.
	  JvSynchronize sync (clazz);

	  jint count = (is_static
			? JvNumStaticFields (clazz)
			: JvNumInstanceFields (clazz));
	  jfieldID field = (is_static
			    ? JvGetFirstStaticField (clazz)
			    : JvGetFirstInstanceField (clazz));
	  for (jint i = 0; i < count; ++i)
	    {
	      _Jv_Utf8Const *f_name = field->getNameUtf8Const(clazz);

	      // The field might be resolved or it might not be.  It
	      // is much simpler to always resolve it.
	      _Jv_ResolveField (field, loader);
	      if (_Jv_equalUtf8Consts (f_name, a_name)
		  && field->getClass() == field_class)
		return field;

	      field = field->getNextField ();
	    }

	  clazz = clazz->getSuperclass ();
	}

      env->ex = new java::lang::NoSuchFieldError ();
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }
  return NULL;
}

template<typename T>
static T
_Jv_JNI_GetStaticField (JNIEnv *env, jclass, jfieldID field)
{
  T *ptr = (T *) field->u.addr;
  return wrap_value (env, *ptr);
}

template<typename T>
static void
_Jv_JNI_SetStaticField (JNIEnv *, jclass, jfieldID field, T value)
{
  value = unwrap (value);
  T *ptr = (T *) field->u.addr;
  *ptr = value;
}

static jstring
_Jv_JNI_NewString (JNIEnv *env, const jchar *unichars, jsize len)
{
  try
    {
      jstring r = _Jv_NewString (unichars, len);
      return (jstring) wrap_value (env, r);
    }
  catch (jthrowable t)
    {
      env->ex = t;
      return NULL;
    }
}

static jsize
_Jv_JNI_GetStringLength (JNIEnv *, jstring string)
{
  return unwrap (string)->length();
}

static const jchar *
_Jv_JNI_GetStringChars (JNIEnv *, jstring string, jboolean *isCopy)
{
  string = unwrap (string);
  jchar *result = _Jv_GetStringChars (string);
  mark_for_gc (string, global_ref_table);
  if (isCopy)
    *isCopy = false;
  return (const jchar *) result;
}

static void
_Jv_JNI_ReleaseStringChars (JNIEnv *, jstring string, const jchar *)
{
  unmark_for_gc (unwrap (string), global_ref_table);
}

static jstring
_Jv_JNI_NewStringUTF (JNIEnv *env, const char *bytes)
{
  try
    {
      jstring result = JvNewStringUTF (bytes);
      return (jstring) wrap_value (env, result);
    }
  catch (jthrowable t)
    {
      env->ex = t;
      return NULL;
    }
}

static jsize
_Jv_JNI_GetStringUTFLength (JNIEnv *, jstring string)
{
  return JvGetStringUTFLength (unwrap (string));
}

static const char *
_Jv_JNI_GetStringUTFChars (JNIEnv *env, jstring string, jboolean *isCopy)
{
  string = unwrap (string);
  jsize len = JvGetStringUTFLength (string);
  try
    {
      char *r = (char *) _Jv_Malloc (len + 1);
      JvGetStringUTFRegion (string, 0, len, r);
      r[len] = '\0';

      if (isCopy)
	*isCopy = true;

      return (const char *) r;
    }
  catch (jthrowable t)
    {
      env->ex = t;
      return NULL;
    }
}

static void
_Jv_JNI_ReleaseStringUTFChars (JNIEnv *, jstring, const char *utf)
{
  _Jv_Free ((void *) utf);
}

static void
_Jv_JNI_GetStringRegion (JNIEnv *env, jstring string, jsize start, jsize len,
			 jchar *buf)
{
  string = unwrap (string);
  jchar *result = _Jv_GetStringChars (string);
  if (start < 0 || start > string->length ()
      || len < 0 || start + len > string->length ())
    {
      try
	{
	  env->ex = new java::lang::StringIndexOutOfBoundsException ();
	}
      catch (jthrowable t)
	{
	  env->ex = t;
	}
    }
  else
    memcpy (buf, &result[start], len * sizeof (jchar));
}

static void
_Jv_JNI_GetStringUTFRegion (JNIEnv *env, jstring str, jsize start,
			    jsize len, char *buf)
{
  str = unwrap (str);
    
  if (start < 0 || start > str->length ()
      || len < 0 || start + len > str->length ())
    {
      try
	{
	  env->ex = new java::lang::StringIndexOutOfBoundsException ();
	}
      catch (jthrowable t)
	{
	  env->ex = t;
	}
    }
  else
    _Jv_GetStringUTFRegion (str, start, len, buf);
}

static const jchar *
_Jv_JNI_GetStringCritical (JNIEnv *, jstring str, jboolean *isCopy)
{
  jchar *result = _Jv_GetStringChars (unwrap (str));
  if (isCopy)
    *isCopy = false;
  return result;
}

static void
_Jv_JNI_ReleaseStringCritical (JNIEnv *, jstring, const jchar *)
{
  // Nothing.
}

static jsize
_Jv_JNI_GetArrayLength (JNIEnv *, jarray array)
{
  return unwrap (array)->length;
}

static jarray
_Jv_JNI_NewObjectArray (JNIEnv *env, jsize length, jclass elementClass,
			jobject init)
{
  try
    {
      elementClass = unwrap (elementClass);
      init = unwrap (init);

      jarray result = JvNewObjectArray (length, elementClass, init);
      return (jarray) wrap_value (env, result);
    }
  catch (jthrowable t)
    {
      env->ex = t;
      return NULL;
    }
}

static jobject
_Jv_JNI_GetObjectArrayElement (JNIEnv *env, jobjectArray array, jsize index)
{
  jobject *elts = elements (unwrap (array));
  return wrap_value (env, elts[index]);
}

static void
_Jv_JNI_SetObjectArrayElement (JNIEnv *env, jobjectArray array, jsize index,
			       jobject value)
{
  try
    {
      array = unwrap (array);
      value = unwrap (value);

      _Jv_CheckArrayStore (array, value);
      jobject *elts = elements (array);
      elts[index] = value;
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }
}

template<typename T, jclass K>
static JArray<T> *
_Jv_JNI_NewPrimitiveArray (JNIEnv *env, jsize length)
{
  try
    {
      return (JArray<T> *) wrap_value (env, _Jv_NewPrimArray (K, length));
    }
  catch (jthrowable t)
    {
      env->ex = t;
      return NULL;
    }
}

template<typename T>
static T *
_Jv_JNI_GetPrimitiveArrayElements (JNIEnv *, JArray<T> *array,
				   jboolean *isCopy)
{
  array = unwrap (array);
  T *elts = elements (array);
  if (isCopy)
    {
      // We elect never to copy.
      *isCopy = false;
    }
  mark_for_gc (array, global_ref_table);
  return elts;
}

template<typename T>
static void
_Jv_JNI_ReleasePrimitiveArrayElements (JNIEnv *, JArray<T> *array,
				       T *, jint /* mode */)
{
  array = unwrap (array);
  // Note that we ignore MODE.  We can do this because we never copy
  // the array elements.  My reading of the JNI documentation is that
  // this is an option for the implementor.
  unmark_for_gc (array, global_ref_table);
}

template<typename T>
static void
_Jv_JNI_GetPrimitiveArrayRegion (JNIEnv *env, JArray<T> *array,
				 jsize start, jsize len,
				 T *buf)
{
  array = unwrap (array);

  // The cast to unsigned lets us save a comparison.
  if (start < 0 || len < 0
      || (unsigned long) (start + len) > (unsigned long) array->length)
    {
      try
	{
	  // FIXME: index.
	  env->ex = new java::lang::ArrayIndexOutOfBoundsException ();
	}
      catch (jthrowable t)
	{
	  // Could have thown out of memory error.
	  env->ex = t;
	}
    }
  else
    {
      T *elts = elements (array) + start;
      memcpy (buf, elts, len * sizeof (T));
    }
}

template<typename T>
static void
_Jv_JNI_SetPrimitiveArrayRegion (JNIEnv *env, JArray<T> *array,
				 jsize start, jsize len, T *buf)
{
  array = unwrap (array);

  // The cast to unsigned lets us save a comparison.
  if (start < 0 || len < 0
      || (unsigned long) (start + len) > (unsigned long) array->length)
    {
      try
	{
	  // FIXME: index.
	  env->ex = new java::lang::ArrayIndexOutOfBoundsException ();
	}
      catch (jthrowable t)
	{
	  env->ex = t;
	}
    }
  else
    {
      T *elts = elements (array) + start;
      memcpy (elts, buf, len * sizeof (T));
    }
}

static void *
_Jv_JNI_GetPrimitiveArrayCritical (JNIEnv *, jarray array,
				   jboolean *isCopy)
{
  array = unwrap (array);
  // FIXME: does this work?
  jclass klass = array->getClass()->getComponentType();
  JvAssert (klass->isPrimitive ());
  char *r = _Jv_GetArrayElementFromElementType (array, klass);
  if (isCopy)
    *isCopy = false;
  return r;
}

static void
_Jv_JNI_ReleasePrimitiveArrayCritical (JNIEnv *, jarray, void *, jint)
{
  // Nothing.
}

static jint
_Jv_JNI_MonitorEnter (JNIEnv *env, jobject obj)
{
  try
    {
      _Jv_MonitorEnter (unwrap (obj));
      return 0;
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }
  return JNI_ERR;
}

static jint
_Jv_JNI_MonitorExit (JNIEnv *env, jobject obj)
{
  try
    {
      _Jv_MonitorExit (unwrap (obj));
      return 0;
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }
  return JNI_ERR;
}

// JDK 1.2
jobject
_Jv_JNI_ToReflectedField (JNIEnv *env, jclass cls, jfieldID fieldID,
			  jboolean)
{
  try
    {
      cls = unwrap (cls);
      java::lang::reflect::Field *field = new java::lang::reflect::Field();
      field->declaringClass = cls;
      field->offset = (char*) fieldID - (char *) cls->fields;
      field->name = _Jv_NewStringUtf8Const (fieldID->getNameUtf8Const (cls));
      return wrap_value (env, field);
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }
  return NULL;
}

// JDK 1.2
static jfieldID
_Jv_JNI_FromReflectedField (JNIEnv *, jobject f)
{
  using namespace java::lang::reflect;

  f = unwrap (f);
  Field *field = reinterpret_cast<Field *> (f);
  return _Jv_FromReflectedField (field);
}

jobject
_Jv_JNI_ToReflectedMethod (JNIEnv *env, jclass klass, jmethodID id,
			   jboolean)
{
  using namespace java::lang::reflect;

  jobject result = NULL;
  klass = unwrap (klass);

  try
    {
      if (_Jv_equalUtf8Consts (id->name, init_name))
	{
	  // A constructor.
	  Constructor *cons = new Constructor ();
	  cons->offset = (char *) id - (char *) &klass->methods;
	  cons->declaringClass = klass;
	  result = cons;
	}
      else
	{
	  Method *meth = new Method ();
	  meth->offset = (char *) id - (char *) &klass->methods;
	  meth->declaringClass = klass;
	  result = meth;
	}
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }

  return wrap_value (env, result);
}

static jmethodID
_Jv_JNI_FromReflectedMethod (JNIEnv *, jobject method)
{
  using namespace java::lang::reflect;
  method = unwrap (method);
  if (Method::class$.isInstance (method))
    return _Jv_FromReflectedMethod (reinterpret_cast<Method *> (method));
  return
    _Jv_FromReflectedConstructor (reinterpret_cast<Constructor *> (method));
}

// JDK 1.2.
jweak
_Jv_JNI_NewWeakGlobalRef (JNIEnv *env, jobject obj)
{
  using namespace gnu::gcj::runtime;
  JNIWeakRef *ref = NULL;

  try
    {
      // This seems weird but I think it is correct.
      obj = unwrap (obj);
      ref = new JNIWeakRef (obj);
      mark_for_gc (ref, global_ref_table);
    }
  catch (jthrowable t)
    {
      env->ex = t;
    }

  return reinterpret_cast<jweak> (ref);
}

void
_Jv_JNI_DeleteWeakGlobalRef (JNIEnv *, jweak obj)
{
  using namespace gnu::gcj::runtime;
  JNIWeakRef *ref = reinterpret_cast<JNIWeakRef *> (obj);
  unmark_for_gc (ref, global_ref_table);
  ref->clear ();
}



// Direct byte buffers.

static jobject
_Jv_JNI_NewDirectByteBuffer (JNIEnv *, void *, jlong)
{
  // For now we don't support this.
  return NULL;
}

static void *
_Jv_JNI_GetDirectBufferAddress (JNIEnv *, jobject)
{
  // For now we don't support this.
  return NULL;
}

static jlong
_Jv_JNI_GetDirectBufferCapacity (JNIEnv *, jobject)
{
  // For now we don't support this.
  return -1;
}



// Hash table of native methods.
static JNINativeMethod *nathash;
// Number of slots used.
static int nathash_count = 0;
// Number of slots available.  Must be power of 2.
static int nathash_size = 0;

#define DELETED_ENTRY ((char *) (~0))

// Compute a hash value for a native method descriptor.
static int
hash (const JNINativeMethod *method)
{
  char *ptr;
  int hash = 0;

  ptr = method->name;
  while (*ptr)
    hash = (31 * hash) + *ptr++;

  ptr = method->signature;
  while (*ptr)
    hash = (31 * hash) + *ptr++;

  return hash;
}

// Find the slot where a native method goes.
static JNINativeMethod *
nathash_find_slot (const JNINativeMethod *method)
{
  jint h = hash (method);
  int step = (h ^ (h >> 16)) | 1;
  int w = h & (nathash_size - 1);
  int del = -1;

  for (;;)
    {
      JNINativeMethod *slotp = &nathash[w];
      if (slotp->name == NULL)
	{
	  if (del >= 0)
	    return &nathash[del];
	  else
	    return slotp;
	}
      else if (slotp->name == DELETED_ENTRY)
	del = w;
      else if (! strcmp (slotp->name, method->name)
	       && ! strcmp (slotp->signature, method->signature))
	return slotp;
      w = (w + step) & (nathash_size - 1);
    }
}

// Find a method.  Return NULL if it isn't in the hash table.
static void *
nathash_find (JNINativeMethod *method)
{
  if (nathash == NULL)
    return NULL;
  JNINativeMethod *slot = nathash_find_slot (method);
  if (slot->name == NULL || slot->name == DELETED_ENTRY)
    return NULL;
  return slot->fnPtr;
}

static void
natrehash ()
{
  if (nathash == NULL)
    {
      nathash_size = 1024;
      nathash =
	(JNINativeMethod *) _Jv_AllocBytes (nathash_size
					    * sizeof (JNINativeMethod));
      memset (nathash, 0, nathash_size * sizeof (JNINativeMethod));
    }
  else
    {
      int savesize = nathash_size;
      JNINativeMethod *savehash = nathash;
      nathash_size *= 2;
      nathash =
	(JNINativeMethod *) _Jv_AllocBytes (nathash_size
					    * sizeof (JNINativeMethod));
      memset (nathash, 0, nathash_size * sizeof (JNINativeMethod));

      for (int i = 0; i < savesize; ++i)
	{
	  if (savehash[i].name != NULL && savehash[i].name != DELETED_ENTRY)
	    {
	      JNINativeMethod *slot = nathash_find_slot (&savehash[i]);
	      *slot = savehash[i];
	    }
	}
    }
}

static void
nathash_add (const JNINativeMethod *method)
{
  if (3 * nathash_count >= 2 * nathash_size)
    natrehash ();
  JNINativeMethod *slot = nathash_find_slot (method);
  // If the slot has a real entry in it, then there is no work to do.
  if (slot->name != NULL && slot->name != DELETED_ENTRY)
    return;
  // FIXME
  slot->name = strdup (method->name);
  slot->signature = strdup (method->signature);
  slot->fnPtr = method->fnPtr;
}

static jint
_Jv_JNI_RegisterNatives (JNIEnv *env, jclass klass,
			 const JNINativeMethod *methods,
			 jint nMethods)
{
  // Synchronize while we do the work.  This must match
  // synchronization in some other functions that manipulate or use
  // the nathash table.
  JvSynchronize sync (global_ref_table);

  // Look at each descriptor given us, and find the corresponding
  // method in the class.
  for (int j = 0; j < nMethods; ++j)
    {
      bool found = false;

      _Jv_Method *imeths = JvGetFirstMethod (klass);
      for (int i = 0; i < JvNumMethods (klass); ++i)
	{
	  _Jv_Method *self = &imeths[i];

	  if (! strcmp (self->name->data, methods[j].name)
	      && ! strcmp (self->signature->data, methods[j].signature))
	    {
	      if (! (self->accflags
		     & java::lang::reflect::Modifier::NATIVE))
		break;

	      // Found a match that is native.
	      found = true;
	      nathash_add (&methods[j]);

	      break;
	    }
	}

      if (! found)
	{
	  jstring m = JvNewStringUTF (methods[j].name);
	  try
	    {
	      env->ex =new java::lang::NoSuchMethodError (m);
	    }
	  catch (jthrowable t)
	    {
	      env->ex = t;
	    }
	  return JNI_ERR;
	}
    }

  return JNI_OK;
}

static jint
_Jv_JNI_UnregisterNatives (JNIEnv *, jclass)
{
  // FIXME -- we could implement this.
  return JNI_ERR;
}



// Add a character to the buffer, encoding properly.
static void
add_char (char *buf, jchar c, int *here)
{
  if (c == '_')
    {
      buf[(*here)++] = '_';
      buf[(*here)++] = '1';
    }
  else if (c == ';')
    {
      buf[(*here)++] = '_';
      buf[(*here)++] = '2';
    }
  else if (c == '[')
    {
      buf[(*here)++] = '_';
      buf[(*here)++] = '3';
    }

  // Also check for `.' here because we might be passed an internal
  // qualified class name like `foo.bar'.
  else if (c == '/' || c == '.')
    buf[(*here)++] = '_';
  else if ((c >= '0' && c <= '9')
	   || (c >= 'a' && c <= 'z')
	   || (c >= 'A' && c <= 'Z'))
    buf[(*here)++] = (char) c;
  else
    {
      // "Unicode" character.
      buf[(*here)++] = '_';
      buf[(*here)++] = '0';
      for (int i = 0; i < 4; ++i)
	{
	  int val = c & 0x0f;
	  buf[(*here) + 3 - i] = (val > 10) ? ('a' + val - 10) : ('0' + val);
	  c >>= 4;
	}
      *here += 4;
    }
}

// Compute a mangled name for a native function.  This computes the
// long name, and also returns an index which indicates where a NUL
// can be placed to create the short name.  This function assumes that
// the buffer is large enough for its results.
static void
mangled_name (jclass klass, _Jv_Utf8Const *func_name,
	      _Jv_Utf8Const *signature, char *buf, int *long_start)
{
  strcpy (buf, "Java_");
  int here = 5;

  // Add fully qualified class name.
  jchar *chars = _Jv_GetStringChars (klass->getName ());
  jint len = klass->getName ()->length ();
  for (int i = 0; i < len; ++i)
    add_char (buf, chars[i], &here);

  // Don't use add_char because we need a literal `_'.
  buf[here++] = '_';

  const unsigned char *fn = (const unsigned char *) func_name->data;
  const unsigned char *limit = fn + func_name->length;
  for (int i = 0; ; ++i)
    {
      int ch = UTF8_GET (fn, limit);
      if (ch < 0)
	break;
      add_char (buf, ch, &here);
    }

  // This is where the long signature begins.
  *long_start = here;
  buf[here++] = '_';
  buf[here++] = '_';

  const unsigned char *sig = (const unsigned char *) signature->data;
  limit = sig + signature->length;
  JvAssert (sig[0] == '(');
  ++sig;
  while (1)
    {
      int ch = UTF8_GET (sig, limit);
      if (ch == ')' || ch < 0)
	break;
      add_char (buf, ch, &here);
    }

  buf[here] = '\0';
}

// Return the current thread's JNIEnv; if one does not exist, create
// it.  Also create a new system frame for use.  This is `extern "C"'
// because the compiler calls it.
extern "C" JNIEnv *
_Jv_GetJNIEnvNewFrame (jclass klass)
{
  JNIEnv *env = _Jv_GetCurrentJNIEnv ();
  if (env == NULL)
    {
      env = (JNIEnv *) _Jv_MallocUnchecked (sizeof (JNIEnv));
      env->p = &_Jv_JNIFunctions;
      env->ex = NULL;
      env->klass = klass;
      env->locals = NULL;

      _Jv_SetCurrentJNIEnv (env);
    }

  _Jv_JNI_LocalFrame *frame
    = (_Jv_JNI_LocalFrame *) _Jv_MallocUnchecked (sizeof (_Jv_JNI_LocalFrame)
						  + (FRAME_SIZE
						     * sizeof (jobject)));

  frame->marker = MARK_SYSTEM;
  frame->size = FRAME_SIZE;
  frame->next = env->locals;
  env->locals = frame;

  for (int i = 0; i < frame->size; ++i)
    frame->vec[i] = NULL;

  return env;
}

// Return the function which implements a particular JNI method.  If
// we can't find the function, we throw the appropriate exception.
// This is `extern "C"' because the compiler uses it.
extern "C" void *
_Jv_LookupJNIMethod (jclass klass, _Jv_Utf8Const *name,
		     _Jv_Utf8Const *signature)
{
  char buf[10 + 6 * (name->length + signature->length)];
  int long_start;
  void *function;

  // Synchronize on something convenient.  Right now we use the hash.
  JvSynchronize sync (global_ref_table);

  // First see if we have an override in the hash table.
  strncpy (buf, name->data, name->length);
  buf[name->length] = '\0';
  strncpy (buf + name->length + 1, signature->data, signature->length);
  buf[name->length + signature->length + 1] = '\0';
  JNINativeMethod meth;
  meth.name = buf;
  meth.signature = buf + name->length + 1;
  function = nathash_find (&meth);
  if (function != NULL)
    return function;

  // If there was no override, then look in the symbol table.
  mangled_name (klass, name, signature, buf, &long_start);
  char c = buf[long_start];
  buf[long_start] = '\0';
  function = _Jv_FindSymbolInExecutable (buf);
  if (function == NULL)
    {
      buf[long_start] = c;
      function = _Jv_FindSymbolInExecutable (buf);
      if (function == NULL)
	{
	  jstring str = JvNewStringUTF (name->data);
	  throw new java::lang::UnsatisfiedLinkError (str);
	}
    }

  return function;
}

#ifdef INTERPRETER

// This function is the stub which is used to turn an ordinary (CNI)
// method call into a JNI call.
void
_Jv_JNIMethod::call (ffi_cif *, void *ret, ffi_raw *args, void *__this)
{
  _Jv_JNIMethod* _this = (_Jv_JNIMethod *) __this;

  JNIEnv *env = _Jv_GetJNIEnvNewFrame (_this->defining_class);

  // FIXME: we should mark every reference parameter as a local.  For
  // now we assume a conservative GC, and we assume that the
  // references are on the stack somewhere.

  // We cache the value that we find, of course, but if we don't find
  // a value we don't cache that fact -- we might subsequently load a
  // library which finds the function in question.
  {
    // Synchronize on a convenient object to ensure sanity in case two
    // threads reach this point for the same function at the same
    // time.
    JvSynchronize sync (global_ref_table);
    if (_this->function == NULL)
      _this->function = _Jv_LookupJNIMethod (_this->defining_class,
					     _this->self->name,
					     _this->self->signature);
  }

  JvAssert (_this->args_raw_size % sizeof (ffi_raw) == 0);
  ffi_raw real_args[2 + _this->args_raw_size / sizeof (ffi_raw)];
  int offset = 0;

  // First argument is always the environment pointer.
  real_args[offset++].ptr = env;

  // For a static method, we pass in the Class.  For non-static
  // methods, the `this' argument is already handled.
  if ((_this->self->accflags & java::lang::reflect::Modifier::STATIC))
    real_args[offset++].ptr = _this->defining_class;

  // In libgcj, the callee synchronizes.
  jobject sync = NULL;
  if ((_this->self->accflags & java::lang::reflect::Modifier::SYNCHRONIZED))
    {
      if ((_this->self->accflags & java::lang::reflect::Modifier::STATIC))
	sync = _this->defining_class;
      else
	sync = (jobject) args[0].ptr;
      _Jv_MonitorEnter (sync);
    }

  // Copy over passed-in arguments.
  memcpy (&real_args[offset], args, _this->args_raw_size);

  // The actual call to the JNI function.
  ffi_raw_call (&_this->jni_cif, (void (*)()) _this->function,
		ret, real_args);

  if (sync != NULL)
    _Jv_MonitorExit (sync);

  _Jv_JNI_PopSystemFrame (env);
}

#endif /* INTERPRETER */



//
// Invocation API.
//

// An internal helper function.
static jint
_Jv_JNI_AttachCurrentThread (JavaVM *, jstring name, void **penv,
			     void *args, jboolean is_daemon)
{
  JavaVMAttachArgs *attach = reinterpret_cast<JavaVMAttachArgs *> (args);
  java::lang::ThreadGroup *group = NULL;

  if (attach)
    {
      // FIXME: do we really want to support 1.1?
      if (attach->version != JNI_VERSION_1_4
	  && attach->version != JNI_VERSION_1_2
	  && attach->version != JNI_VERSION_1_1)
	return JNI_EVERSION;

      JvAssert (java::lang::ThreadGroup::class$.isInstance (attach->group));
      group = reinterpret_cast<java::lang::ThreadGroup *> (attach->group);
    }

  // Attaching an already-attached thread is a no-op.
  if (_Jv_GetCurrentJNIEnv () != NULL)
    return 0;

  JNIEnv *env = (JNIEnv *) _Jv_MallocUnchecked (sizeof (JNIEnv));
  if (env == NULL)
    return JNI_ERR;
  env->p = &_Jv_JNIFunctions;
  env->ex = NULL;
  env->klass = NULL;
  env->locals
    = (_Jv_JNI_LocalFrame *) _Jv_MallocUnchecked (sizeof (_Jv_JNI_LocalFrame)
						  + (FRAME_SIZE
						     * sizeof (jobject)));
  if (env->locals == NULL)
    {
      _Jv_Free (env);
      return JNI_ERR;
    }

  env->locals->marker = MARK_SYSTEM;
  env->locals->size = FRAME_SIZE;
  env->locals->next = NULL;

  for (int i = 0; i < env->locals->size; ++i)
    env->locals->vec[i] = NULL;

  *penv = reinterpret_cast<void *> (env);

  // This thread might already be a Java thread -- this function might
  // have been called simply to set the new JNIEnv.
  if (_Jv_ThreadCurrent () == NULL)
    {
      try
	{
	  if (is_daemon)
	    _Jv_AttachCurrentThreadAsDaemon (name, group);
	  else
	    _Jv_AttachCurrentThread (name, group);
	}
      catch (jthrowable t)
	{
	  return JNI_ERR;
	}
    }
  _Jv_SetCurrentJNIEnv (env);

  return 0;
}

// This is the one actually used by JNI.
static jint
_Jv_JNI_AttachCurrentThread (JavaVM *vm, void **penv, void *args)
{
  return _Jv_JNI_AttachCurrentThread (vm, NULL, penv, args, false);
}

static jint
_Jv_JNI_AttachCurrentThreadAsDaemon (JavaVM *vm, void **penv, void *args)
{
  return _Jv_JNI_AttachCurrentThread (vm, NULL, penv, args, true);
}

static jint
_Jv_JNI_DestroyJavaVM (JavaVM *vm)
{
  JvAssert (the_vm && vm == the_vm);

  JNIEnv *env;
  if (_Jv_ThreadCurrent () != NULL)
    {
      jstring main_name;
      // This sucks.
      try
	{
	  main_name = JvNewStringLatin1 ("main");
	}
      catch (jthrowable t)
	{
	  return JNI_ERR;
	}

      jint r = _Jv_JNI_AttachCurrentThread (vm, main_name,
					    reinterpret_cast<void **> (&env),
					    NULL, false);
      if (r < 0)
	return r;
    }
  else
    env = _Jv_GetCurrentJNIEnv ();

  _Jv_ThreadWait ();

  // Docs say that this always returns an error code.
  return JNI_ERR;
}

jint
_Jv_JNI_DetachCurrentThread (JavaVM *)
{
  jint code = _Jv_DetachCurrentThread ();
  return code  ? JNI_EDETACHED : 0;
}

static jint
_Jv_JNI_GetEnv (JavaVM *, void **penv, jint version)
{
  if (_Jv_ThreadCurrent () == NULL)
    {
      *penv = NULL;
      return JNI_EDETACHED;
    }

#ifdef ENABLE_JVMPI
  // Handle JVMPI requests.
  if (version == JVMPI_VERSION_1)
    {
      *penv = (void *) &_Jv_JVMPI_Interface;
      return 0;
    }
#endif

  // FIXME: do we really want to support 1.1?
  if (version != JNI_VERSION_1_4 && version != JNI_VERSION_1_2
      && version != JNI_VERSION_1_1)
    {
      *penv = NULL;
      return JNI_EVERSION;
    }

  *penv = (void *) _Jv_GetCurrentJNIEnv ();
  return 0;
}

jint
JNI_GetDefaultJavaVMInitArgs (void *args)
{
  jint version = * (jint *) args;
  // Here we only support 1.2 and 1.4.
  if (version != JNI_VERSION_1_2 && version != JNI_VERSION_1_4)
    return JNI_EVERSION;

  JavaVMInitArgs *ia = reinterpret_cast<JavaVMInitArgs *> (args);
  ia->version = JNI_VERSION_1_4;
  ia->nOptions = 0;
  ia->options = NULL;
  ia->ignoreUnrecognized = true;

  return 0;
}

jint
JNI_CreateJavaVM (JavaVM **vm, void **penv, void *args)
{
  JvAssert (! the_vm);

  _Jv_CreateJavaVM (NULL);

  // FIXME: synchronize
  JavaVM *nvm = (JavaVM *) _Jv_MallocUnchecked (sizeof (JavaVM));
  if (nvm == NULL)
    return JNI_ERR;
  nvm->functions = &_Jv_JNI_InvokeFunctions;

  // Parse the arguments.
  if (args != NULL)
    {
      jint version = * (jint *) args;
      // We only support 1.2 and 1.4.
      if (version != JNI_VERSION_1_2 && version != JNI_VERSION_1_4)
	return JNI_EVERSION;
      JavaVMInitArgs *ia = reinterpret_cast<JavaVMInitArgs *> (args);
      for (int i = 0; i < ia->nOptions; ++i)
	{
	  if (! strcmp (ia->options[i].optionString, "vfprintf")
	      || ! strcmp (ia->options[i].optionString, "exit")
	      || ! strcmp (ia->options[i].optionString, "abort"))
	    {
	      // We are required to recognize these, but for now we
	      // don't handle them in any way.  FIXME.
	      continue;
	    }
	  else if (! strncmp (ia->options[i].optionString,
			      "-verbose", sizeof ("-verbose") - 1))
	    {
	      // We don't do anything with this option either.  We
	      // might want to make sure the argument is valid, but we
	      // don't really care all that much for now.
	      continue;
	    }
	  else if (! strncmp (ia->options[i].optionString, "-D", 2))
	    {
	      // FIXME.
	      continue;
	    }
	  else if (ia->ignoreUnrecognized)
	    {
	      if (ia->options[i].optionString[0] == '_'
		  || ! strncmp (ia->options[i].optionString, "-X", 2))
		continue;
	    }

	  return JNI_ERR;
	}
    }

  jint r =_Jv_JNI_AttachCurrentThread (nvm, penv, NULL);
  if (r < 0)
    return r;

  the_vm = nvm;
  *vm = the_vm;

  return 0;
}

jint
JNI_GetCreatedJavaVMs (JavaVM **vm_buffer, jsize buf_len, jsize *n_vms)
{
  if (buf_len <= 0)
    return JNI_ERR;

  // We only support a single VM.
  if (the_vm != NULL)
    {
      vm_buffer[0] = the_vm;
      *n_vms = 1;
    }
  else
    *n_vms = 0;
  return 0;
}

JavaVM *
_Jv_GetJavaVM ()
{
  // FIXME: synchronize
  if (! the_vm)
    {
      JavaVM *nvm = (JavaVM *) _Jv_MallocUnchecked (sizeof (JavaVM));
      if (nvm != NULL)
	nvm->functions = &_Jv_JNI_InvokeFunctions;
      the_vm = nvm;
    }

  // If this is a Java thread, we want to make sure it has an
  // associated JNIEnv.
  if (_Jv_ThreadCurrent () != NULL)
    {
      void *ignore;
      _Jv_JNI_AttachCurrentThread (the_vm, &ignore, NULL);
    }

  return the_vm;
}

static jint
_Jv_JNI_GetJavaVM (JNIEnv *, JavaVM **vm)
{
  *vm = _Jv_GetJavaVM ();
  return *vm == NULL ? JNI_ERR : JNI_OK;
}



#define RESERVED NULL

struct JNINativeInterface _Jv_JNIFunctions =
{
  RESERVED,
  RESERVED,
  RESERVED,
  RESERVED,
  _Jv_JNI_GetVersion,		// GetVersion
  _Jv_JNI_DefineClass,		// DefineClass
  _Jv_JNI_FindClass,		// FindClass
  _Jv_JNI_FromReflectedMethod,	// FromReflectedMethod
  _Jv_JNI_FromReflectedField,	// FromReflectedField
  _Jv_JNI_ToReflectedMethod,	// ToReflectedMethod
  _Jv_JNI_GetSuperclass,	// GetSuperclass
  _Jv_JNI_IsAssignableFrom,	// IsAssignableFrom
  _Jv_JNI_ToReflectedField,	// ToReflectedField
  _Jv_JNI_Throw,		// Throw
  _Jv_JNI_ThrowNew,		// ThrowNew
  _Jv_JNI_ExceptionOccurred,	// ExceptionOccurred
  _Jv_JNI_ExceptionDescribe,	// ExceptionDescribe
  _Jv_JNI_ExceptionClear,	// ExceptionClear
  _Jv_JNI_FatalError,		// FatalError

  _Jv_JNI_PushLocalFrame,	// PushLocalFrame
  _Jv_JNI_PopLocalFrame,	// PopLocalFrame
  _Jv_JNI_NewGlobalRef,		// NewGlobalRef
  _Jv_JNI_DeleteGlobalRef,	// DeleteGlobalRef
  _Jv_JNI_DeleteLocalRef,	// DeleteLocalRef

  _Jv_JNI_IsSameObject,		// IsSameObject

  _Jv_JNI_NewLocalRef,		// NewLocalRef
  _Jv_JNI_EnsureLocalCapacity,	// EnsureLocalCapacity

  _Jv_JNI_AllocObject,		    // AllocObject
  _Jv_JNI_NewObject,		    // NewObject
  _Jv_JNI_NewObjectV,		    // NewObjectV
  _Jv_JNI_NewObjectA,		    // NewObjectA
  _Jv_JNI_GetObjectClass,	    // GetObjectClass
  _Jv_JNI_IsInstanceOf,		    // IsInstanceOf
  _Jv_JNI_GetAnyMethodID<false>,    // GetMethodID

  _Jv_JNI_CallMethod<jobject>,		// CallObjectMethod
  _Jv_JNI_CallMethodV<jobject>,		// CallObjectMethodV
  _Jv_JNI_CallMethodA<jobject>,		// CallObjectMethodA
  _Jv_JNI_CallMethod<jboolean>,		// CallBooleanMethod
  _Jv_JNI_CallMethodV<jboolean>,	// CallBooleanMethodV
  _Jv_JNI_CallMethodA<jboolean>,	// CallBooleanMethodA
  _Jv_JNI_CallMethod<jbyte>,		// CallByteMethod
  _Jv_JNI_CallMethodV<jbyte>,		// CallByteMethodV
  _Jv_JNI_CallMethodA<jbyte>,		// CallByteMethodA
  _Jv_JNI_CallMethod<jchar>,		// CallCharMethod
  _Jv_JNI_CallMethodV<jchar>,		// CallCharMethodV
  _Jv_JNI_CallMethodA<jchar>,		// CallCharMethodA
  _Jv_JNI_CallMethod<jshort>,		// CallShortMethod
  _Jv_JNI_CallMethodV<jshort>,		// CallShortMethodV
  _Jv_JNI_CallMethodA<jshort>,		// CallShortMethodA
  _Jv_JNI_CallMethod<jint>,		// CallIntMethod
  _Jv_JNI_CallMethodV<jint>,		// CallIntMethodV
  _Jv_JNI_CallMethodA<jint>,		// CallIntMethodA
  _Jv_JNI_CallMethod<jlong>,		// CallLongMethod
  _Jv_JNI_CallMethodV<jlong>,		// CallLongMethodV
  _Jv_JNI_CallMethodA<jlong>,		// CallLongMethodA
  _Jv_JNI_CallMethod<jfloat>,		// CallFloatMethod
  _Jv_JNI_CallMethodV<jfloat>,		// CallFloatMethodV
  _Jv_JNI_CallMethodA<jfloat>,		// CallFloatMethodA
  _Jv_JNI_CallMethod<jdouble>,		// CallDoubleMethod
  _Jv_JNI_CallMethodV<jdouble>,		// CallDoubleMethodV
  _Jv_JNI_CallMethodA<jdouble>,		// CallDoubleMethodA
  _Jv_JNI_CallVoidMethod,		// CallVoidMethod
  _Jv_JNI_CallVoidMethodV,		// CallVoidMethodV
  _Jv_JNI_CallVoidMethodA,		// CallVoidMethodA

  // Nonvirtual method invocation functions follow.
  _Jv_JNI_CallAnyMethod<jobject, nonvirtual>,	// CallNonvirtualObjectMethod
  _Jv_JNI_CallAnyMethodV<jobject, nonvirtual>,	// CallNonvirtualObjectMethodV
  _Jv_JNI_CallAnyMethodA<jobject, nonvirtual>,	// CallNonvirtualObjectMethodA
  _Jv_JNI_CallAnyMethod<jboolean, nonvirtual>,	// CallNonvirtualBooleanMethod
  _Jv_JNI_CallAnyMethodV<jboolean, nonvirtual>,	// CallNonvirtualBooleanMethodV
  _Jv_JNI_CallAnyMethodA<jboolean, nonvirtual>,	// CallNonvirtualBooleanMethodA
  _Jv_JNI_CallAnyMethod<jbyte, nonvirtual>,	// CallNonvirtualByteMethod
  _Jv_JNI_CallAnyMethodV<jbyte, nonvirtual>,	// CallNonvirtualByteMethodV
  _Jv_JNI_CallAnyMethodA<jbyte, nonvirtual>,	// CallNonvirtualByteMethodA
  _Jv_JNI_CallAnyMethod<jchar, nonvirtual>,	// CallNonvirtualCharMethod
  _Jv_JNI_CallAnyMethodV<jchar, nonvirtual>,	// CallNonvirtualCharMethodV
  _Jv_JNI_CallAnyMethodA<jchar, nonvirtual>,	// CallNonvirtualCharMethodA
  _Jv_JNI_CallAnyMethod<jshort, nonvirtual>,	// CallNonvirtualShortMethod
  _Jv_JNI_CallAnyMethodV<jshort, nonvirtual>,	// CallNonvirtualShortMethodV
  _Jv_JNI_CallAnyMethodA<jshort, nonvirtual>,	// CallNonvirtualShortMethodA
  _Jv_JNI_CallAnyMethod<jint, nonvirtual>,	// CallNonvirtualIntMethod
  _Jv_JNI_CallAnyMethodV<jint, nonvirtual>,	// CallNonvirtualIntMethodV
  _Jv_JNI_CallAnyMethodA<jint, nonvirtual>,	// CallNonvirtualIntMethodA
  _Jv_JNI_CallAnyMethod<jlong, nonvirtual>,	// CallNonvirtualLongMethod
  _Jv_JNI_CallAnyMethodV<jlong, nonvirtual>,	// CallNonvirtualLongMethodV
  _Jv_JNI_CallAnyMethodA<jlong, nonvirtual>,	// CallNonvirtualLongMethodA
  _Jv_JNI_CallAnyMethod<jfloat, nonvirtual>,	// CallNonvirtualFloatMethod
  _Jv_JNI_CallAnyMethodV<jfloat, nonvirtual>,	// CallNonvirtualFloatMethodV
  _Jv_JNI_CallAnyMethodA<jfloat, nonvirtual>,	// CallNonvirtualFloatMethodA
  _Jv_JNI_CallAnyMethod<jdouble, nonvirtual>,	// CallNonvirtualDoubleMethod
  _Jv_JNI_CallAnyMethodV<jdouble, nonvirtual>,	// CallNonvirtualDoubleMethodV
  _Jv_JNI_CallAnyMethodA<jdouble, nonvirtual>,	// CallNonvirtualDoubleMethodA
  _Jv_JNI_CallAnyVoidMethod<nonvirtual>,	// CallNonvirtualVoidMethod
  _Jv_JNI_CallAnyVoidMethodV<nonvirtual>,	// CallNonvirtualVoidMethodV
  _Jv_JNI_CallAnyVoidMethodA<nonvirtual>,	// CallNonvirtualVoidMethodA

  _Jv_JNI_GetAnyFieldID<false>,	// GetFieldID
  _Jv_JNI_GetField<jobject>,	// GetObjectField
  _Jv_JNI_GetField<jboolean>,	// GetBooleanField
  _Jv_JNI_GetField<jbyte>,	// GetByteField
  _Jv_JNI_GetField<jchar>,	// GetCharField
  _Jv_JNI_GetField<jshort>,	// GetShortField
  _Jv_JNI_GetField<jint>,	// GetIntField
  _Jv_JNI_GetField<jlong>,	// GetLongField
  _Jv_JNI_GetField<jfloat>,	// GetFloatField
  _Jv_JNI_GetField<jdouble>,	// GetDoubleField
  _Jv_JNI_SetField,		// SetObjectField
  _Jv_JNI_SetField,		// SetBooleanField
  _Jv_JNI_SetField,		// SetByteField
  _Jv_JNI_SetField,		// SetCharField
  _Jv_JNI_SetField,		// SetShortField
  _Jv_JNI_SetField,		// SetIntField
  _Jv_JNI_SetField,		// SetLongField
  _Jv_JNI_SetField,		// SetFloatField
  _Jv_JNI_SetField,		// SetDoubleField
  _Jv_JNI_GetAnyMethodID<true>,	// GetStaticMethodID

  _Jv_JNI_CallStaticMethod<jobject>,	  // CallStaticObjectMethod
  _Jv_JNI_CallStaticMethodV<jobject>,	  // CallStaticObjectMethodV
  _Jv_JNI_CallStaticMethodA<jobject>,	  // CallStaticObjectMethodA
  _Jv_JNI_CallStaticMethod<jboolean>,	  // CallStaticBooleanMethod
  _Jv_JNI_CallStaticMethodV<jboolean>,	  // CallStaticBooleanMethodV
  _Jv_JNI_CallStaticMethodA<jboolean>,	  // CallStaticBooleanMethodA
  _Jv_JNI_CallStaticMethod<jbyte>,	  // CallStaticByteMethod
  _Jv_JNI_CallStaticMethodV<jbyte>,	  // CallStaticByteMethodV
  _Jv_JNI_CallStaticMethodA<jbyte>,	  // CallStaticByteMethodA
  _Jv_JNI_CallStaticMethod<jchar>,	  // CallStaticCharMethod
  _Jv_JNI_CallStaticMethodV<jchar>,	  // CallStaticCharMethodV
  _Jv_JNI_CallStaticMethodA<jchar>,	  // CallStaticCharMethodA
  _Jv_JNI_CallStaticMethod<jshort>,	  // CallStaticShortMethod
  _Jv_JNI_CallStaticMethodV<jshort>,	  // CallStaticShortMethodV
  _Jv_JNI_CallStaticMethodA<jshort>,	  // CallStaticShortMethodA
  _Jv_JNI_CallStaticMethod<jint>,	  // CallStaticIntMethod
  _Jv_JNI_CallStaticMethodV<jint>,	  // CallStaticIntMethodV
  _Jv_JNI_CallStaticMethodA<jint>,	  // CallStaticIntMethodA
  _Jv_JNI_CallStaticMethod<jlong>,	  // CallStaticLongMethod
  _Jv_JNI_CallStaticMethodV<jlong>,	  // CallStaticLongMethodV
  _Jv_JNI_CallStaticMethodA<jlong>,	  // CallStaticLongMethodA
  _Jv_JNI_CallStaticMethod<jfloat>,	  // CallStaticFloatMethod
  _Jv_JNI_CallStaticMethodV<jfloat>,	  // CallStaticFloatMethodV
  _Jv_JNI_CallStaticMethodA<jfloat>,	  // CallStaticFloatMethodA
  _Jv_JNI_CallStaticMethod<jdouble>,	  // CallStaticDoubleMethod
  _Jv_JNI_CallStaticMethodV<jdouble>,	  // CallStaticDoubleMethodV
  _Jv_JNI_CallStaticMethodA<jdouble>,	  // CallStaticDoubleMethodA
  _Jv_JNI_CallStaticVoidMethod,		  // CallStaticVoidMethod
  _Jv_JNI_CallStaticVoidMethodV,	  // CallStaticVoidMethodV
  _Jv_JNI_CallStaticVoidMethodA,	  // CallStaticVoidMethodA

  _Jv_JNI_GetAnyFieldID<true>,	       // GetStaticFieldID
  _Jv_JNI_GetStaticField<jobject>,     // GetStaticObjectField
  _Jv_JNI_GetStaticField<jboolean>,    // GetStaticBooleanField
  _Jv_JNI_GetStaticField<jbyte>,       // GetStaticByteField
  _Jv_JNI_GetStaticField<jchar>,       // GetStaticCharField
  _Jv_JNI_GetStaticField<jshort>,      // GetStaticShortField
  _Jv_JNI_GetStaticField<jint>,	       // GetStaticIntField
  _Jv_JNI_GetStaticField<jlong>,       // GetStaticLongField
  _Jv_JNI_GetStaticField<jfloat>,      // GetStaticFloatField
  _Jv_JNI_GetStaticField<jdouble>,     // GetStaticDoubleField
  _Jv_JNI_SetStaticField,	       // SetStaticObjectField
  _Jv_JNI_SetStaticField,	       // SetStaticBooleanField
  _Jv_JNI_SetStaticField,	       // SetStaticByteField
  _Jv_JNI_SetStaticField,	       // SetStaticCharField
  _Jv_JNI_SetStaticField,	       // SetStaticShortField
  _Jv_JNI_SetStaticField,	       // SetStaticIntField
  _Jv_JNI_SetStaticField,	       // SetStaticLongField
  _Jv_JNI_SetStaticField,	       // SetStaticFloatField
  _Jv_JNI_SetStaticField,	       // SetStaticDoubleField
  _Jv_JNI_NewString,		       // NewString
  _Jv_JNI_GetStringLength,	       // GetStringLength
  _Jv_JNI_GetStringChars,	       // GetStringChars
  _Jv_JNI_ReleaseStringChars,	       // ReleaseStringChars
  _Jv_JNI_NewStringUTF,		       // NewStringUTF
  _Jv_JNI_GetStringUTFLength,	       // GetStringUTFLength
  _Jv_JNI_GetStringUTFChars,	       // GetStringUTFLength
  _Jv_JNI_ReleaseStringUTFChars,       // ReleaseStringUTFChars
  _Jv_JNI_GetArrayLength,	       // GetArrayLength
  _Jv_JNI_NewObjectArray,	       // NewObjectArray
  _Jv_JNI_GetObjectArrayElement,       // GetObjectArrayElement
  _Jv_JNI_SetObjectArrayElement,       // SetObjectArrayElement
  _Jv_JNI_NewPrimitiveArray<jboolean, JvPrimClass (boolean)>,
							    // NewBooleanArray
  _Jv_JNI_NewPrimitiveArray<jbyte, JvPrimClass (byte)>,	    // NewByteArray
  _Jv_JNI_NewPrimitiveArray<jchar, JvPrimClass (char)>,	    // NewCharArray
  _Jv_JNI_NewPrimitiveArray<jshort, JvPrimClass (short)>,   // NewShortArray
  _Jv_JNI_NewPrimitiveArray<jint, JvPrimClass (int)>,	    // NewIntArray
  _Jv_JNI_NewPrimitiveArray<jlong, JvPrimClass (long)>,	    // NewLongArray
  _Jv_JNI_NewPrimitiveArray<jfloat, JvPrimClass (float)>,   // NewFloatArray
  _Jv_JNI_NewPrimitiveArray<jdouble, JvPrimClass (double)>, // NewDoubleArray
  _Jv_JNI_GetPrimitiveArrayElements,	    // GetBooleanArrayElements
  _Jv_JNI_GetPrimitiveArrayElements,	    // GetByteArrayElements
  _Jv_JNI_GetPrimitiveArrayElements,	    // GetCharArrayElements
  _Jv_JNI_GetPrimitiveArrayElements,	    // GetShortArrayElements
  _Jv_JNI_GetPrimitiveArrayElements,	    // GetIntArrayElements
  _Jv_JNI_GetPrimitiveArrayElements,	    // GetLongArrayElements
  _Jv_JNI_GetPrimitiveArrayElements,	    // GetFloatArrayElements
  _Jv_JNI_GetPrimitiveArrayElements,	    // GetDoubleArrayElements
  _Jv_JNI_ReleasePrimitiveArrayElements,    // ReleaseBooleanArrayElements
  _Jv_JNI_ReleasePrimitiveArrayElements,    // ReleaseByteArrayElements
  _Jv_JNI_ReleasePrimitiveArrayElements,    // ReleaseCharArrayElements
  _Jv_JNI_ReleasePrimitiveArrayElements,    // ReleaseShortArrayElements
  _Jv_JNI_ReleasePrimitiveArrayElements,    // ReleaseIntArrayElements
  _Jv_JNI_ReleasePrimitiveArrayElements,    // ReleaseLongArrayElements
  _Jv_JNI_ReleasePrimitiveArrayElements,    // ReleaseFloatArrayElements
  _Jv_JNI_ReleasePrimitiveArrayElements,    // ReleaseDoubleArrayElements
  _Jv_JNI_GetPrimitiveArrayRegion,	    // GetBooleanArrayRegion
  _Jv_JNI_GetPrimitiveArrayRegion,	    // GetByteArrayRegion
  _Jv_JNI_GetPrimitiveArrayRegion,	    // GetCharArrayRegion
  _Jv_JNI_GetPrimitiveArrayRegion,	    // GetShortArrayRegion
  _Jv_JNI_GetPrimitiveArrayRegion,	    // GetIntArrayRegion
  _Jv_JNI_GetPrimitiveArrayRegion,	    // GetLongArrayRegion
  _Jv_JNI_GetPrimitiveArrayRegion,	    // GetFloatArrayRegion
  _Jv_JNI_GetPrimitiveArrayRegion,	    // GetDoubleArrayRegion
  _Jv_JNI_SetPrimitiveArrayRegion,	    // SetBooleanArrayRegion
  _Jv_JNI_SetPrimitiveArrayRegion,	    // SetByteArrayRegion
  _Jv_JNI_SetPrimitiveArrayRegion,	    // SetCharArrayRegion
  _Jv_JNI_SetPrimitiveArrayRegion,	    // SetShortArrayRegion
  _Jv_JNI_SetPrimitiveArrayRegion,	    // SetIntArrayRegion
  _Jv_JNI_SetPrimitiveArrayRegion,	    // SetLongArrayRegion
  _Jv_JNI_SetPrimitiveArrayRegion,	    // SetFloatArrayRegion
  _Jv_JNI_SetPrimitiveArrayRegion,	    // SetDoubleArrayRegion
  _Jv_JNI_RegisterNatives,		    // RegisterNatives
  _Jv_JNI_UnregisterNatives,		    // UnregisterNatives
  _Jv_JNI_MonitorEnter,			    // MonitorEnter
  _Jv_JNI_MonitorExit,			    // MonitorExit
  _Jv_JNI_GetJavaVM,			    // GetJavaVM

  _Jv_JNI_GetStringRegion,		    // GetStringRegion
  _Jv_JNI_GetStringUTFRegion,		    // GetStringUTFRegion
  _Jv_JNI_GetPrimitiveArrayCritical,	    // GetPrimitiveArrayCritical
  _Jv_JNI_ReleasePrimitiveArrayCritical,    // ReleasePrimitiveArrayCritical
  _Jv_JNI_GetStringCritical,		    // GetStringCritical
  _Jv_JNI_ReleaseStringCritical,	    // ReleaseStringCritical

  _Jv_JNI_NewWeakGlobalRef,		    // NewWeakGlobalRef
  _Jv_JNI_DeleteWeakGlobalRef,		    // DeleteWeakGlobalRef

  _Jv_JNI_ExceptionCheck,		    // ExceptionCheck

  _Jv_JNI_NewDirectByteBuffer,		    // NewDirectByteBuffer
  _Jv_JNI_GetDirectBufferAddress,	    // GetDirectBufferAddress
  _Jv_JNI_GetDirectBufferCapacity	    // GetDirectBufferCapacity
};

struct JNIInvokeInterface _Jv_JNI_InvokeFunctions =
{
  RESERVED,
  RESERVED,
  RESERVED,

  _Jv_JNI_DestroyJavaVM,
  _Jv_JNI_AttachCurrentThread,
  _Jv_JNI_DetachCurrentThread,
  _Jv_JNI_GetEnv,
  _Jv_JNI_AttachCurrentThreadAsDaemon
};
