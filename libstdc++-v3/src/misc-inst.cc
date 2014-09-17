// Explicit instantiation file.

// Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002
// Free Software Foundation, Inc.
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
// ISO C++ 14882:
//

#include <string>
#include <algorithm>
#include <locale>
#include <vector>
#include <iterator>
#include <streambuf>
#include <sstream>
#include <fstream>
#include <ios>
#include <istream>
#include <ostream>
#include <iomanip>

// NB: Unnecessary if the .h headers already include these.
#ifndef  _GLIBCPP_FULLY_COMPLIANT_HEADERS
#include <bits/sstream.tcc>
#include <bits/fstream.tcc>
#include <bits/streambuf.tcc>
#include <bits/istream.tcc>
#include <bits/ostream.tcc>
#endif

namespace std
{
  // streambuf
  template class basic_streambuf<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_streambuf<wchar_t>;
#endif

  // stringbuf
  template class basic_stringbuf<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_stringbuf<wchar_t>;
#endif

  // filebuf
  template class basic_filebuf<char, char_traits<char> >;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_filebuf<wchar_t, char_traits<wchar_t> >;
#endif

  // basic_ios
  template class basic_ios<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_ios<wchar_t>;
#endif

  // iomanip
  template class _Setfill<char>;
  template _Setfill<char> setfill(char);
#ifdef _GLIBCPP_USE_WCHAR_T
  template class _Setfill<wchar_t>;
  template _Setfill<wchar_t> setfill(wchar_t);
#endif

  // istream
  template class basic_istream<char>;
  template istream& ws(istream&);
  template istream& operator>>(istream&, char&);
  template istream& operator>>(istream&, unsigned char&);
  template istream& operator>>(istream&, signed char&);
  template istream& operator>>(istream&, char*);
  template istream& operator>>(istream&, unsigned char*);
  template istream& operator>>(istream&, signed char*);

  template istream& operator>>(istream&, _Setfill<char>);
  template istream& operator>>(istream&, _Setiosflags);
  template istream& operator>>(istream&, _Resetiosflags);
  template istream& operator>>(istream&, _Setbase);
  template istream& operator>>(istream&, _Setprecision);
  template istream& operator>>(istream&, _Setw);

#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_istream<wchar_t>;
  template wistream& ws(wistream&);
  template wistream& operator>>(wistream&, wchar_t&);
  template wistream& operator>>(wistream&, wchar_t*);

  template wistream& operator>>(wistream&, _Setfill<wchar_t>);
  template wistream& operator>>(wistream&, _Setiosflags);
  template wistream& operator>>(wistream&, _Resetiosflags);
  template wistream& operator>>(wistream&, _Setbase);
  template wistream& operator>>(wistream&, _Setprecision);
  template wistream& operator>>(wistream&, _Setw);
#endif

  // ostream
  template class basic_ostream<char>;
  template ostream& endl(ostream&);
  template ostream& ends(ostream&);
  template ostream& flush(ostream&);
  template ostream& operator<<(ostream&, char);
  template ostream& operator<<(ostream&, unsigned char);
  template ostream& operator<<(ostream&, signed char);
  template ostream& operator<<(ostream&, const char*);
  template ostream& operator<<(ostream&, const unsigned char*);
  template ostream& operator<<(ostream&, const signed char*);

  template ostream& operator<<(ostream&, _Setfill<char>);
  template ostream& operator<<(ostream&, _Setiosflags);
  template ostream& operator<<(ostream&, _Resetiosflags);
  template ostream& operator<<(ostream&, _Setbase);
  template ostream& operator<<(ostream&, _Setprecision);
  template ostream& operator<<(ostream&, _Setw);

#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_ostream<wchar_t>;
  template wostream& endl(wostream&);
  template wostream& ends(wostream&);
  template wostream& flush(wostream&);
  template wostream& operator<<(wostream&, wchar_t);
  template wostream& operator<<(wostream&, char);
  template wostream& operator<<(wostream&, const wchar_t*);
  template wostream& operator<<(wostream&, const char*);

  template wostream& operator<<(wostream&, _Setfill<wchar_t>);
  template wostream& operator<<(wostream&, _Setiosflags);
  template wostream& operator<<(wostream&, _Resetiosflags);
  template wostream& operator<<(wostream&, _Setbase);
  template wostream& operator<<(wostream&, _Setprecision);
  template wostream& operator<<(wostream&, _Setw);
#endif
  

  // iostream
  template class basic_iostream<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_iostream<wchar_t>; 
#endif

  // ifstream
  template class basic_ifstream<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_ifstream<wchar_t>;
#endif

  // ofstream
  template class basic_ofstream<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_ofstream<wchar_t>;
#endif

  // fstream
  template class basic_fstream<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_fstream<wchar_t>;
#endif

  // istringstream
  template class basic_istringstream<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_istringstream<wchar_t>; 
#endif

  // ostringstream
  template class basic_ostringstream<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_ostringstream<wchar_t>; 
#endif

  // stringstream
  template class basic_stringstream<char>;
#ifdef _GLIBCPP_USE_WCHAR_T
  template class basic_stringstream<wchar_t>; 
#endif

  // string related to iostreams
  template 
    basic_istream<char>& 
    operator>>(basic_istream<char>&, string&);
  template 
    basic_ostream<char>& 
    operator<<(basic_ostream<char>&, const string&);
  template 
    basic_istream<char>& 
    getline(basic_istream<char>&, string&, char);
  template 
    basic_istream<char>& 
    getline(basic_istream<char>&, string&);
#ifdef _GLIBCPP_USE_WCHAR_T
  template 
    basic_istream<wchar_t>& 
    operator>>(basic_istream<wchar_t>&, wstring&);
  template 
    basic_ostream<wchar_t>& 
    operator<<(basic_ostream<wchar_t>&, const wstring&);
  template 
    basic_istream<wchar_t>& 
    getline(basic_istream<wchar_t>&, wstring&, wchar_t);
  template 
    basic_istream<wchar_t>& 
    getline(basic_istream<wchar_t>&, wstring&);
#endif

  // algorithm
  typedef  _Char_traits_match<char, char_traits<char> > char_match;

  template 
    const char*  
    find_if<const char *, char_match>
    (const char *, const char *, char_match, random_access_iterator_tag);

#ifdef _GLIBCPP_USE_WCHAR_T
  typedef  _Char_traits_match<wchar_t, char_traits<wchar_t> > wchar_match;

  template const wchar_t*  
    find_if<const wchar_t*, wchar_match>
    (const wchar_t*, const wchar_t*, wchar_match, random_access_iterator_tag);
#endif
  
  template 
    string* 
    __uninitialized_fill_n_aux<string*, size_t, string>
    (string*, size_t, string const &, __false_type);

  template 
    string* 
    __uninitialized_copy_aux<vector<string>::const_iterator, string *>
    (vector<string>::const_iterator, vector<string>::const_iterator, 
     string*, __false_type);

  template
    streamsize
    __copy_streambufs(basic_ios<char>&, basic_streambuf<char>*,
		      basic_streambuf<char>*); 
#ifdef _GLIBCPP_USE_WCHAR_T
  template
    streamsize
    __copy_streambufs(basic_ios<wchar_t>&, basic_streambuf<wchar_t>*,
		      basic_streambuf<wchar_t>*); 
#endif
} //std
