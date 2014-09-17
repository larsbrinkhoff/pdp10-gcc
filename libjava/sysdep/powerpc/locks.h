// locks.h - Thread synchronization primitives. PowerPC implementation.

/* Copyright (C) 2002  Free Software Foundation

   This file is part of libgcj.

This software is copyrighted work licensed under the terms of the
Libgcj License.  Please consult the file "LIBGCJ_LICENSE" for
details.  */

#ifndef __SYSDEP_LOCKS_H__
#define __SYSDEP_LOCKS_H__

typedef size_t obj_addr_t;	/* Integer type big enough for object	*/
				/* address.				*/

inline static bool
compare_and_swap(volatile obj_addr_t *addr,
		  			      obj_addr_t old,
					      obj_addr_t new_val) 
{
  int ret;

  __asm__ __volatile__ (
	   "0:    lwarx %0,0,%1 ;"
	   "      xor. %0,%3,%0;"
	   "      bne 1f;"
	   "      stwcx. %2,0,%1;"
	   "      bne- 0b;"
	   "1:    "
	: "=&r"(ret)
	: "r"(addr), "r"(new_val), "r"(old)
	: "cr0", "memory");
  /* This version of __compare_and_swap is to be used when acquiring
     a lock, so we don't need to worry about whether other memory
     operations have completed, but we do need to be sure that any loads
     after this point really occur after we have acquired the lock.  */
  __asm__ __volatile__ ("isync" : : : "memory");
  return ret == 0;
}

inline static void
release_set(volatile obj_addr_t *addr, obj_addr_t new_val)
{
  __asm__ __volatile__ ("sync" : : : "memory");
  *(addr) = new_val;
}

inline static bool
compare_and_swap_release(volatile obj_addr_t *addr,
		  				     obj_addr_t old,
						     obj_addr_t new_val)
{
  int ret;

  __asm__ __volatile__ ("sync" : : : "memory");
  __asm__ __volatile__ (
	   "0:    lwarx %0,0,%1 ;"
	   "      xor. %0,%3,%0;"
	   "      bne 1f;"
	   "      stwcx. %2,0,%1;"
	   "      bne- 0b;"
	   "1:    "
	: "=&r"(ret)
	: "r"(addr), "r"(new_val), "r"(old)
	: "cr0", "memory");
  return ret == 0;
}

// Ensure that subsequent instructions do not execute on stale
// data that was loaded from memory before the barrier.
inline static void
read_barrier()
{
  __asm__ __volatile__ ("isync" : : : "memory");
}

// Ensure that prior stores to memory are completed with respect to other
// processors.
inline static void
write_barrier()
{
  __asm__ __volatile__ ("sync" : : : "memory");
}

#endif
