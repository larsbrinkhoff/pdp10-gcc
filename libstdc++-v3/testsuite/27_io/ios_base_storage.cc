// 2000-12-19 bkoz

// Copyright (C) 2000, 2002 Free Software Foundation
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

// 27.4.2.5 ios_base storage functions

#include <sstream>
#include <iostream>

#include <testsuite_hooks.h>

// http://gcc.gnu.org/ml/gcc-bugs/2000-12/msg00413.html
void test01() 
{
  bool test = true;
  
  using namespace std;

  long x1 = ios::xalloc();
  long x2 = ios::xalloc();
  long x3 = ios::xalloc();
  long x4 = ios::xalloc();

  ostringstream out("the element of crime, lars von trier");
  out.pword(++x4); // should not crash
}

// libstdc++/3129
void test02()
{
  bool test = true;
  int max = std::numeric_limits<int>::max();
  std::stringbuf        strbuf;
  std::ios              ios(&strbuf);

  long l = 0;
  void* v = 0;

  // pword
  try 
    {
      v = ios.pword(max);
    }
  catch(std::ios_base::failure& obj)
    {
      // Ok.
      VERIFY( ios.bad() );
    }
  catch(...)
    {
      VERIFY( test = false );
    }
  VERIFY( v == 0 );

  // iword
  try 
    {
      l = ios.iword(max);
    }
  catch(std::ios_base::failure& obj)
    {
      // Ok.
      VERIFY( ios.bad() );
    }
  catch(...)
    {
      VERIFY( test = false );
    }
  VERIFY( l == 0 );
}

int main(void)
{
  test01();
  test02();
  return 0;
}
