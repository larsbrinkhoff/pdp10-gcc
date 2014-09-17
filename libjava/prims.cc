// prims.cc - Code for core of runtime environment.

/* Copyright (C) 1998, 1999, 2000, 2001, 2002  Free Software Foundation

   This file is part of libgcj.

This software is copyrighted work licensed under the terms of the
Libgcj License.  Please consult the file "LIBGCJ_LICENSE" for
details.  */

#include <config.h>
#include <platform.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gcj/cni.h>
#include <jvm.h>
#include <java-signal.h>
#include <java-threads.h>

#ifdef ENABLE_JVMPI
#include <jvmpi.h>
#include <java/lang/ThreadGroup.h>
#endif

#ifndef DISABLE_GETENV_PROPERTIES
#include <ctype.h>
#include <java-props.h>
#define PROCESS_GCJ_PROPERTIES process_gcj_properties()
#else
#define PROCESS_GCJ_PROPERTIES
#endif // DISABLE_GETENV_PROPERTIES

#include <java/lang/Class.h>
#include <java/lang/ClassLoader.h>
#include <java/lang/Runtime.h>
#include <java/lang/String.h>
#include <java/lang/Thread.h>
#include <java/lang/ThreadGroup.h>
#include <java/lang/ArrayIndexOutOfBoundsException.h>
#include <java/lang/ArithmeticException.h>
#include <java/lang/ClassFormatError.h>
#include <java/lang/InternalError.h>
#include <java/lang/NegativeArraySizeException.h>
#include <java/lang/NullPointerException.h>
#include <java/lang/OutOfMemoryError.h>
#include <java/lang/System.h>
#include <java/lang/reflect/Modifier.h>
#include <java/io/PrintStream.h>
#include <java/lang/UnsatisfiedLinkError.h>
#include <java/lang/VirtualMachineError.h>
#include <gnu/gcj/runtime/VMClassLoader.h>
#include <gnu/gcj/runtime/FinalizerThread.h>
#include <gnu/gcj/runtime/FirstThread.h>

#ifdef USE_LTDL
#include <ltdl.h>
#endif

// We allocate a single OutOfMemoryError exception which we keep
// around for use if we run out of memory.
static java::lang::OutOfMemoryError *no_memory;

// Largest representable size_t.
#define SIZE_T_MAX ((size_t) (~ (size_t) 0))

static const char *no_properties[] = { NULL };

// Properties set at compile time.
const char **_Jv_Compiler_Properties = no_properties;

// The JAR file to add to the beginning of java.class.path.
const char *_Jv_Jar_Class_Path;

#ifndef DISABLE_GETENV_PROPERTIES
// Property key/value pairs.
property_pair *_Jv_Environment_Properties;
#endif

// The name of this executable.
static char *_Jv_execName;

// Stash the argv pointer to benefit native libraries that need it.
const char **_Jv_argv;
int _Jv_argc;

#ifdef ENABLE_JVMPI
// Pointer to JVMPI notification functions.
void (*_Jv_JVMPI_Notify_OBJECT_ALLOC) (JVMPI_Event *event);
void (*_Jv_JVMPI_Notify_THREAD_START) (JVMPI_Event *event);
void (*_Jv_JVMPI_Notify_THREAD_END) (JVMPI_Event *event);
#endif


extern "C" void _Jv_ThrowSignal (jthrowable) __attribute ((noreturn));

// Just like _Jv_Throw, but fill in the stack trace first.  Although
// this is declared extern in order that its name not be mangled, it
// is not intended to be used outside this file.
void 
_Jv_ThrowSignal (jthrowable throwable)
{
  throwable->fillInStackTrace ();
  throw throwable;
}
 
#ifdef HANDLE_SEGV
static java::lang::NullPointerException *nullp;

SIGNAL_HANDLER (catch_segv)
{
  MAKE_THROW_FRAME (nullp);
  _Jv_ThrowSignal (nullp);
}
#endif

static java::lang::ArithmeticException *arithexception;

#ifdef HANDLE_FPE
SIGNAL_HANDLER (catch_fpe)
{
#ifdef HANDLE_DIVIDE_OVERFLOW
  HANDLE_DIVIDE_OVERFLOW;
#else
  MAKE_THROW_FRAME (arithexception);
#endif
  _Jv_ThrowSignal (arithexception);
}
#endif



jboolean
_Jv_equalUtf8Consts (Utf8Const* a, Utf8Const *b)
{
  int len;
  _Jv_ushort *aptr, *bptr;
  if (a == b)
    return true;
  if (a->hash != b->hash)
    return false;
  len = a->length;
  if (b->length != len)
    return false;
  aptr = (_Jv_ushort *)a->data;
  bptr = (_Jv_ushort *)b->data;
  len = (len + 1) >> 1;
  while (--len >= 0)
    if (*aptr++ != *bptr++)
      return false;
  return true;
}

/* True iff A is equal to STR.
   HASH is STR->hashCode().  
*/

jboolean
_Jv_equal (Utf8Const* a, jstring str, jint hash)
{
  if (a->hash != (_Jv_ushort) hash)
    return false;
  jint len = str->length();
  jint i = 0;
  jchar *sptr = _Jv_GetStringChars (str);
  unsigned char* ptr = (unsigned char*) a->data;
  unsigned char* limit = ptr + a->length;
  for (;; i++, sptr++)
    {
      int ch = UTF8_GET (ptr, limit);
      if (i == len)
	return ch < 0;
      if (ch != *sptr)
	return false;
    }
  return true;
}

/* Like _Jv_equal, but stop after N characters.  */
jboolean
_Jv_equaln (Utf8Const *a, jstring str, jint n)
{
  jint len = str->length();
  jint i = 0;
  jchar *sptr = _Jv_GetStringChars (str);
  unsigned char* ptr = (unsigned char*) a->data;
  unsigned char* limit = ptr + a->length;
  for (; n-- > 0; i++, sptr++)
    {
      int ch = UTF8_GET (ptr, limit);
      if (i == len)
	return ch < 0;
      if (ch != *sptr)
	return false;
    }
  return true;
}

/* Count the number of Unicode chars encoded in a given Ut8 string. */
int
_Jv_strLengthUtf8(char* str, int len)
{
  unsigned char* ptr;
  unsigned char* limit;
  int str_length;

  ptr = (unsigned char*) str;
  limit = ptr + len;
  str_length = 0;
  for (; ptr < limit; str_length++)
    {
      if (UTF8_GET (ptr, limit) < 0)
	return (-1);
    }
  return (str_length);
}

/* Calculate a hash value for a string encoded in Utf8 format.
 * This returns the same hash value as specified or java.lang.String.hashCode.
 */
static jint
hashUtf8String (char* str, int len)
{
  unsigned char* ptr = (unsigned char*) str;
  unsigned char* limit = ptr + len;
  jint hash = 0;

  for (; ptr < limit;)
    {
      int ch = UTF8_GET (ptr, limit);
      /* Updated specification from
	 http://www.javasoft.com/docs/books/jls/clarify.html. */
      hash = (31 * hash) + ch;
    }
  return hash;
}

_Jv_Utf8Const *
_Jv_makeUtf8Const (char* s, int len)
{
  if (len < 0)
    len = strlen (s);
  Utf8Const* m = (Utf8Const*) _Jv_AllocBytes (sizeof(Utf8Const) + len + 1);
  memcpy (m->data, s, len);
  m->data[len] = 0;
  m->length = len;
  m->hash = hashUtf8String (s, len) & 0xFFFF;
  return (m);
}

_Jv_Utf8Const *
_Jv_makeUtf8Const (jstring string)
{
  jint hash = string->hashCode ();
  jint len = _Jv_GetStringUTFLength (string);

  Utf8Const* m = (Utf8Const*)
    _Jv_AllocBytes (sizeof(Utf8Const) + len + 1);

  m->hash = hash;
  m->length = len;

  _Jv_GetStringUTFRegion (string, 0, string->length (), m->data);
  m->data[len] = 0;
  
  return m;
}



#ifdef DEBUG
void
_Jv_Abort (const char *function, const char *file, int line,
	   const char *message)
#else
void
_Jv_Abort (const char *, const char *, int, const char *message)
#endif
{
#ifdef DEBUG
  fprintf (stderr,
	   "libgcj failure: %s\n   in function %s, file %s, line %d\n",
	   message, function, file, line);
#else
  fprintf (stderr, "libgcj failure: %s\n", message);
#endif
  abort ();
}

static void
fail_on_finalization (jobject)
{
  JvFail ("object was finalized");
}

void
_Jv_GCWatch (jobject obj)
{
  _Jv_RegisterFinalizer (obj, fail_on_finalization);
}

void
_Jv_ThrowBadArrayIndex(jint bad_index)
{
  throw new java::lang::ArrayIndexOutOfBoundsException
    (java::lang::String::valueOf (bad_index));
}

void
_Jv_ThrowNullPointerException ()
{
  throw new java::lang::NullPointerException;
}

// Explicitly throw a no memory exception.
// The collector calls this when it encounters an out-of-memory condition.
void _Jv_ThrowNoMemory()
{
  throw no_memory;
}

#ifdef ENABLE_JVMPI
static void
jvmpi_notify_alloc(jclass klass, jint size, jobject obj)
{
  // Service JVMPI allocation request.
  if (__builtin_expect (_Jv_JVMPI_Notify_OBJECT_ALLOC != 0, false))
    {
      JVMPI_Event event;

      event.event_type = JVMPI_EVENT_OBJECT_ALLOC;
      event.env_id = NULL;
      event.u.obj_alloc.arena_id = 0;
      event.u.obj_alloc.class_id = (jobjectID) klass;
      event.u.obj_alloc.is_array = 0;
      event.u.obj_alloc.size = size;
      event.u.obj_alloc.obj_id = (jobjectID) obj;

      // FIXME:  This doesn't look right for the Boehm GC.  A GC may
      // already be in progress.  _Jv_DisableGC () doesn't wait for it.
      // More importantly, I don't see the need for disabling GC, since we
      // blatantly have a pointer to obj on our stack, ensuring that the
      // object can't be collected.  Even for a nonconservative collector,
      // it appears to me that this must be true, since we are about to
      // return obj. Isn't this whole approach way too intrusive for
      // a useful profiling interface?			- HB
      _Jv_DisableGC ();
      (*_Jv_JVMPI_Notify_OBJECT_ALLOC) (&event);
      _Jv_EnableGC ();
    }
}
#else /* !ENABLE_JVMPI */
# define jvmpi_notify_alloc(klass,size,obj) /* do nothing */
#endif

// Allocate a new object of class KLASS.  SIZE is the size of the object
// to allocate.  You might think this is redundant, but it isn't; some
// classes, such as String, aren't of fixed size.
// First a version that assumes that we have no finalizer, and that
// the class is already initialized.
// If we know that JVMPI is disabled, this can be replaced by a direct call
// to the allocator for the appropriate GC.
jobject
_Jv_AllocObjectNoInitNoFinalizer (jclass klass, jint size)
{
  jobject obj = (jobject) _Jv_AllocObj (size, klass);
  jvmpi_notify_alloc (klass, size, obj);
  return obj;
}

// And now a version that initializes if necessary.
jobject
_Jv_AllocObjectNoFinalizer (jclass klass, jint size)
{
  _Jv_InitClass (klass);
  jobject obj = (jobject) _Jv_AllocObj (size, klass);
  jvmpi_notify_alloc (klass, size, obj);
  return obj;
}

// And now the general version that registers a finalizer if necessary.
jobject
_Jv_AllocObject (jclass klass, jint size)
{
  jobject obj = _Jv_AllocObjectNoFinalizer (klass, size);

  // We assume that the compiler only generates calls to this routine
  // if there really is an interesting finalizer.
  // Unfortunately, we still have to the dynamic test, since there may
  // be cni calls to this routine.
  // Nore that on IA64 get_finalizer() returns the starting address of the
  // function, not a function pointer.  Thus this still works.
  if (klass->vtable->get_finalizer ()
      != java::lang::Object::class$.vtable->get_finalizer ())
    _Jv_RegisterFinalizer (obj, _Jv_FinalizeObject);
  return obj;
}

// A version of the above that assumes the object contains no pointers,
// and requires no finalization.  This can't happen if we need pointers
// to locks.
#ifdef JV_HASH_SYNCHRONIZATION
jobject
_Jv_AllocPtrFreeObject (jclass klass, jint size)
{
  _Jv_InitClass (klass);

  jobject obj = (jobject) _Jv_AllocPtrFreeObj (size, klass);

#ifdef ENABLE_JVMPI
  // Service JVMPI request.

  if (__builtin_expect (_Jv_JVMPI_Notify_OBJECT_ALLOC != 0, false))
    {
      JVMPI_Event event;

      event.event_type = JVMPI_EVENT_OBJECT_ALLOC;
      event.env_id = NULL;
      event.u.obj_alloc.arena_id = 0;
      event.u.obj_alloc.class_id = (jobjectID) klass;
      event.u.obj_alloc.is_array = 0;
      event.u.obj_alloc.size = size;
      event.u.obj_alloc.obj_id = (jobjectID) obj;

      _Jv_DisableGC ();
      (*_Jv_JVMPI_Notify_OBJECT_ALLOC) (&event);
      _Jv_EnableGC ();
    }
#endif

  return obj;
}
#endif /* JV_HASH_SYNCHRONIZATION */


// Allocate a new array of Java objects.  Each object is of type
// `elementClass'.  `init' is used to initialize each slot in the
// array.
jobjectArray
_Jv_NewObjectArray (jsize count, jclass elementClass, jobject init)
{
  if (__builtin_expect (count < 0, false))
    throw new java::lang::NegativeArraySizeException;

  JvAssert (! elementClass->isPrimitive ());

  // Ensure that elements pointer is properly aligned.
  jobjectArray obj = NULL;
  size_t size = (size_t) elements (obj);
  size += count * sizeof (jobject);

  // FIXME: second argument should be "current loader"
  jclass klass = _Jv_GetArrayClass (elementClass, 0);

  obj = (jobjectArray) _Jv_AllocArray (size, klass);
  // Cast away const.
  jsize *lp = const_cast<jsize *> (&obj->length);
  *lp = count;
  // We know the allocator returns zeroed memory.  So don't bother
  // zeroing it again.
  if (init)
    {
      jobject *ptr = elements(obj);
      while (--count >= 0)
	*ptr++ = init;
    }
  return obj;
}

// Allocate a new array of primitives.  ELTYPE is the type of the
// element, COUNT is the size of the array.
jobject
_Jv_NewPrimArray (jclass eltype, jint count)
{
  int elsize = eltype->size();
  if (__builtin_expect (count < 0, false))
    throw new java::lang::NegativeArraySizeException;

  JvAssert (eltype->isPrimitive ());
  jobject dummy = NULL;
  size_t size = (size_t) _Jv_GetArrayElementFromElementType (dummy, eltype);

  // Check for overflow.
  if (__builtin_expect ((size_t) count > 
			(SIZE_T_MAX - size) / elsize, false))
    throw no_memory;

  jclass klass = _Jv_GetArrayClass (eltype, 0);

# ifdef JV_HASH_SYNCHRONIZATION
  // Since the vtable is always statically allocated,
  // these are completely pointerfree!  Make sure the GC doesn't touch them.
  __JArray *arr =
    (__JArray*) _Jv_AllocPtrFreeObj (size + elsize * count, klass);
  memset((char *)arr + size, 0, elsize * count);
# else
  __JArray *arr = (__JArray*) _Jv_AllocObj (size + elsize * count, klass);
  // Note that we assume we are given zeroed memory by the allocator.
# endif
  // Cast away const.
  jsize *lp = const_cast<jsize *> (&arr->length);
  *lp = count;

  return arr;
}

jobject
_Jv_NewArray (jint type, jint size)
{
  switch (type)
    {
      case  4:  return JvNewBooleanArray (size);
      case  5:  return JvNewCharArray (size);
      case  6:  return JvNewFloatArray (size);
      case  7:  return JvNewDoubleArray (size);
      case  8:  return JvNewByteArray (size);
      case  9:  return JvNewShortArray (size);
      case 10:  return JvNewIntArray (size);
      case 11:  return JvNewLongArray (size);
    }
  throw new java::lang::InternalError
    (JvNewStringLatin1 ("invalid type code in _Jv_NewArray"));
}

// Allocate a possibly multi-dimensional array but don't check that
// any array length is <0.
static jobject
_Jv_NewMultiArrayUnchecked (jclass type, jint dimensions, jint *sizes)
{
  JvAssert (type->isArray());
  jclass element_type = type->getComponentType();
  jobject result;
  if (element_type->isPrimitive())
    result = _Jv_NewPrimArray (element_type, sizes[0]);
  else
    result = _Jv_NewObjectArray (sizes[0], element_type, NULL);

  if (dimensions > 1)
    {
      JvAssert (! element_type->isPrimitive());
      JvAssert (element_type->isArray());
      jobject *contents = elements ((jobjectArray) result);
      for (int i = 0; i < sizes[0]; ++i)
	contents[i] = _Jv_NewMultiArrayUnchecked (element_type, dimensions - 1,
						  sizes + 1);
    }

  return result;
}

jobject
_Jv_NewMultiArray (jclass type, jint dimensions, jint *sizes)
{
  for (int i = 0; i < dimensions; ++i)
    if (sizes[i] < 0)
      throw new java::lang::NegativeArraySizeException;

  return _Jv_NewMultiArrayUnchecked (type, dimensions, sizes);
}

jobject
_Jv_NewMultiArray (jclass array_type, jint dimensions, ...)
{
  va_list args;
  jint sizes[dimensions];
  va_start (args, dimensions);
  for (int i = 0; i < dimensions; ++i)
    {
      jint size = va_arg (args, jint);
      if (size < 0)
	throw new java::lang::NegativeArraySizeException;
      sizes[i] = size;
    }
  va_end (args);

  return _Jv_NewMultiArrayUnchecked (array_type, dimensions, sizes);
}



// Ensure 8-byte alignment, for hash synchronization.
#define DECLARE_PRIM_TYPE(NAME)			\
  _Jv_ArrayVTable _Jv_##NAME##VTable;		\
  java::lang::Class _Jv_##NAME##Class __attribute__ ((aligned (8)));

DECLARE_PRIM_TYPE(byte);
DECLARE_PRIM_TYPE(short);
DECLARE_PRIM_TYPE(int);
DECLARE_PRIM_TYPE(long);
DECLARE_PRIM_TYPE(boolean);
DECLARE_PRIM_TYPE(char);
DECLARE_PRIM_TYPE(float);
DECLARE_PRIM_TYPE(double);
DECLARE_PRIM_TYPE(void);

void
_Jv_InitPrimClass (jclass cl, char *cname, char sig, int len, 
                   _Jv_ArrayVTable *array_vtable)
{    
  using namespace java::lang::reflect;

  _Jv_InitNewClassFields (cl);

  // We must set the vtable for the class; the Java constructor
  // doesn't do this.
  (*(_Jv_VTable **) cl) = java::lang::Class::class$.vtable;

  // Initialize the fields we care about.  We do this in the same
  // order they are declared in Class.h.
  cl->name = _Jv_makeUtf8Const ((char *) cname, -1);
  cl->accflags = Modifier::PUBLIC | Modifier::FINAL | Modifier::ABSTRACT;
  cl->method_count = sig;
  cl->size_in_bytes = len;
  cl->vtable = JV_PRIMITIVE_VTABLE;
  cl->state = JV_STATE_DONE;
  cl->depth = -1;
  if (sig != 'V')
    _Jv_NewArrayClass (cl, NULL, (_Jv_VTable *) array_vtable);
}

jclass
_Jv_FindClassFromSignature (char *sig, java::lang::ClassLoader *loader)
{
  switch (*sig)
    {
    case 'B':
      return JvPrimClass (byte);
    case 'S':
      return JvPrimClass (short);
    case 'I':
      return JvPrimClass (int);
    case 'J':
      return JvPrimClass (long);
    case 'Z':
      return JvPrimClass (boolean);
    case 'C':
      return JvPrimClass (char);
    case 'F':
      return JvPrimClass (float);
    case 'D':
      return JvPrimClass (double);
    case 'V':
      return JvPrimClass (void);
    case 'L':
      {
	int i;
	for (i = 1; sig[i] && sig[i] != ';'; ++i)
	  ;
	_Jv_Utf8Const *name = _Jv_makeUtf8Const (&sig[1], i - 1);
	return _Jv_FindClass (name, loader);

      }
    case '[':
      {
	jclass klass = _Jv_FindClassFromSignature (&sig[1], loader);
	if (! klass)
	  return NULL;
	return _Jv_GetArrayClass (klass, loader);
      }
    }

  return NULL;			// Placate compiler.
}



JArray<jstring> *
JvConvertArgv (int argc, const char **argv)
{
  if (argc < 0)
    argc = 0;
  jobjectArray ar = JvNewObjectArray(argc, &StringClass, NULL);
  jobject *ptr = elements(ar);
  jbyteArray bytes = NULL;
  for (int i = 0;  i < argc;  i++)
    {
      const char *arg = argv[i];
      int len = strlen (arg);
      if (bytes == NULL || bytes->length < len)
	bytes = JvNewByteArray (len);
      jbyte *bytePtr = elements (bytes);
      // We assume jbyte == char.
      memcpy (bytePtr, arg, len);

      // Now convert using the default encoding.
      *ptr++ = new java::lang::String (bytes, 0, len);
    }
  return (JArray<jstring>*) ar;
}

// FIXME: These variables are static so that they will be
// automatically scanned by the Boehm collector.  This is needed
// because with qthreads the collector won't scan the initial stack --
// it will only scan the qthreads stacks.

// Command line arguments.
static JArray<jstring> *arg_vec;

// The primary thread.
static java::lang::Thread *main_thread;

char *
_Jv_ThisExecutable (void)
{
  return _Jv_execName;
}

void
_Jv_ThisExecutable (const char *name)
{
  if (name)
    {
      _Jv_execName = (char *) _Jv_Malloc (strlen (name) + 1);
      strcpy (_Jv_execName, name);
    }
}

#ifndef DISABLE_GETENV_PROPERTIES

static char *
next_property_key (char *s, size_t *length)
{
  size_t l = 0;

  JvAssert (s);

  // Skip over whitespace
  while (isspace (*s))
    s++;

  // If we've reached the end, return NULL.  Also return NULL if for
  // some reason we've come across a malformed property string.
  if (*s == 0
      || *s == ':'
      || *s == '=')
    return NULL;

  // Determine the length of the property key.
  while (s[l] != 0
	 && ! isspace (s[l])
	 && s[l] != ':'
	 && s[l] != '=')
    {
      if (s[l] == '\\'
	  && s[l+1] != 0)
	l++;
      l++;
    }

  *length = l;

  return s;
}

static char *
next_property_value (char *s, size_t *length)
{
  size_t l = 0;

  JvAssert (s);

  while (isspace (*s))
    s++;

  if (*s == ':'
      || *s == '=')
    s++;

  while (isspace (*s))
    s++;

  // If we've reached the end, return NULL.
  if (*s == 0)
    return NULL;

  // Determine the length of the property value.
  while (s[l] != 0
	 && ! isspace (s[l])
	 && s[l] != ':'
	 && s[l] != '=')
    {
      if (s[l] == '\\'
	  && s[l+1] != 0)
	l += 2;
      else
	l++;
    }

  *length = l;

  return s;
}

static void
process_gcj_properties ()
{
  char *props = getenv("GCJ_PROPERTIES");
  char *p = props;
  size_t length;
  size_t property_count = 0;

  if (NULL == props)
    return;

  // Whip through props quickly in order to count the number of
  // property values.
  while (p && (p = next_property_key (p, &length)))
    {
      // Skip to the end of the key
      p += length;

      p = next_property_value (p, &length);
      if (p)
	p += length;
      
      property_count++;
    }

  // Allocate an array of property value/key pairs.
  _Jv_Environment_Properties = 
    (property_pair *) malloc (sizeof(property_pair) 
			      * (property_count + 1));

  // Go through the properties again, initializing _Jv_Properties
  // along the way.
  p = props;
  property_count = 0;
  while (p && (p = next_property_key (p, &length)))
    {
      _Jv_Environment_Properties[property_count].key = p;
      _Jv_Environment_Properties[property_count].key_length = length;

      // Skip to the end of the key
      p += length;

      p = next_property_value (p, &length);
      
      _Jv_Environment_Properties[property_count].value = p;
      _Jv_Environment_Properties[property_count].value_length = length;

      if (p)
	p += length;

      property_count++;
    }
  memset ((void *) &_Jv_Environment_Properties[property_count], 
	  0, sizeof (property_pair));
  {
    size_t i = 0;

    // Null terminate the strings.
    while (_Jv_Environment_Properties[i].key)
      {
	_Jv_Environment_Properties[i].key[_Jv_Environment_Properties[i].key_length] = 0;
	_Jv_Environment_Properties[i++].value[_Jv_Environment_Properties[i].value_length] = 0;
      }
  }
}
#endif // DISABLE_GETENV_PROPERTIES

namespace gcj
{
  _Jv_Utf8Const *void_signature;
  _Jv_Utf8Const *clinit_name;
  _Jv_Utf8Const *init_name;
  _Jv_Utf8Const *finit_name;
  
  bool runtimeInitialized = false;
}

jint
_Jv_CreateJavaVM (void* /*vm_args*/)
{
  using namespace gcj;
  
  if (runtimeInitialized)
    return -1;

  runtimeInitialized = true;

  PROCESS_GCJ_PROPERTIES;

  _Jv_InitThreads ();
  _Jv_InitGC ();
  _Jv_InitializeSyncMutex ();

  /* Initialize Utf8 constants declared in jvm.h. */
  void_signature = _Jv_makeUtf8Const ("()V", 3);
  clinit_name = _Jv_makeUtf8Const ("<clinit>", 8);
  init_name = _Jv_makeUtf8Const ("<init>", 6);
  finit_name = _Jv_makeUtf8Const ("finit$", 6);

  /* Initialize built-in classes to represent primitive TYPEs. */
  _Jv_InitPrimClass (&_Jv_byteClass,    "byte",    'B', 1, &_Jv_byteVTable);
  _Jv_InitPrimClass (&_Jv_shortClass,   "short",   'S', 2, &_Jv_shortVTable);
  _Jv_InitPrimClass (&_Jv_intClass,     "int",     'I', 4, &_Jv_intVTable);
  _Jv_InitPrimClass (&_Jv_longClass,    "long",    'J', 8, &_Jv_longVTable);
  _Jv_InitPrimClass (&_Jv_booleanClass, "boolean", 'Z', 1, &_Jv_booleanVTable);
  _Jv_InitPrimClass (&_Jv_charClass,    "char",    'C', 2, &_Jv_charVTable);
  _Jv_InitPrimClass (&_Jv_floatClass,   "float",   'F', 4, &_Jv_floatVTable);
  _Jv_InitPrimClass (&_Jv_doubleClass,  "double",  'D', 8, &_Jv_doubleVTable);
  _Jv_InitPrimClass (&_Jv_voidClass,    "void",    'V', 0, &_Jv_voidVTable);

  // Turn stack trace generation off while creating exception objects.
  _Jv_InitClass (&java::lang::Throwable::class$);
  java::lang::Throwable::trace_enabled = 0;
  
  INIT_SEGV;
#ifdef HANDLE_FPE
  INIT_FPE;
#else
  arithexception = new java::lang::ArithmeticException
    (JvNewStringLatin1 ("/ by zero"));
#endif

  no_memory = new java::lang::OutOfMemoryError;

  java::lang::Throwable::trace_enabled = 1;

#ifdef USE_LTDL
  LTDL_SET_PRELOADED_SYMBOLS ();
#endif

  _Jv_platform_initialize ();

  _Jv_JNI_Init ();

  _Jv_GCInitializeFinalizers (&::gnu::gcj::runtime::FinalizerThread::finalizerReady);

  // Start the GC finalizer thread.  A VirtualMachineError can be
  // thrown by the runtime if, say, threads aren't available.  In this
  // case finalizers simply won't run.
  try
    {
      using namespace gnu::gcj::runtime;
      FinalizerThread *ft = new FinalizerThread ();
      ft->start ();
    }
  catch (java::lang::VirtualMachineError *ignore)
    {
    }

  return 0;
}

void
_Jv_RunMain (jclass klass, const char *name, int argc, const char **argv, 
	     bool is_jar)
{
  _Jv_argv = argv;
  _Jv_argc = argc;

  java::lang::Runtime *runtime = NULL;


#ifdef DISABLE_MAIN_ARGS
  _Jv_ThisExecutable ("[Embedded App]");
#else
#ifdef HAVE_PROC_SELF_EXE
  char exec_name[20];
  sprintf (exec_name, "/proc/%d/exe", getpid ());
  _Jv_ThisExecutable (exec_name);
#else
  _Jv_ThisExecutable (argv[0]);
#endif /* HAVE_PROC_SELF_EXE */
#endif /* DISABLE_MAIN_ARGS */

  try
    {
      // Set this very early so that it is seen when java.lang.System
      // is initialized.
      if (is_jar)
	_Jv_Jar_Class_Path = strdup (name);
      _Jv_CreateJavaVM (NULL);

      // Get the Runtime here.  We want to initialize it before searching
      // for `main'; that way it will be set up if `main' is a JNI method.
      runtime = java::lang::Runtime::getRuntime ();

#ifdef DISABLE_MAIN_ARGS
      arg_vec = JvConvertArgv (0, 0);
#else      
      arg_vec = JvConvertArgv (argc - 1, argv + 1);
#endif

      using namespace gnu::gcj::runtime;
      if (klass)
	main_thread = new FirstThread (klass, arg_vec);
      else
	main_thread = new FirstThread (JvNewStringLatin1 (name),
				       arg_vec, is_jar);
    }
  catch (java::lang::Throwable *t)
    {
      java::lang::System::err->println (JvNewStringLatin1 
        ("Exception during runtime initialization"));
      t->printStackTrace();
      runtime->exit (1);
    }

  _Jv_AttachCurrentThread (main_thread);
  _Jv_ThreadRun (main_thread);
  _Jv_ThreadWait ();

  int status = (int) java::lang::ThreadGroup::had_uncaught_exception;
  runtime->exit (status);
}

void
JvRunMain (jclass klass, int argc, const char **argv)
{
  _Jv_RunMain (klass, NULL, argc, argv, false);
}



// Parse a string and return a heap size.
static size_t
parse_heap_size (const char *spec)
{
  char *end;
  unsigned long val = strtoul (spec, &end, 10);
  if (*end == 'k' || *end == 'K')
    val *= 1024;
  else if (*end == 'm' || *end == 'M')
    val *= 1048576;
  return (size_t) val;
}

// Set the initial heap size.  This might be ignored by the GC layer.
// This must be called before _Jv_RunMain.
void
_Jv_SetInitialHeapSize (const char *arg)
{
  size_t size = parse_heap_size (arg);
  _Jv_GCSetInitialHeapSize (size);
}

// Set the maximum heap size.  This might be ignored by the GC layer.
// This must be called before _Jv_RunMain.
void
_Jv_SetMaximumHeapSize (const char *arg)
{
  size_t size = parse_heap_size (arg);
  _Jv_GCSetMaximumHeapSize (size);
}



void *
_Jv_Malloc (jsize size)
{
  if (__builtin_expect (size == 0, false))
    size = 1;
  void *ptr = malloc ((size_t) size);
  if (__builtin_expect (ptr == NULL, false))
    throw no_memory;
  return ptr;
}

void *
_Jv_Realloc (void *ptr, jsize size)
{
  if (__builtin_expect (size == 0, false))
    size = 1;
  ptr = realloc (ptr, (size_t) size);
  if (__builtin_expect (ptr == NULL, false))
    throw no_memory;
  return ptr;
}

void *
_Jv_MallocUnchecked (jsize size)
{
  if (__builtin_expect (size == 0, false))
    size = 1;
  return malloc ((size_t) size);
}

void
_Jv_Free (void* ptr)
{
  return free (ptr);
}



// In theory, these routines can be #ifdef'd away on machines which
// support divide overflow signals.  However, we never know if some
// code might have been compiled with "-fuse-divide-subroutine", so we
// always include them in libgcj.

jint
_Jv_divI (jint dividend, jint divisor)
{
  if (__builtin_expect (divisor == 0, false))
    _Jv_ThrowSignal (arithexception);
  
  if (dividend == (jint) 0x80000000L && divisor == -1)
    return dividend;

  return dividend / divisor;
}

jint
_Jv_remI (jint dividend, jint divisor)
{
  if (__builtin_expect (divisor == 0, false))
    _Jv_ThrowSignal (arithexception);
  
  if (dividend == (jint) 0x80000000L && divisor == -1)
    return 0;

  return dividend % divisor;
}

jlong
_Jv_divJ (jlong dividend, jlong divisor)
{
  if (__builtin_expect (divisor == 0, false))
    _Jv_ThrowSignal (arithexception);
  
  if (dividend == (jlong) 0x8000000000000000LL && divisor == -1)
    return dividend;

  return dividend / divisor;
}

jlong
_Jv_remJ (jlong dividend, jlong divisor)
{
  if (__builtin_expect (divisor == 0, false))
    _Jv_ThrowSignal (arithexception);
  
  if (dividend == (jlong) 0x8000000000000000LL && divisor == -1)
    return 0;

  return dividend % divisor;
}
