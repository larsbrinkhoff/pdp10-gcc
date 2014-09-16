/* float.h for target with PDP-10 36-bit and 72-bit floating point formats */
#ifndef _FLOAT_H_
#define _FLOAT_H_

   /* Radix of exponent representation */
#undef FLT_RADIX
#define FLT_RADIX 2
   /* Number of base-FLT_RADIX digits in the significand of a float */
#undef FLT_MANT_DIG
#define FLT_MANT_DIG 27
   /* Number of decimal digits of precision in a float */
#undef FLT_DIG
#define FLT_DIG 8
   /* Addition rounds to 0: zero, 1: nearest, 2: +inf, 3: -inf, -1: unknown */
#undef FLT_ROUNDS
#define FLT_ROUNDS 1
   /* Difference between 1.0 and the minimum float greater than 1.0 */
#undef FLT_EPSILON
#define FLT_EPSILON 1.49011612e-08F
   /* Minimum int x such that FLT_RADIX**(x-1) is a normalised float */
#undef FLT_MIN_EXP
#define FLT_MIN_EXP (-128)
   /* Minimum normalised float */
#undef FLT_MIN
#define FLT_MIN 1.46936794e-39F
   /* Minimum int x such that 10**x is a normalised float */
#undef FLT_MIN_10_EXP
#define FLT_MIN_10_EXP (-38)
   /* Maximum int x such that FLT_RADIX**(x-1) is a representable float */
#undef FLT_MAX_EXP
#define FLT_MAX_EXP 127
   /* Maximum float */
#undef FLT_MAX
#define FLT_MAX 1.70141182e+38F
   /* Maximum int x such that 10**x is a representable float */
#undef FLT_MAX_10_EXP
#define FLT_MAX_10_EXP 38

#ifndef __GFLOAT__

   /* Number of base-FLT_RADIX digits in the significand of a double */
#undef DBL_MANT_DIG
#define DBL_MANT_DIG 62
   /* Number of decimal digits of precision in a double */
#undef DBL_DIG
#define DBL_DIG 18
   /* Difference between 1.0 and the minimum double greater than 1.0 */
#undef DBL_EPSILON
#define DBL_EPSILON 4.33680868994201774e-19
   /* Minimum int x such that FLT_RADIX**(x-1) is a normalised double */
#undef DBL_MIN_EXP
#define DBL_MIN_EXP (-128)
   /* Minimum normalised double */
#undef DBL_MIN
#define DBL_MIN 1.469367938527859384e-39
   /* Minimum int x such that 10**x is a normalised double */
#undef DBL_MIN_10_EXP
#define DBL_MIN_10_EXP (-38)
   /* Maximum int x such that FLT_RADIX**(x-1) is a representable double */
#undef DBL_MAX_EXP
#define DBL_MAX_EXP 127
   /* Maximum double */
#undef DBL_MAX
#define DBL_MAX 1.701411834604692317e+38
   /* Maximum int x such that 10**x is a representable double */
#undef DBL_MAX_10_EXP
#define DBL_MAX_10_EXP 38

#else /* __GFLOAT__ */

   /* Number of base-FLT_RADIX digits in the significand of a double */
#undef DBL_MANT_DIG
#define DBL_MANT_DIG 59
   /* Number of decimal digits of precision in a double */
#undef DBL_DIG
#define DBL_DIG 17
   /* Difference between 1.0 and the minimum double greater than 1.0 */
#undef DBL_EPSILON
#define DBL_EPSILON 3.4694469519536142e-18
   /* Minimum int x such that FLT_RADIX**(x-1) is a normalised double */
#undef DBL_MIN_EXP
#define DBL_MIN_EXP (-1024)
   /* Minimum normalised double */
#undef DBL_MIN
#define DBL_MIN 2.7813423231340017e-309
   /* Minimum int x such that 10**x is a normalised double */
#undef DBL_MIN_10_EXP
#define DBL_MIN_10_EXP (-308)
   /* Maximum int x such that FLT_RADIX**(x-1) is a representable double */
#undef DBL_MAX_EXP
#define DBL_MAX_EXP 1023
   /* Maximum double */
#undef DBL_MAX
#define DBL_MAX 8.9884656743115795e307
   /* Maximum int x such that 10**x is a representable double */
#undef DBL_MAX_10_EXP
#define DBL_MAX_10_EXP 307

#endif /* __GFLOAT__ */

#if 0 /* no long double */

#ifndef __GFLOAT__

   /* Number of base-FLT_RADIX digits in the significand of a long double */
#undef LDBL_MANT_DIG
#define LDBL_MANT_DIG 62
   /* Number of decimal digits of precision in a long double */
#undef LDBL_DIG
#define LDBL_DIG 18
   /* Difference between 1.0 and the minimum long double greater than 1.0 */
#undef LDBL_EPSILON
#define LDBL_EPSILON 4.33680868994201774e-19L
   /* Minimum int x such that FLT_RADIX**(x-1) is a normalised long double */
#undef LDBL_MIN_EXP
#define LDBL_MIN_EXP (-128)
   /* Minimum normalised long double */
#undef LDBL_MIN
#define LDBL_MIN 1.469367938527859384e-39L
   /* Minimum int x such that 10**x is a normalised long double */
#undef LDBL_MIN_10_EXP
#define LDBL_MIN_10_EXP (-38)
   /* Maximum int x such that FLT_RADIX**(x-1) is a representable long double */
#undef LDBL_MAX_EXP
#define LDBL_MAX_EXP 127
   /* Maximum long double */
#undef LDBL_MAX
#define LDBL_MAX 170141183460469232e+38L
   /* Maximum int x such that 10**x is a representable long double */
#undef LDBL_MAX_10_EXP
#define LDBL_MAX_10_EXP 38

#else /* __GFLOAT__ */

   /* Number of base-FLT_RADIX digits in the significand of a long double */
#undef LDBL_MANT_DIG
#define LDBL_MANT_DIG 59
   /* Number of decimal digits of precision in a long double */
#undef LDBL_DIG
#define LDBL_DIG 17
   /* Difference between 1.0 and the minimum long double greater than 1.0 */
#undef LDBL_EPSILON
#define LDBL_EPSILON 3.4694469519536142e-18L
   /* Minimum int x such that FLT_RADIX**(x-1) is a normalised long double */
#undef LDBL_MIN_EXP
#define LDBL_MIN_EXP (-1024)
   /* Minimum normalised long double */
#undef LDBL_MIN
#define LDBL_MIN 2.7813423231340017e-309L
   /* Minimum int x such that 10**x is a normalised long double */
#undef LDBL_MIN_10_EXP
#define LDBL_MIN_10_EXP (-308)
   /* Maximum int x such that FLT_RADIX**(x-1) is a representable long double */
#undef LDBL_MAX_EXP
#define LDBL_MAX_EXP 1023
   /* Maximum long double */
#undef LDBL_MAX
#define LDBL_MAX 8.9884656743115795e307L
   /* Maximum int x such that 10**x is a representable long double */
#undef LDBL_MAX_10_EXP
#define LDBL_MAX_10_EXP 307

#endif /* __GFLOAT__ */

#endif /* no LDBL */

#if __STDC_VERSION__ >= 199901L
   /* The floating-point expression evaluation method.
        -1  indeterminate
         0  evaluate all operations and constants just to the range and
            precision of the type
         1  evaluate operations and constants of type float and double
            to the range and precision of the double type, evaluate
            long double operations and constants to the range and
            precision of the long double type
         2  evaluate all operations and constants to the range and
            precision of the long double type
   */
# undef FLT_EVAL_METHOD
# define FLT_EVAL_METHOD	0

   /* Number of decimal digits to enable rounding to the given number of
      decimal digits without loss of precision.
         if FLT_RADIX == 10^n:  #mantissa * log10 (FLT_RADIX)
         else                :  ceil (1 + #mantissa * log10 (FLT_RADIX))
      where #mantissa is the number of bits in the mantissa of the widest
      supported floating-point type.
   */
# undef DECIMAL_DIG
#ifndef __GFLOAT__
# define DECIMAL_DIG	18
#else
# define DECIMAL_DIG	17
#endif

#endif	/* C99 */

#endif /*  _FLOAT_H_ */
