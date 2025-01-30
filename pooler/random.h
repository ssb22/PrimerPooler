/*
This file is part of Primer Pooler (c) Silas S. Brown.  For Wen.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef XorShift_no_star
#define XorShift_no_star 0
/* #define this to 1 for a marginal speed improvement at
   the expense of randomness
   (gives a different sequence).
   Versions of Primer Pooler before v1.41 effectively had
   #define XorShift_no_star 1
*/
#endif

#include <stdint.h>
typedef uint64_t RandState;
static inline int _XS_randr(RandState *s) {
  /* Xorshift was discovered by George Marsaglia 2003 and
     has 3 shifts + 3 XORs (total 6 operations);
     XorShift* 64/32 adds a mult (3 cycles pipelinable) +
     another shift; both should be faster than x86 RdRand
     (limited to 6.4Gb/sec shared between all cores) */
  RandState x = *s; x^=x>>12; x^=x<<25; x^=x>>27; *s=x;
  #if XorShift_no_star
  return x & 0x7FFFFFFF; /* as per versions before 1.41 (high bits might have been better than low bits though) */
  #else
  return (x * 2685821657736338717LL) >> 33;
  /* (RandState is unsigned, so >> will be a logical, not
     arithmetic, right shift, so won't fill with 1's, but
     if int is 32-bit we'd better make sure the result
     fits within 0x7FFFFFFF, so shift 33 bits to save
     having to AND it off at the end) */
  #endif
}

static RandState _RandState;
/* (we assume srand will be called with non-0 argument) */

static inline int _XS_rand() {
  return _XS_randr(&_RandState);
}
static inline void _XS_srand(unsigned seed) {
  _RandState = seed;
}

#ifdef rand
#undef rand
#endif
#ifdef srand
#undef srand
#endif
#ifdef rand_r
#undef rand_r
#endif

#define rand _XS_rand
#define srand _XS_srand
#define rand_r _XS_randr
