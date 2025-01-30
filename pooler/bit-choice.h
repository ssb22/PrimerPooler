/* bit-common.c is the only file that's allowed to include
   bit-choice.h, expanding to the architecture-dependent
   choice of (32 + 64) or (64 + 128) bit functions.
   All other modules that need these functions should
   include all-primers.h instead.
*/
#include "bit-basics.h"
#include "triangle.h"
#include "64.h"
#if Has_128bit
#include "128.h"
#include "64-128.h"
#else
#if CPU_64bit
#include "64-only.h"
#else
#include "32.h"
#include "32-64.h"
#endif
#endif
