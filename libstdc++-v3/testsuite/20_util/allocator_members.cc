// 2001-06-14  Benjamin Kosnik  <bkoz@redhat.com>

// Copyright (C) 2001 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// 20.4.1.1 allocator members

#include <memory>
#include <cstdlib>
#include <testsuite_hooks.h>

struct gnu { };

bool check_new = false;
bool check_delete = false;

void* 
operator new(std::size_t n) throw(std::bad_alloc)
{
  check_new = true;
  return std::malloc(n);
}

void operator delete(void *v) throw()
{
  check_delete = true;
  return std::free(v);
}

int main(void)
{
  bool test = true;
  std::allocator<gnu> obj;

  // XXX These should work for various size allocation and
  // deallocations.  Currently, they only work as expected for sizes >
  // _MAX_BYTES as defined in stl_alloc.h, which happes to be 128. 
  gnu* pobj = obj.allocate(256);
  VERIFY( check_new );

  obj.deallocate(pobj, 256);
  VERIFY( check_delete );

  return 0;
}
