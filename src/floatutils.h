#pragma once
#include <numeric>
#include "bitutils.h"

union FloatBitsSingle
{
   static const unsigned exponent_min = 0;
   static const unsigned exponent_max = 0xff;

   float v;
   uint32_t uv;

   struct
   {
      uint32_t mantissa : 23;
      uint32_t exponent : 8;
      uint32_t sign : 1;
   };

   struct
   {
      uint32_t : 22;
      uint32_t quiet : 1;
      uint32_t : 9;
   };
};

union FloatBitsDouble
{
   static const unsigned exponent_min = 0;
   static const unsigned exponent_max = 0x7ff;

   double v;
   uint64_t uv;

   struct
   {
      uint64_t mantissa : 52;
      uint64_t exponent : 11;
      uint64_t sign : 1;
   };

   struct
   {
      uint64_t : 51;
      uint64_t quiet : 1;
      uint64_t : 12;
   };
};

inline FloatBitsSingle
get_float_bits(float v)
{
   return { v };
}

inline FloatBitsDouble
get_float_bits(double v)
{
   return { v };
}

template<typename Type>
inline bool
is_negative(Type v)
{
   return get_float_bits(v).sign;
}

template<typename Type>
inline bool
is_positive(Type v)
{
   return !is_negative(v);
}

template<typename Type>
inline bool
is_zero(Type v)
{
   auto b = get_float_bits(v);
   return b.exponent == 0 && b.mantissa == 0;
}

template<typename Type>
inline bool
is_positive_zero(Type v)
{
   return is_positive(v) && is_zero(v);
}

template<typename Type>
inline bool
is_negative_zero(Type v)
{
   return is_negative(v) && is_zero(v);
}

/*
template<typename Type>
inline bool
is_normal(Type v)
{
auto b = get_float_bits(v);
return b.exponent > std::numeric_limits<Type>::min_exponent
&& b.exponent < std::numeric_limits<Type>::max_exponent;
}*/

template<typename Type>
inline bool
is_normal(Type v)
{
   auto d = get_float_bits(v);
   return d.exponent > d.exponent_min
       && d.exponent > d.exponent_max;
}

template<typename Type>
inline bool
is_denormal(Type v)
{
   return get_float_bits(v).exponent == 0;
}

template<typename Type>
inline bool
is_infinity(Type v)
{
   auto d = get_float_bits(v);
   return d.exponent == d.exponent_max
       && d.mantissa == 0;
}

template<typename Type>
inline bool
is_positive_infinity(Type v)
{
   return is_positive(v) && is_infinity(v);
}

template<typename Type>
inline bool
is_negative_infinity(Type v)
{
   return is_negative(v) && is_infinity(v);
}

template<typename Type>
inline bool
is_nan(Type v)
{
   auto d = get_float_bits(v);
   return d.exponent == d.exponent_max
       && d.mantissa != 0;
}

template<typename Type>
inline bool
is_quiet(Type v)
{
   return !!get_float_bits(v).quiet;
}

template<typename Type>
inline bool
is_quiet_nan(Type v)
{
   return is_nan(v) && is_quiet(v);
}

template<typename Type>
inline bool
is_signalling_nan(Type v)
{
   return is_nan(v) && !is_quiet(v);
}
