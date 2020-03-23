/*
# This file is part of Primer Pooler v1.7 (c) 2016-20 Silas S. Brown.  For Wen.
# 
# This program is free software; you can redistribute and
# modify it under the terms of the General Public License
# as published by the Free Software Foundation; either
# version 3 of the License, or any later version.
#
# This program is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY.  See the GNU General
# Public License for more details.
*/
#include <ctype.h>
#include <stdint.h>
typedef uint32_t bit32;
typedef uint64_t bit64;
typedef uint64_t ULL; /* for bit64 w/out sed changes */
#if defined(__x86_64) && (defined(__GNUC__) || defined(__clang__)) && !defined(Has_128bit)
/* __uint128_t is available in x86-64 on GCC and clang,
   but not MSVC (as of 2015); don't know about Intel */
#define Has_128bit 1
#endif
#if Has_128bit
typedef __uint128_t bit128;
#endif

static inline int leading0_32(bit32 b) {
  return __builtin_clz(b);
}
static inline int leading0_64(bit64 b) {
  return __builtin_clzll(b);
}
#if Has_128bit
static inline int leading0_128(bit128 b) {
  if (b < (bit128)0xFFFFFFFFFFFFFFFFUL)
    return 64+leading0_64((bit64)b);
  else
    return leading0_64((bit64)(b>>64));
}
#endif
static inline int trail0_32(bit32 b) {
  return __builtin_ctz(b);
}
static inline int trail0_64(bit64 b) {
  return __builtin_ctzll(b);
}
#if Has_128bit
static inline int trail0_128(bit128 b) {
  if (!(b & (bit128)0xFFFFFFFFFFFFFFFFUL))
    return 64+trail0_64((bit64)(b>>64));
  else
    return trail0_64((bit64)b);
}
#endif
static inline int popcount32(bit32 i) {
  return __builtin_popcount(i);
}
static inline int popcount64(bit64 i) {
  return __builtin_popcountll(i);
}
#if Has_128bit
static inline int popcount128(bit128 i) {
  return popcount64((bit64)i) + popcount64((bit64)(i>>64));
}
#endif
static inline char fast_toUpper(char c) {
  /* nowhere near the critical path, but as we're talking
     about bit patterns, might as well do this:
     For our purposes we can leave the output undefined
     when c is not an ASCII alphabetical letter (as long
     as an input of 0 still gives 0 output), so: */
  if ('A' == 0x41 && 'a' == 0x61) return c & ~0x20;
  else return toupper(c); // non-ASCII system??
}
