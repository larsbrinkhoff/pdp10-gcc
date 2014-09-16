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

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

#ifndef _GLIBCPP_CPU_LIMITS
#define _GLIBCPP_CPU_LIMITS 1

#define __glibcpp_char_bits		 9
#define __glibcpp_short_bits		18
#define __glibcpp_int_bits		36
#define __glibcpp_long_bits		36
#define __glibcpp_wchar_t_bits		36
#define __glibcpp_long_long_bits	72
#define __glibcpp_float_bits		36
#define __glibcpp_double_bits		72

#define __glibcpp_s9_max		255
#define __glibcpp_s9_min		-256
#define __glibcpp_s8_digits		8
#define __glibcpp_s9_digits10		3
#define __glibcpp_u9_min		0U
#define __glibcpp_u9_max		511
#define __glibcpp_u9_digits		9
#define __glibcpp_u9_digits10		3
#define __glibcpp_s18_max		131071
#define __glibcpp_s18_min		-131072
#define __glibcpp_s18_digits		17
#define __glibcpp_s18_digits10		6
#define __glibcpp_u18_min		0U
#define __glibcpp_u18_max		262143
#define __glibcpp_u18_digits		18
#define __glibcpp_u18_digits10		6
#define __glibcpp_s36_max		34359738367L
#define __glibcpp_s36_min		-34359738367L
#define __glibcpp_s36_digits		35
#define __glibcpp_s36_digits10		11
#define __glibcpp_u36_min		0UL
#define __glibcpp_u36_max		68719476736UL
#define __glibcpp_u36_digits		36
#define __glibcpp_u36_digits10		11
#define __glibcpp_s72_max		1180591620717411303423LL
#define __glibcpp_s72_min		-1180591620717411303424LL
#define __glibcpp_s72_digits		70
#define __glibcpp_s72_digits10		22
#define __glibcpp_u72_min		0ULL
#define __glibcpp_u72_max		2361183241434822606847ULL
#define __glibcpp_u72_digits		71
#define __glibcpp_u72_digits10		22

#define __glibcpp_f36_min		1.46936794e-39F
#define __glibcpp_f36_max		1.70141182e+38F
#define __glibcpp_f36_digits		27
#define __glibcpp_f36_digits10		8
#define __glibcpp_f36_radix		2
#define __glibcpp_f36_epsilon		1.49011612e-08F
#define __glibcpp_f36_round_error	1.0F
#define __glibcpp_f36_min_exponent	-128
#define __glibcpp_f36_min_exponent10	-38
#define __glibcpp_f36_max_exponent	127
#define __glibcpp_f36_max_exponent10	38
#define __glibcpp_f72_min		1.469367938527859384e-39
#define __glibcpp_f72_max		1.701411834604692317e+38
#define __glibcpp_f72_digits		62
#define __glibcpp_f72_digits10		18
#define __glibcpp_f72_radix		2
#define __glibcpp_f72_epsilon		4.33680868994201774e-19
#define __glibcpp_f72_round_error	1.0
#define __glibcpp_f72_min_exponent	-128
#define __glibcpp_f72_min_exponent10	-38
#define __glibcpp_f72_max_exponent	127
#define __glibcpp_f72_max_exponent10	38

#define __glibcpp_signed_char_min __glibcpp_s9_min
#define __glibcpp_signed_char_max __glibcpp_s9_max
#define __glibcpp_signed_char_digits __glibcpp_s9_digits
#define __glibcpp_signed_char_digits10 __glibcpp_s9_digits10
#define __glibcpp_unsigned_char_min __glibcpp_u9_min
#define __glibcpp_unsigned_char_max __glibcpp_u9_max
#define __glibcpp_unsigned_char_digits __glibcpp_u9_digits
#define __glibcpp_unsigned_char_digits10 __glibcpp_u9_digits10

#define __glibcpp_signed_short_min __glibcpp_s18_min
#define __glibcpp_signed_short_max __glibcpp_s18_max
#define __glibcpp_signed_short_digits __glibcpp_s18_digits
#define __glibcpp_signed_short_digits10 __glibcpp_s18_digits10
#define __glibcpp_unsigned_short_min __glibcpp_u18_min
#define __glibcpp_unsigned_short_max __glibcpp_u18_max
#define __glibcpp_unsigned_short_digits __glibcpp_u18_digits
#define __glibcpp_unsigned_short_digits10 __glibcpp_u18_digits10

#define __glibcpp_signed_int_min (int)__glibcpp_s36_min
#define __glibcpp_signed_int_max (int)__glibcpp_s36_max
#define __glibcpp_signed_int_digits __glibcpp_s36_digits
#define __glibcpp_signed_int_digits10 __glibcpp_s36_digits10
#define __glibcpp_unsigned_int_min (unsigned)__glibcpp_u36_min
#define __glibcpp_unsigned_int_max (unsigned)__glibcpp_u36_max
#define __glibcpp_unsigned_int_digits __glibcpp_u36_digits
#define __glibcpp_unsigned_int_digits10 __glibcpp_u36_digits10

#define __glibcpp_signed_long_min __glibcpp_s36_min
#define __glibcpp_signed_long_max __glibcpp_s36_max
#define __glibcpp_signed_long_digits __glibcpp_s36_digits
#define __glibcpp_signed_long_digits10 __glibcpp_s36_digits10
#define __glibcpp_unsigned_long_min __glibcpp_u36_min
#define __glibcpp_unsigned_long_max __glibcpp_u36_max
#define __glibcpp_unsigned_long_digits __glibcpp_u36_digits
#define __glibcpp_unsigned_long_digits10 __glibcpp_u36_digits10

#define __glibcpp_signed_long_long_min __glibcpp_s72_min
#define __glibcpp_signed_long_long_max __glibcpp_s72_max
#define __glibcpp_signed_long_long_digits __glibcpp_s72_digits
#define __glibcpp_signed_long_long_digits10 __glibcpp_s72_digits10
#define __glibcpp_signed_long_long_traps true
#define __glibcpp_unsigned_long_long_min __glibcpp_u72_min
#define __glibcpp_unsigned_long_long_max __glibcpp_u72_max
#define __glibcpp_unsigned_long_long_digits __glibcpp_u72_digits
#define __glibcpp_unsigned_long_long_digits10 __glibcpp_u72_digits10
#define __glibcpp_unsigned_long_long_traps true

#if __glibcpp_wchar_t_is_signed
#define __glibcpp_wchar_t_min (wchar_t)__glibcpp_s36_min
#define __glibcpp_wchar_t_max (wchar_t)__glibcpp_s36_max
#define __glibcpp_wchar_t_digits __glibcpp_s36_digits
#define __glibcpp_wchar_t_digits10 __glibcpp_s36_digits10
#else
#define __glibcpp_wchar_t_min (wchar_t)__glibcpp_u36_min
#define __glibcpp_wchar_t_max (wchar_t)__glibcpp_u36_max
#define __glibcpp_wchar_t_digits __glibcpp_u36_digits
#define __glibcpp_wchar_t_digits10 __glibcpp_u36_digits10
#endif

#define __glibcpp_float_min __glibcpp_f36_min
#define __glibcpp_float_max __glibcpp_f36_max
#define __glibcpp_float_digits __glibcpp_f36_digits
#define __glibcpp_float_digits10 __glibcpp_f36_digits10
#define __glibcpp_float_radix __glibcpp_f36_radix
#define __glibcpp_float_epsilon __glibcpp_f36_epsilon
#define __glibcpp_float_round_error __glibcpp_f36_round_error
#define __glibcpp_float_min_exponent __glibcpp_f36_min_exponent
#define __glibcpp_float_min_exponent10 __glibcpp_f36_min_exponent10
#define __glibcpp_float_max_exponent __glibcpp_f36_max_exponent
#define __glibcpp_float_max_exponent10 __glibcpp_f36_max_exponent10

#define __glibcpp_double_min __glibcpp_f72_min
#define __glibcpp_double_max __glibcpp_f72_max
#define __glibcpp_double_digits __glibcpp_f72_digits
#define __glibcpp_double_digits10 __glibcpp_f72_digits10
#define __glibcpp_double_radix __glibcpp_f72_radix
#define __glibcpp_double_epsilon __glibcpp_f72_epsilon
#define __glibcpp_double_round_error __glibcpp_f72_round_error
#define __glibcpp_double_min_exponent __glibcpp_f72_min_exponent
#define __glibcpp_double_min_exponent10 __glibcpp_f72_min_exponent10
#define __glibcpp_double_max_exponent __glibcpp_f72_max_exponent
#define __glibcpp_double_max_exponent10 __glibcpp_f72_max_exponent10

#endif
