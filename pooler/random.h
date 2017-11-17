/*
# This file is part of Primer Pooler v1.4 (c) 2016-17 Silas S. Brown.  For Wen.
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
/* Random number generator types: */
#define rt_System 0
#define rt_TinyMT 1
#define rt_XorShift 2
/* (these are listed in expected slow-to-fast order) */

#define RNG_Type rt_XorShift /* the type of RNG to use */
#define XorShift_is_Star 0

#if RNG_Type == rt_TinyMT
#include "tinymt32.h"
typedef tinymt32_t RandState; static RandState _RandState;
static inline int _TM32_rand() { return tinymt32_generate_uint32(&_RandState) & 0x7FFFFFFF; } static inline void _TM32_srand(unsigned seed) { tinymt32_init(&_RandState,seed); }
#define rand _TM32_rand
#define srand _TM32_srand

#elif RNG_Type == rt_XorShift
#include <stdint.h>
typedef uint64_t RandState; static RandState _RandState;
static inline int _XS_randr(RandState *s) {
  /* Xorshift was discovered by George Marsaglia 2003 and
     has 3 shifts + 3 XORs (total 6 operations) - should
     be faster than the 100MHz x86 RdRand of ~2015/16
     (even if you write code to make sure to use all 64 of
     its bits, you are limited to 6.4Gb/sec shared between
     all cores/threads; Xorshift at 2GHz 4-core =80Gb/s)*/
  RandState x = *s; x^=x>>12; x^=x<<25; x^=x>>27; *s=x;
  #if XorShift_is_Star
  x *= 2685821657736338717LL;
  #endif
  return x & 0x7FFFFFFF;
}
static inline int _XS_rand() { return _XS_randr(&_RandState); } static inline void _XS_srand(unsigned seed) { _RandState=seed; }
#define rand _XS_rand
#define srand _XS_srand

#elif RNG_Type == rt_System
#include <sys/param.h> /* needed for #ifdef BSD */
typedef unsigned long RandState;
#ifdef BSD
/* BSD has a bad old rand()/srand() implementation; use
   random()/srandom() instead (which is not standard C) */
#define rand random
#define srand srandom
#endif

#else
#error Unknown RNG_Type
#endif

static inline RandState RSInit(int i) {
  RandState r;
  #if RNG_Type == rt_TinyMT
  tinymt32_init(&r, i);
  #else
  r = i; /* rt_XorShift (we assume i != 0), rt_System */
  #endif
  return r;
}
 
static inline int portable_rand_r(RandState *state) {
  /* For 100% reproducibility across all platforms
     (provided the setting of RNGType is constant) */
  #if RNG_Type == rt_TinyMT
  return tinymt32_generate_uint32(state) & 0x7FFFFFFF;
  #elif RNG_Type == rt_XorShift
  return _XS_randr(state);
  #elif RNG_Type == rt_System
  /* The system RNG may vary across platforms, so we'd
     better provide an invariant.
     These numbers are courtesy of MS via WINE. */
  *state = (*state*214013L) + 2531011L;
  return ((*state) >> 16) & 0x7FFF;
  #endif
}

#if RNG_Type == rt_System
static int seedless_rand() {
  /* Non-reentrant version of the above (well, you could
     re-enter it if you like - after all it's supposed to
     be RANDOM - but doing so just might result in 2 or
     more threads getting too-similar sequences) */
  static RandState state = 1;
  return portable_rand_r(&state);
}
static inline int Rand(int use_portable) {
  return use_portable ? seedless_rand() : rand();
}
#else
/* non-System: rand() will already have been overridden */
static inline int Rand(int use_portable) { return rand(); }
#endif
