// std::ctype implementation details, GNU version -*- C++ -*-

// Copyright (C) 2001, 2002 Free Software Foundation, Inc.
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

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

//
// ISO C++ 14882: 22.2.1.1.2  ctype virtual functions.
//

// Written by Benjamin Kosnik <bkoz@redhat.com>

#include <locale>

namespace std
{
  // NB: The other ctype<char> specializations are in src/locale.cc and
  // various /config/os/* files.
  template<>
    ctype_byname<char>::ctype_byname(const char* __s, size_t __refs)
    : ctype<char>(0, false, __refs) 
    { 	
      if (_M_c_locale_ctype != _S_c_locale)
	_S_destroy_c_locale(_M_c_locale_ctype);
      _S_create_c_locale(_M_c_locale_ctype, __s); 
      _M_toupper = _M_c_locale_ctype->__ctype_toupper;
      _M_tolower = _M_c_locale_ctype->__ctype_tolower;
      _M_table = _M_c_locale_ctype->__ctype_b;
    }

#ifdef _GLIBCPP_USE_WCHAR_T  
  ctype<wchar_t>::__wmask_type
  ctype<wchar_t>::_M_convert_to_wmask(const mask __m) const
  {
    __wmask_type __ret;
    switch (__m)
      {
      case space:
	__ret = __wctype_l("space", _M_c_locale_ctype);
	break;
      case print:
	__ret = __wctype_l("print", _M_c_locale_ctype);
	break;
      case cntrl:
	__ret = __wctype_l("cntrl", _M_c_locale_ctype);
	break;
      case upper:
	__ret = __wctype_l("upper", _M_c_locale_ctype);
	break;
      case lower:
	__ret = __wctype_l("lower", _M_c_locale_ctype);
	break;
      case alpha:
	__ret = __wctype_l("alpha", _M_c_locale_ctype);
	break;
      case digit:
	__ret = __wctype_l("digit", _M_c_locale_ctype);
	break;
      case punct:
	__ret = __wctype_l("punct", _M_c_locale_ctype);
	break;
      case xdigit:
	__ret = __wctype_l("xdigit", _M_c_locale_ctype);
	break;
      case alnum:
	__ret = __wctype_l("alnum", _M_c_locale_ctype);
	break;
      case graph:
	__ret = __wctype_l("graph", _M_c_locale_ctype);
	break;
      default:
	__ret = 0;
      }
    return __ret;
  };
  
  wchar_t
  ctype<wchar_t>::do_toupper(wchar_t __c) const
  { return __towupper_l(__c, _M_c_locale_ctype); }

  const wchar_t*
  ctype<wchar_t>::do_toupper(wchar_t* __lo, const wchar_t* __hi) const
  {
    while (__lo < __hi)
      {
        *__lo = __towupper_l(*__lo, _M_c_locale_ctype);
        ++__lo;
      }
    return __hi;
  }
  
  wchar_t
  ctype<wchar_t>::do_tolower(wchar_t __c) const
  { return __towlower_l(__c, _M_c_locale_ctype); }
  
  const wchar_t*
  ctype<wchar_t>::do_tolower(wchar_t* __lo, const wchar_t* __hi) const
  {
    while (__lo < __hi)
      {
        *__lo = __towlower_l(*__lo, _M_c_locale_ctype);
        ++__lo;
      }
    return __hi;
  }

  bool
  ctype<wchar_t>::
  do_is(mask __m, char_type __c) const
  { 
    return static_cast<bool>(__iswctype_l(__c, _M_convert_to_wmask(__m), 
					  _M_c_locale_ctype)); 
  }
  
  const wchar_t* 
  ctype<wchar_t>::
  do_is(const wchar_t* __lo, const wchar_t* __hi, mask* __m) const
  {
    while (__lo < __hi && !this->do_is(*__m, *__lo))
      ++__lo;
    return __lo;
  }
  
  const wchar_t* 
  ctype<wchar_t>::
  do_scan_is(mask __m, const wchar_t* __lo, const wchar_t* __hi) const
  {
    while (__lo < __hi && !this->do_is(__m, *__lo))
      ++__lo;
    return __lo;
  }

  const wchar_t*
  ctype<wchar_t>::
  do_scan_not(mask __m, const char_type* __lo, const char_type* __hi) const
  {
    while (__lo < __hi && this->do_is(__m, *__lo) != 0)
      ++__lo;
    return __lo;
  }

  wchar_t
  ctype<wchar_t>::
  do_widen(char __c) const
  { return btowc(__c); }
  
  const char* 
  ctype<wchar_t>::
  do_widen(const char* __lo, const char* __hi, wchar_t* __dest) const
  {
    mbstate_t __state;
    memset(static_cast<void*>(&__state), 0, sizeof(mbstate_t));
    mbsrtowcs(__dest, &__lo, __hi - __lo, &__state);
    return __hi;
  }

  char
  ctype<wchar_t>::
  do_narrow(wchar_t __wc, char __dfault) const
  { 
    int __c = wctob(__wc);
    return (__c == EOF ? __dfault : static_cast<char>(__c)); 
  }

  const wchar_t*
  ctype<wchar_t>::
  do_narrow(const wchar_t* __lo, const wchar_t* __hi, char __dfault, 
	    char* __dest) const
  {
    mbstate_t __state;
    memset(static_cast<void*>(&__state), 0, sizeof(mbstate_t));
    size_t __len = __hi - __lo;
    size_t __conv = wcsrtombs(__dest, &__lo, __len, &__state);
    if (__conv == __len)
      *__dest = __dfault;
    return __hi;
  }
#endif //  _GLIBCPP_USE_WCHAR_T
}
